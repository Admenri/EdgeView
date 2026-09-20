#include "ev_msgpump.h"

namespace edgeview {

namespace {

const wchar_t kWndClass[] = L"EdgeView_MessageWindow";
const wchar_t kTaskMessageName[] = L"EdgeView_TaskMsgId";

static bool pump_register = false;

// Same value for every MessagePump instance (RegisterWindowMessage is
// process global), kept here so WndProc can recognize a task message even
// when the pump instance is already gone.
static UINT g_task_msg_id = 0;

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
  g_task_msg_id = task_msgId;

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
  if (message_window) SetUserDataPtr(message_window, this);
}

MessagePump::~MessagePump() {
  if (!message_window) return;

  // Drop every task that was posted but never dispatched. The closure is heap
  // allocated and the window is going away, so those messages would never be
  // handled and the memory would leak.
  MSG msg = {0};
  while (::PeekMessage(&msg, message_window, task_msgId, task_msgId,
                       PM_REMOVE)) {
    delete reinterpret_cast<base::OnceClosure*>(msg.wParam);
  }

  SetUserDataPtr(message_window, nullptr);
  ::DestroyWindow(message_window);
  message_window = nullptr;
}

void MessagePump::PostTask(base::OnceClosure task) {
  if (task.is_null()) return;

  // The pump is unusable (window creation failed or already destroyed):
  // do not queue a task that can never run.
  if (!message_window) return;

  base::OnceClosure* task_ptr = new base::OnceClosure(std::move(task));

  // Post the task for execution by the message window.
  if (!::PostMessage(message_window, task_msgId,
                     reinterpret_cast<WPARAM>(task_ptr), 0)) {
    // Nothing will ever delete the closure - release it right away.
    delete task_ptr;
  }
}

LRESULT MessagePump::WndProc(HWND hWnd, UINT message, WPARAM wParam,
                             LPARAM lParam) {
  MessagePump* self = GetUserDataPtr<MessagePump*>(hWnd);

  if (message == g_task_msg_id) {
    // Execute the task if the pump is still alive; otherwise just release it
    // so the heap allocated closure cannot leak.
    base::OnceClosure* task = reinterpret_cast<base::OnceClosure*>(wParam);
    if (self && task) {
      std::move(*task).Run();
    }
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
