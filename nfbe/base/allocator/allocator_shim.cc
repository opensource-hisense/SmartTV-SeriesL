// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/allocator/allocator_shim.h"

#include <errno.h>
#include <unistd.h>

#include <new>

#include "base/atomicops.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/threading/platform_thread.h"
#include "build/build_config.h"

// No calls to malloc / new in this file. They would would cause re-entrancy of
// the shim, which is hard to deal with. Keep this code as simple as possible
// and don't use any external C++ object here, not even //base ones. Even if
// they are safe to use today, in future they might be refactored.

namespace {

using namespace base;

subtle::AtomicWord g_chain_head = reinterpret_cast<subtle::AtomicWord>(
    &allocator::AllocatorDispatch::default_dispatch);

bool g_call_new_handler_on_malloc_failure = false;
subtle::Atomic32 g_new_handler_lock = 0;

// In theory this should be just base::ThreadChecker. But we can't afford
// the luxury of a LazyInstance<ThreadChecker> here as it would cause a new().
bool CalledOnValidThread() {
  using subtle::Atomic32;
  const Atomic32 kInvalidTID = static_cast<Atomic32>(kInvalidThreadId);
  static Atomic32 g_tid = kInvalidTID;
  Atomic32 cur_tid = static_cast<Atomic32>(PlatformThread::CurrentId());
  Atomic32 prev_tid =
      subtle::NoBarrier_CompareAndSwap(&g_tid, kInvalidTID, cur_tid);
  return prev_tid == kInvalidTID || prev_tid == cur_tid;
}

inline size_t GetPageSize() {
  static size_t pagesize = 0;
  if (!pagesize)
    pagesize = sysconf(_SC_PAGESIZE);
  return pagesize;
}

// Calls the std::new handler thread-safely. Returns true if a new_handler was
// set and called, false if no new_handler was set.
bool CallNewHandler() {
  // TODO(primiano): C++11 has introduced ::get_new_handler() which is supposed
  // to be thread safe and would avoid the spinlock boilerplate here. However
  // it doesn't seem to be available yet in the Linux chroot headers yet.
  std::new_handler nh;
  {
    while (subtle::Acquire_CompareAndSwap(&g_new_handler_lock, 0, 1))
      PlatformThread::YieldCurrentThread();
    nh = std::set_new_handler(0);
    ignore_result(std::set_new_handler(nh));
    subtle::Release_Store(&g_new_handler_lock, 0);
  }
  if (!nh)
    return false;
  (*nh)();
  // Assume the new_handler will abort if it fails. Exception are disabled and
  // we don't support the case of a new_handler throwing std::bad_balloc.
  return true;
}

inline const allocator::AllocatorDispatch* GetChainHead() {
  // TODO(primiano): Just use NoBarrier_Load once crbug.com/593344 is fixed.
  // Unfortunately due to that bug NoBarrier_Load() is mistakenly fully
  // barriered on Linux+Clang, and that causes visible perf regressons.
  return reinterpret_cast<const allocator::AllocatorDispatch*>(
#if defined(OS_LINUX) && defined(__clang__)
      *static_cast<const volatile subtle::AtomicWord*>(&g_chain_head)
#else
      subtle::NoBarrier_Load(&g_chain_head)
#endif
  );
}

}  // namespace

namespace base {
namespace allocator {

void SetCallNewHandlerOnMallocFailure(bool value) {
  g_call_new_handler_on_malloc_failure = value;
}

void* UncheckedAlloc(size_t size) {
  const allocator::AllocatorDispatch* const chain_head = GetChainHead();
  return chain_head->alloc_function(chain_head, size);
}

void InsertAllocatorDispatch(AllocatorDispatch* dispatch) {
  // Ensure this is always called on the same thread.
  DCHECK(CalledOnValidThread());

  dispatch->next = GetChainHead();

  // This function does not guarantee to be thread-safe w.r.t. concurrent
  // insertions, but still has to guarantee that all the threads always
  // see a consistent chain, hence the MemoryBarrier() below.
  // InsertAllocatorDispatch() is NOT a fastpath, as opposite to malloc(), so
  // we don't really want this to be a release-store with a corresponding
  // acquire-load during malloc().
  subtle::MemoryBarrier();

  subtle::NoBarrier_Store(&g_chain_head,
                          reinterpret_cast<subtle::AtomicWord>(dispatch));
}

void RemoveAllocatorDispatchForTesting(AllocatorDispatch* dispatch) {
  DCHECK(CalledOnValidThread());
  DCHECK_EQ(GetChainHead(), dispatch);
  subtle::NoBarrier_Store(&g_chain_head,
                          reinterpret_cast<subtle::AtomicWord>(dispatch->next));
}

}  // namespace allocator
}  // namespace base

