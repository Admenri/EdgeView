#include "webview_host.h"

#include <tuple>

#include "edgeview_data.h"

static bool class_registered = false;

static const wchar_t kEdgeViewWidgetClassName[] = L"EdgeView_WidgetWin_0";

static void SetUserDataPtr(HWND hWnd, void* ptr) {
  SetLastError(ERROR_SUCCESS);
  std::ignore =
      ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr));
}

template <typename T>
static T GetUserDataPtr(HWND hWnd) {
  return reinterpret_cast<T>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

namespace edgeview {

EdgeWidgetHost::EdgeWidgetHost(base::WeakPtr<BrowserData> parent, HWND hParent,
                               RECT rtRect)
    : webview(parent) {
  RegisterWidgetClass();

  const DWORD dwStyle = hParent ? WS_CHILD | WS_VISIBLE : WS_POPUP | WS_VISIBLE;
  base_window = CreateWindowExW(WS_EX_TRANSPARENT, kEdgeViewWidgetClassName,
                                kEdgeViewWidgetClassName, dwStyle, rtRect.left,
                                rtRect.top, rtRect.right, rtRect.bottom,
                                hParent, nullptr, GetModuleHandle(0), this);
}

EdgeWidgetHost::~EdgeWidgetHost() {
  SetUserDataPtr(base_window, nullptr);
  DestroyWindow(base_window);
}

void EdgeWidgetHost::OnSize() {
  if (webview->core_controller) {
    RECT rt;
    ::GetClientRect(base_window, &rt);
    webview->core_controller->put_Bounds(rt);
  }
}

void EdgeWidgetHost::OnFocus() {
  if (webview->core_controller) {
    HWND chrome_widget =
        ::GetWindow(::GetWindow(base_window, GW_CHILD), GW_CHILD);
    ::SetFocus(chrome_widget);
  }
}

void EdgeWidgetHost::RegisterWidgetClass() {
  // Only register the class one time.
  if (class_registered) return;
  class_registered = true;

  WNDCLASSEX wcex = {0};

  wcex.cbSize = sizeof(WNDCLASSEX);
  wcex.lpszClassName = kEdgeViewWidgetClassName;
  wcex.lpfnWndProc = MainWndProc;
  wcex.hInstance = GetModuleHandle(0);
  wcex.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

  RegisterClassEx(&wcex);
}

LRESULT EdgeWidgetHost::MainWndProc(HWND hWnd, UINT message, WPARAM wParam,
                                    LPARAM lParam) {
  EdgeWidgetHost* self = nullptr;
  self = GetUserDataPtr<EdgeWidgetHost*>(hWnd);

  // Callback for the main window
  switch (message) {
    case WM_NCCREATE: {
      SetLayeredWindowAttributes(hWnd, 0, 0, LWA_ALPHA);
      CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lParam);
      self = reinterpret_cast<EdgeWidgetHost*>(cs->lpCreateParams);
      SetUserDataPtr(hWnd, self);
      break;
    }
    case WM_SIZE: {
      if (self) self->OnSize();
      break;
    }
    case WM_SETFOCUS: {
      if (self) self->OnFocus();
      break;
    }
    default:
      break;
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

}  // namespace edgeview
