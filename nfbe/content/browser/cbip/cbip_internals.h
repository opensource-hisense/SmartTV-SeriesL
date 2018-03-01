#ifndef CONTENT_BROWSER_CBIP_CBIP_INTERNALS_H_
#define CONTENT_BROWSER_CBIP_CBIP_INTERNALS_H_

#include <map>

#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"

namespace content {

class CbipJs00MessageFilter;
class CbipTaskRunner;

class CbipInternals : public base::RefCounted<CbipInternals> {
 public:
  static CbipInternals* GetInstance();
  CbipInternals();
  CbipTaskRunner* task_runner() const;

  void RegisterJs00MessageFilter(int render_process_id,
                                 CbipJs00MessageFilter* sender);
  void UnregisterJs00MessageFilter(int render_process_id,
                                   CbipJs00MessageFilter* sender);
  CbipJs00MessageFilter* GetJs00MessageFilter(int render_process_id);

 private:
  friend class base::RefCounted<CbipInternals>;
  ~CbipInternals();
  scoped_refptr<content::CbipTaskRunner> task_runner_;
  std::map<int,CbipJs00MessageFilter*> sender_map_;
  DISALLOW_COPY_AND_ASSIGN(CbipInternals);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CBIP_CBIP_TASK_RUNNER_H_
