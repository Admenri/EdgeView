#pragma once

#include "edgeview_data.h"
#include "event_notify.h"
#include "util.h"

namespace edgeview {
extern DWORD fnEnvironmentTable[];
}  // namespace edgeview

using EnvCreateParams = struct {
  LPCSTR pszBrowserPath;
  LPCSTR pszUserDataPath;
  LPCSTR pszCommandLine;
  LPCSTR pszLanguage;
  BOOL bEnableBrowserExtensions;
};

EV_EXPORTS(CreateEnvironment, BOOL)(EnvCreateParams* params, DWORD* retObj);
