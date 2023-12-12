#include "ev_msgpump.h"

namespace edgeview {

namespace {

const wchar_t kWndClass[] = L"EdgeView_MessageWindow";
const wchar_t kTaskMessageName[] = L"EdgeView_TaskMsgId";

static bool pump_register = false;

void SetUserDataPtr(HWND hWnd, void* ptr) {
  SetLastError(ERROR_SUCCESS);
  ::SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(ptr));
}

// Return the window's user data pointer.
template <typename T>
T GetUserDataPtr(HWND hWnd) {
  return reinterpret_cast<T>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
}

}  // namespace

MessagePump::MessagePump()
    : task_msgId(RegisterWindowMessage(kTaskMessageName)) {
  if (!pump_register) {
    WNDCLASSEX wc = {0};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = WndProc;
    wc.hInstance = GetModuleHandle(0);
    wc.lpszClassName = kWndClass;
    RegisterClassEx(&wc);
  }
  pump_register = true;

  message_window = CreateWindow(kWndClass, 0, 0, 0, 0, 0, 0, HWND_MESSAGE, 0,
                                GetModuleHandle(0), 0);
  SetUserDataPtr(message_window, this);
}

MessagePump::~MessagePump() { DestroyWindow(message_window); }

void MessagePump::PostTask(base::OnceClosure task) {
  if (task.is_null()) return;
  base::OnceClosure* task_ptr = new base::OnceClosure(std::move(task));

  // Post the task for execution by the message window.
  PostMessage(message_window, task_msgId, reinterpret_cast<WPARAM>(task_ptr),
              0);
}

LRESULT MessagePump::WndProc(HWND hWnd, UINT message, WPARAM wParam,
                             LPARAM lParam) {
  MessagePump* self = GetUserDataPtr<MessagePump*>(hWnd);

  if (self && message == self->task_msgId) {
    // Execute the task.
    base::OnceClosure* task = reinterpret_cast<base::OnceClosure*>(wParam);
    std::move(*task).Run();
    delete task;
  } else {
    switch (message) {
      case WM_NCDESTROY:
        // Clear the reference to |self|.
        SetUserDataPtr(hWnd, nullptr);
        break;
    }
  }

  return DefWindowProc(hWnd, message, wParam, lParam);
}

}  // namespace edgeview
