#include "ev_extension.h"

#include "edgeview_data.h"

namespace edgeview {

namespace {

void WINAPI Enable(ExtensionData* obj, BOOL enable) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ExtensionData> obj, BOOL enable) {
        obj->core_extension->Enable(enable, nullptr);
      },
      scoped_refptr(obj), enable));
}

LPCSTR WINAPI GetExtensionId(ExtensionData* obj) {
  LPSTR cpp_url = nullptr;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ExtensionData> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        LPWSTR url = nullptr;
        self->core_extension->get_Id(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &cpp_url));
  obj->parent->SyncWaitIfNeed();

  return cpp_url;
}

LPCSTR WINAPI GetName(ExtensionData* obj) {
  LPSTR cpp_url = nullptr;

  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ExtensionData> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        LPWSTR url = nullptr;
        self->core_extension->get_Name(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->parent->semaphore(), &cpp_url));
  obj->parent->SyncWaitIfNeed();

  return cpp_url;
}

void WINAPI Remove(ExtensionData* obj) {
  obj->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ExtensionData> obj) {
        obj->core_extension->Remove(nullptr);
      },
      scoped_refptr(obj)));
}

}  // namespace

DWORD fnExtensionDataTable[] = {
    (DWORD)Enable,
    (DWORD)GetExtensionId,
    (DWORD)GetName,
    (DWORD)Remove,
};

}  // namespace edgeview
