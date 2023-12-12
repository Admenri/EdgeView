#pragma once

#include "base/memory/ref_counted.h"
#include "edgeview_data.h"
#include "util.h"

namespace edgeview {

struct BrowserData;

class EdgeWidgetHost : public base::RefCounted<EdgeWidgetHost> {
 public:
  EdgeWidgetHost(base::WeakPtr<BrowserData> parent, HWND hParent, RECT rtRect);
  ~EdgeWidgetHost();

  EdgeWidgetHost(const EdgeWidgetHost&) = delete;
  EdgeWidgetHost& operator=(const EdgeWidgetHost&) = delete;

  HWND GetHandle() const { return base_window; }

  void OnSize();
  void OnFocus();

 private:
  void RegisterWidgetClass();
  static LRESULT CALLBACK MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                      LPARAM lParam);

  HWND base_window;
  base::WeakPtr<BrowserData> webview;
};

}  // namespace edgeview
