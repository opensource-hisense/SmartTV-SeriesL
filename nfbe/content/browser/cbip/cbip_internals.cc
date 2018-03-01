#include "content/browser/cbip/cbip_internals.h"

#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/memory/ref_counted.h"
#include "content/browser/cbip/cbip_task_runner.h"
#include "content/common/content_export.h"

#define X_LOG_V 0

namespace {

static base::LazyInstance<content::CbipInternals>::Leaky g_cbip_internals =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

namespace content {

CbipInternals* CbipInternals::GetInstance() {
  return g_cbip_internals.Pointer();
}

CbipInternals::CbipInternals()
    : task_runner_(new content::CbipTaskRunner()) {
}

CbipInternals::~CbipInternals() {
  // FIXME: this will never be called because the above leaky g_cbip_internals
  // will never be destructed - causing memory leak.
  {
    VLOG(X_LOG_V) << "ZZZZ: CbipInternals::dtor:"
                  << " this=" << static_cast<void*>(this)
      ;
  }
}

CbipTaskRunner* CbipInternals::task_runner() const {
  return task_runner_.get();
}


void CbipInternals::RegisterJs00MessageFilter(
    int render_process_id,
    CbipJs00MessageFilter* sender) {
  sender_map_[render_process_id] = sender;
}

void CbipInternals::UnregisterJs00MessageFilter(
    int render_process_id,
    CbipJs00MessageFilter* sender) {
  std::map<int,CbipJs00MessageFilter*>::iterator it =
      sender_map_.find(render_process_id);
  DCHECK(it != sender_map_.end());
  if (it != sender_map_.end())
    sender_map_.erase(it);
}

CbipJs00MessageFilter* CbipInternals::GetJs00MessageFilter(
    int render_process_id) {
  std::map<int,CbipJs00MessageFilter*>::iterator it =
      sender_map_.find(render_process_id);
  if (it != sender_map_.end())
    return it->second;
  return 0;
}

}  // namespace content
