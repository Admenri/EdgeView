#pragma once

#include "base/bind/callback.h"
#include "base/memory/ref_counted.h"
#include "base/third_party/concurrentqueue/blockingconcurrentqueue.h"
#include "util.h"

namespace edgeview {

class MessagePump : public base::RefCounted<MessagePump> {
 public:
  MessagePump();
  ~MessagePump();

  MessagePump(const MessagePump&) = delete;
  MessagePump& operator=(const MessagePump&) = delete;

  void PostTask(base::OnceClosure task);

 private:
  static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam,
                                  LPARAM lParam);
  HWND message_window;
  UINT task_msgId;
};

}  // namespace edgeview
