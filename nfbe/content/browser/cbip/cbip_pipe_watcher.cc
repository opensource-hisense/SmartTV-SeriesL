#include "content/browser/cbip/cbip_pipe_watcher.h"

#include "base/logging.h"
#include "base/strings/nullable_string16.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/cbip/cbip_internals.h"
#include "content/browser/cbip/cbip_js00_message_filter.h"
#include "content/public/browser/browser_thread.h"

// FIXME: non-linux version.
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define X_LOG_V 0

namespace content {

CbipPipeWatcher::CbipPipeWatcher(
    CbipInternals* internals)
    : internals_(internals),
      pipe_fd_r_(-1),
      pipe_fd_w_(-1),
      retry_count_(0) {
  BrowserThread::PostTask(
      BrowserThread::FILE, FROM_HERE,
      base::Bind(&CbipPipeWatcher::init, base::Unretained(this)));
}

CbipPipeWatcher::~CbipPipeWatcher() {
}

void CbipPipeWatcher::init() {
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPipeWatcher::init:"
                  << " retry_count_=" << retry_count_
      ;
  }
  if (pipe_fd_r_ < 0) {
    pipe_fd_r_ = open("/tmp/br.pipe", O_RDONLY|O_NONBLOCK);
    if (pipe_fd_r_ < 0) {
      if (10 < retry_count_)
        return;
      retry_count_ += 1;
      if (EINTR == errno) {
        BrowserThread::PostTask(
            BrowserThread::FILE, FROM_HERE,
            base::Bind(&CbipPipeWatcher::init, base::Unretained(this)));
      }
      return;
    }
  }

  // kludge to avoid hup.
  if (pipe_fd_w_ < 0) {
    pipe_fd_w_ = open("/tmp/br.pipe", O_WRONLY);
    if (pipe_fd_w_ < 0) {
      if (10 < retry_count_)
        return;
      retry_count_ += 1;
      if (EINTR == errno) {
        BrowserThread::PostTask(
            BrowserThread::FILE, FROM_HERE,
            base::Bind(&CbipPipeWatcher::init, base::Unretained(this)));
      }
      return;
    }
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::Bind(&CbipPipeWatcher::start, base::Unretained(this), pipe_fd_r_));
}

void CbipPipeWatcher::start(int fd) {
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPipeWatcher::start:"
                  << " fd=" << fd
      ;
  }

  base::MessageLoopForIO* message_loop = base::MessageLoopForIO::current();

  int result =
      message_loop->WatchFileDescriptor(fd,
                                        true,
                                        base::MessageLoopForIO::WATCH_READ,
                                        &controller_,
                                        this);
  if (!result) {
    (void)0;
  }
}

void CbipPipeWatcher::OnFileCanReadWithoutBlocking(int fd) {
  {
    VLOG(X_LOG_V) << "ZZZZ CbipPipeWatcher::OnFileCanReadWithoutBlocking:"
                  << " fd=" << fd
      ;
  }
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::IO));

  /* FIXME: _POSIX_PIPE_BUF */
  char buf[512+1];

  ssize_t nread = read(fd, buf, 512);
  {
    VLOG(X_LOG_V) << "ZZZZ nread=" << nread;
  }
  if (nread <= 0) {
    return;
  }
  buf[nread] = '\0';

  const char* bp = buf;
  const char* bp_endptr = buf + nread;
  char* endptr;

  errno = 0;
  long int v = strtol(bp, &endptr, 10);
  if (errno) {
    /* error */
    VLOG(X_LOG_V) << "ZZZZ errno=" << errno;
    return;
  }
  int render_process_id = v;
  bp = endptr;
  if (bp_endptr <= bp) {
    /* error */
    VLOG(X_LOG_V) << "ZZZZ not terminated by ':' " << __LINE__;
    return;
  }
  if (':' != *bp) {
    /* error */
    VLOG(X_LOG_V) << "ZZZZ not terminated by ':' " << __LINE__;
    return;
  }
  bp += 1;
  if (bp_endptr <= bp) {
    /* error */
    VLOG(X_LOG_V) << "ZZZZ no message " << __LINE__;
    return;
  }

  CbipJs00MessageFilter* sender =
      CbipInternals::GetInstance()->GetJs00MessageFilter(
          render_process_id);

  {
    VLOG(X_LOG_V) << "ZZZZ sender=" << sender
                  << ":" << render_process_id
    ;
  }

  if (sender) {
    base::NullableString16 i0 = base::NullableString16(
        base::ASCIIToUTF16(bp), false);
    sender->SendCbipJs00AsyncEvent1(i0);
  }
}

void CbipPipeWatcher::OnFileCanWriteWithoutBlocking(int fd) {
  NOTREACHED();
}

}  // namespace content
