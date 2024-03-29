// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_EXTENSIONS_ACTIVITY_LOG_ACTIVITY_LOG_H_
#define CHROME_BROWSER_EXTENSIONS_ACTIVITY_LOG_ACTIVITY_LOG_H_

#include <stdint.h>

#include <map>
#include <string>
#include <vector>

#include "base/callback.h"
#include "base/gtest_prod_util.h"
#include "base/macros.h"
#include "base/observer_list_threadsafe.h"
#include "base/scoped_observer.h"
#include "base/synchronization/lock.h"
#include "base/threading/thread.h"
#include "chrome/browser/extensions/activity_log/activity_actions.h"
#include "chrome/browser/extensions/activity_log/activity_log_policy.h"
#include "extensions/browser/api_activity_monitor.h"
#include "extensions/browser/browser_context_keyed_api_factory.h"
#include "extensions/browser/extension_registry_observer.h"
#include "extensions/browser/script_execution_observer.h"
#include "extensions/common/dom_action_types.h"

class Profile;

namespace content {
class BrowserContext;
}

namespace user_prefs {
class PrefRegistrySyncable;
}

namespace extensions {
class Extension;
class ExtensionRegistry;

// A utility for tracing interesting activity for each extension.
// It writes to an ActivityDatabase on a separate thread to record the activity.
// Each profile has different extensions, so we keep a different database for
// each profile.
//
class ActivityLog : public BrowserContextKeyedAPI,
                    public ApiActivityMonitor,
                    public ScriptExecutionObserver,
                    public ExtensionRegistryObserver {
 public:
  // Observers can listen for activity events. There is probably only one
  // observer: the activityLogPrivate API.
  class Observer {
   public:
    virtual void OnExtensionActivity(scoped_refptr<Action> activity) = 0;
  };

  static BrowserContextKeyedAPIFactory<ActivityLog>* GetFactoryInstance();

  // ActivityLog is a KeyedService, so don't instantiate it with
  // the constructor; use GetInstance instead.
  static ActivityLog* GetInstance(content::BrowserContext* context);

  // Add/remove observer: the activityLogPrivate API only listens when the
  // ActivityLog extension is registered for an event.
  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  // Logs an extension action: passes it to any installed policy to be logged
  // to the database, to any observers, and logs to the console if in testing
  // mode.
  void LogAction(scoped_refptr<Action> action);

  // Gets all actions that match the specified fields. URLs are treated like
  // prefixes; other fields are exact matches. Empty strings are not matched to
  // anything. For daysAgo, today is 0, yesterday is 1, etc.; a negative number
  // of days is treated as a missing parameter.
  void GetFilteredActions(
      const std::string& extension_id,
      const Action::ActionType type,
      const std::string& api_name,
      const std::string& page_url,
      const std::string& arg_url,
      const int days_ago,
      const base::Callback<
          void(std::unique_ptr<std::vector<scoped_refptr<Action>>>)>& callback);

  // ExtensionRegistryObserver.
  // We keep track of whether the whitelisted extension is installed; if it is,
  // we want to recompute whether to have logging enabled.
  void OnExtensionLoaded(content::BrowserContext* browser_context,
                         const Extension* extension) override;
  void OnExtensionUnloaded(content::BrowserContext* browser_context,
                           const Extension* extension,
                           UnloadedExtensionInfo::Reason reason) override;
  void OnExtensionUninstalled(content::BrowserContext* browser_context,
                              const Extension* extension,
                              extensions::UninstallReason reason) override;

  // ApiActivityMonitor.
  void OnApiEventDispatched(
      const std::string& extension_id,
      const std::string& event_name,
      std::unique_ptr<base::ListValue> event_args) override;
  void OnApiFunctionCalled(
      const std::string& extension_id,
      const std::string& api_name,
      std::unique_ptr<base::ListValue> event_args) override;

  static void RegisterProfilePrefs(user_prefs::PrefRegistrySyncable* registry);

  // Remove actions from the activity log database which IDs specified in the
  // action_ids array.
  void RemoveActions(const std::vector<int64_t>& action_ids);

  // Clean up URLs from the activity log database.
  // If restrict_urls is empty then all URLs in the activity log database are
  // removed, otherwise only those in restrict_urls are removed.
  void RemoveURLs(const std::vector<GURL>& restrict_urls);
  void RemoveURLs(const std::set<GURL>& restrict_urls);
  void RemoveURL(const GURL& url);

  // Deletes the database associated with the policy that's currently in use.
  void DeleteDatabase();

  // If we're in a browser test, we need to pretend that the watchdog app is
  // active.
  void SetWatchdogAppActiveForTesting(bool active);

 private:
  friend class ActivityLogTest;
  friend class BrowserContextKeyedAPIFactory<ActivityLog>;

  explicit ActivityLog(content::BrowserContext* context);
  ~ActivityLog() override;

  // Specifies if the Watchdog app is active (installed & enabled).
  // If so, we need to log to the database and stream to the API.
  bool IsWatchdogAppActive();

  // Specifies if we need to record actions to the db. If so, we need to log to
  // the database. This is true if the Watchdog app is active *or* the
  // --enable-extension-activity-logging flag is set.
  bool IsDatabaseEnabled();

  // ScriptExecutionObserver implementation.
  // Fires when a ContentScript is executed.
  void OnScriptsExecuted(const content::WebContents* web_contents,
                         const ExecutingScriptsMap& extension_ids,
                         const GURL& on_url) override;

  // At the moment, ActivityLog will use only one policy for summarization.
  // These methods are used to choose and set the most appropriate policy.
  // Changing policies at runtime is not recommended, and likely only should be
  // done for unit tests.
  void ChooseDatabasePolicy();
  void SetDatabasePolicy(ActivityLogPolicy::PolicyType policy_type);

  // BrowserContextKeyedAPI implementation.
  static const char* service_name() { return "ActivityLog"; }
  static const bool kServiceRedirectedInIncognito = true;
  static const bool kServiceIsCreatedWithBrowserContext = false;

  typedef base::ObserverListThreadSafe<Observer> ObserverList;
  scoped_refptr<ObserverList> observers_;

  // Policy objects are owned by the ActivityLog, but cannot be scoped_ptrs
  // since they may need to do some cleanup work on the database thread.
  // Calling policy->Close() will free the object; see the comments on the
  // ActivityDatabase class for full details.

  // The database policy object takes care of recording & looking up data:
  // data summarization, compression, and logging. There should only be a
  // database_policy_ if the Watchdog app is installed or flag is set.
  ActivityLogDatabasePolicy* database_policy_;
  ActivityLogPolicy::PolicyType database_policy_type_;

  Profile* profile_;
  bool db_enabled_;  // Whether logging to disk is currently enabled.
  // testing_mode_ controls which policy is selected.
  // * By default, we choose a policy that doesn't log most arguments to avoid
  // saving too much data. We also elide some arguments for privacy reasons.
  // * In testing mode, we choose a policy that logs all arguments.
  // testing_mode_ also causes us to print to the console.
  bool testing_mode_;
  // We need the DB, FILE, and IO threads to write to the database.
  // In some cases (tests), these threads might not exist, so we avoid
  // dispatching anything to the policies/database to prevent things from
  // exploding.
  bool has_threads_;

  // Used to track whether the whitelisted extension is installed. If it's
  // added or removed, enabled_ may change.
  ScopedObserver<extensions::ExtensionRegistry,
                 extensions::ExtensionRegistryObserver>
      extension_registry_observer_;

  // Set if the watchdog app is installed and enabled. Maintained by
  // kWatchdogExtensionActive pref variable. Since there are multiple valid
  // extension IDs, this needs to be an int to count how many are installed.
  int watchdog_apps_active_;

  FRIEND_TEST_ALL_PREFIXES(ActivityLogApiTest, TriggerEvent);
  FRIEND_TEST_ALL_PREFIXES(ActivityLogEnabledTest, AppAndCommandLine);
  FRIEND_TEST_ALL_PREFIXES(ActivityLogEnabledTest, CommandLineSwitch);
  FRIEND_TEST_ALL_PREFIXES(ActivityLogEnabledTest, NoSwitch);
  FRIEND_TEST_ALL_PREFIXES(ActivityLogEnabledTest, PrefSwitch);
  FRIEND_TEST_ALL_PREFIXES(ActivityLogEnabledTest, WatchdogSwitch);
  DISALLOW_COPY_AND_ASSIGN(ActivityLog);
};

template <>
void BrowserContextKeyedAPIFactory<ActivityLog>::DeclareFactoryDependencies();

}  // namespace extensions

#endif  // CHROME_BROWSER_EXTENSIONS_ACTIVITY_LOG_ACTIVITY_LOG_H_
