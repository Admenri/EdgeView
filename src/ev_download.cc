#include "ev_download.h"

#include "edgeview_data.h"

namespace edgeview {

namespace {

void WINAPI Confirm_GetOperation(DownloadConfirm* obj, DWORD* retObj) {
  scoped_refptr<DownloadOperation> ckm = new DownloadOperation();

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadConfirm> obj, scoped_refptr<Semaphore> sync,
         scoped_refptr<DownloadOperation> ckm) {
        ckm->browser = obj->browser;
        obj->core_args->get_DownloadOperation(&ckm->core_operation);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), ckm));
  obj->browser->parent->SyncWaitIfNeed();

  if (retObj) {
    ckm->AddRef();
    retObj[1] = (DWORD)ckm.get();
    retObj[2] = (DWORD)fnDownloadOperationTable;
  }
}

void WINAPI Confirm_SetCancel(DownloadConfirm* obj, BOOL cancel) {
  obj->browser->parent->PostUITask(
      base::BindOnce([](scoped_refptr<DownloadConfirm> obj,
                        BOOL cancel) { obj->core_args->put_Cancel(cancel); },
                     scoped_refptr(obj), cancel));
}

void WINAPI Confirm_SetFilePath(DownloadConfirm* obj, LPCSTR path) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadConfirm> obj, std::string path) {
        obj->core_args->put_ResultFilePath(Utf8Conv::Utf8ToUtf16(path).c_str());
      },
      scoped_refptr(obj), std::string(path)));
}

void WINAPI Confirm_HandleRequest(DownloadConfirm* obj, BOOL handled) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadConfirm> obj, BOOL handled) {
        obj->core_args->put_Handled(handled);
        obj->internal_deferral->Complete();
      },
      scoped_refptr(obj), handled));
}

}  // namespace

DWORD fnDownloadConfirmTable[] = {
    (DWORD)Confirm_GetOperation,
    (DWORD)Confirm_SetCancel,
    (DWORD)Confirm_SetFilePath,
    (DWORD)Confirm_HandleRequest,
};

namespace {

void WINAPI DO_Cancel(DownloadOperation* obj) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> obj) {
        obj->core_operation->Cancel();
      },
      scoped_refptr(obj)));
}

void WINAPI DO_Pause(DownloadOperation* obj) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> obj) {
        obj->core_operation->Pause();
      },
      scoped_refptr(obj)));
}

void WINAPI DO_Resume(DownloadOperation* obj) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> obj) {
        obj->core_operation->Resume();
      },
      scoped_refptr(obj)));
}

BOOL WINAPI DO_GetCanResume(DownloadOperation* obj) {
  BOOL value = FALSE;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         BOOL* value) {
        self->core_operation->get_CanResume(value);
        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &value));
  obj->browser->parent->SyncWaitIfNeed();

  return value;
}

COREWEBVIEW2_DOWNLOAD_STATE WINAPI DO_GetState(DownloadOperation* obj) {
  COREWEBVIEW2_DOWNLOAD_STATE value;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         COREWEBVIEW2_DOWNLOAD_STATE* value) {
        self->core_operation->get_State(value);
        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &value));
  obj->browser->parent->SyncWaitIfNeed();

  return value;
}

LPCSTR WINAPI DO_GetURL(DownloadOperation* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_operation->get_Uri(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

LPCSTR WINAPI DO_GetResultFilePath(DownloadOperation* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_operation->get_ResultFilePath(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

LPCSTR WINAPI DO_GetMimeType(DownloadOperation* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_operation->get_MimeType(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

LPCSTR WINAPI DO_GetDisposition(DownloadOperation* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_operation->get_ContentDisposition(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

void WINAPI DO_GetTotalBytes(DownloadOperation* obj, int64_t* value) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         int64_t* value) {
        self->core_operation->get_TotalBytesToReceive(value);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), value));
  obj->browser->parent->SyncWaitIfNeed();
}

void WINAPI DO_GetReceivedBytes(DownloadOperation* obj, int64_t* value) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         int64_t* value) {
        self->core_operation->get_BytesReceived(value);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), value));
  obj->browser->parent->SyncWaitIfNeed();
}

LPCSTR WINAPI DO_GetEstimatedEndTime(DownloadOperation* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<DownloadOperation> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        wil::unique_cotaskmem_string url = nullptr;
        self->core_operation->get_EstimatedEndTime(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

}  // namespace

DWORD fnDownloadOperationTable[] = {
    (DWORD)DO_Cancel,
    (DWORD)DO_Pause,
    (DWORD)DO_Resume,
    (DWORD)DO_GetCanResume,
    (DWORD)DO_GetState,
    (DWORD)DO_GetURL,
    (DWORD)DO_GetResultFilePath,
    (DWORD)DO_GetMimeType,
    (DWORD)DO_GetDisposition,
    (DWORD)DO_GetTotalBytes,
    (DWORD)DO_GetReceivedBytes,
    (DWORD)DO_GetEstimatedEndTime,
};

}  // namespace edgeview