// The Shim* functions below are the entry-points into the shim-layer and
// are supposed to be invoked / aliased by the allocator_shim_override_*
// headers to route the malloc / new symbols through the shim layer.
extern "C" {

// The general pattern for allocations is:
// - Try to allocate, if succeded return the pointer.
// - If the allocation failed:
//   - Call the std::new_handler if it was a C++ allocation.
//   - Call the std::new_handler if it was a malloc() (or calloc() or similar)
//     AND SetCallNewHandlerOnMallocFailure(true).
//   - If the std::new_handler is NOT set just return nullptr.
//   - If the std::new_handler is set:
//     - Assume it will abort() if it fails (very likely the new_handler will
//       just suicide priting a message).
//     - Assume it did succeed if it returns, in which case reattempt the alloc.

void* ShimCppNew(size_t size) {
  const allocator::AllocatorDispatch* const chain_head = GetChainHead();
  void* ptr;
  do {
    ptr = chain_head->alloc_function(chain_head, size);
  } while (!ptr && CallNewHandler());
  return ptr;
}

void ShimCppDelete(void* address) {
  const allocator::AllocatorDispatch* const chain_head = GetChainHead();
  return chain_head->free_function(chain_head, address);
}

void* ShimMalloc(size_t size) {
  const allocator::AllocatorDispatch* const chain_head = GetChainHead();
  void* ptr;
  do {
    ptr = chain_head->alloc_function(chain_head, size);
  } while (!ptr && g_call_new_handler_on_malloc_failure && CallNewHandler());
  return ptr;
}

void* ShimCalloc(size_t n, size_t size) {
  const allocator::AllocatorDispatch* const chain_head = GetChainHead();
  void* ptr;
  do {
    ptr = chain_head->alloc_zero_initialized_function(chain_head, n, size);
  } while (!ptr && g_call_new_handler_on_malloc_failure && CallNewHandler());
  return ptr;
}

void* ShimRealloc(void* address, size_t size) {
  // realloc(size == 0) means free() and might return a nullptr. We should
  // not call the std::new_handler in that case, though.
  const allocator::AllocatorDispatch* const chain_head = GetChainHead();
  void* ptr;
  do {
    ptr = chain_head->realloc_function(chain_head, address, size);
  } while (!ptr && size && g_call_new_handler_on_malloc_failure &&
           CallNewHandler());
  return ptr;
}

void* ShimMemalign(size_t alignment, size_t size) {
  const allocator::AllocatorDispatch* const chain_head = GetChainHead();
  void* ptr;
  do {
    ptr = chain_head->alloc_aligned_function(chain_head, alignment, size);
  } while (!ptr && g_call_new_handler_on_malloc_failure && CallNewHandler());
  return ptr;
}

int ShimPosixMemalign(void** res, size_t alignment, size_t size) {
  // posix_memalign is supposed to check the arguments. See tc_posix_memalign()
  // in tc_malloc.cc.
  if (((alignment % sizeof(void*)) != 0) ||
      ((alignment & (alignment - 1)) != 0) || (alignment == 0)) {
    return EINVAL;
  }
  void* ptr = ShimMemalign(alignment, size);
  *res = ptr;
  return ptr ? 0 : ENOMEM;
}

void* ShimValloc(size_t size) {
  return ShimMemalign(GetPageSize(), size);
}

void* ShimPvalloc(size_t size) {
  // pvalloc(0) should allocate one page, according to its man page.
  if (size == 0) {
    size = GetPageSize();
  } else {
    size = (size + GetPageSize() - 1) & ~(GetPageSize() - 1);
  }
  return ShimMemalign(GetPageSize(), size);
}

void ShimFree(void* address) {
  const allocator::AllocatorDispatch* const chain_head = GetChainHead();
  return chain_head->free_function(chain_head, address);
}

}  // extern "C"

// Cpp symbols (new / delete) should always be routed through the shim layer.
#include "base/allocator/allocator_shim_override_cpp_symbols.h"

// Ditto for plain malloc() / calloc() / free() etc. symbols.
#include "base/allocator/allocator_shim_override_libc_symbols.h"

// In the case of tcmalloc we also want to plumb into the glibc hooks
// to avoid that allocations made in glibc itself (e.g., strdup()) get
// accidentally performed on the glibc heap instead of the tcmalloc one.
#if defined(USE_TCMALLOC)
#include "base/allocator/allocator_shim_override_glibc_weak_symbols.h"
#endif

// Cross-checks.

#if defined(MEMORY_TOOL_REPLACES_ALLOCATOR)
#error The allocator shim should not be compiled when building for memory tools.
#endif

#if (defined(__GNUC__) && defined(__EXCEPTIONS)) || \
    (defined(_HAS_EXCEPTIONS) && _HAS_EXCEPTIONS)
#error This code cannot be used when exceptions are turned on.
#endif
