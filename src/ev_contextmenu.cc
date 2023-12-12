#include "ev_contextmenu.h"

#include "edgeview_data.h"

namespace edgeview {

void WINAPI ProcessMenu(ContextMenuParams* obj, BOOL handled) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuParams> obj, BOOL handled) {
        obj->core_menu->put_Handled(handled);
        obj->internal_deferral->Complete();
      },
      scoped_refptr(obj), handled));
}

using ContextMenuTargetInfoData = struct {
  COREWEBVIEW2_CONTEXT_MENU_TARGET_KIND type;
  BOOL editable;
  BOOL main_frame;
  LPCSTR page_url;
  LPCSTR frame_url;
  BOOL has_link_url;
  LPCSTR link_url;
  BOOL has_link_text;
  LPCSTR link_text;
  BOOL has_source_url;
  LPCSTR source_url;
  BOOL has_selection;
  LPCSTR selection;
};

void WINAPI GetTargetParams(ContextMenuParams* obj,
                            ContextMenuTargetInfoData* data) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuParams> obj, scoped_refptr<Semaphore> sync,
         ContextMenuTargetInfoData* data) {
        WRL::ComPtr<ICoreWebView2ContextMenuTarget> target = nullptr;
        obj->core_menu->get_ContextMenuTarget(&target);

        target->get_Kind(&data->type);
        target->get_IsEditable(&data->editable);
        target->get_IsRequestedForMainFrame(&data->main_frame);

        LPWSTR page_url = nullptr;
        target->get_PageUri(&page_url);
        data->page_url = WrapComString(page_url);

        LPWSTR frame_url = nullptr;
        target->get_FrameUri(&frame_url);
        data->frame_url = WrapComString(frame_url);

        target->get_HasLinkUri(&data->has_link_url);
        LPWSTR link_url = nullptr;
        target->get_LinkUri(&link_url);
        data->link_url = WrapComString(link_url);

        target->get_HasLinkText(&data->has_link_text);
        LPWSTR link_text = nullptr;
        target->get_LinkText(&link_text);
        data->link_text = WrapComString(link_text);

        target->get_HasSourceUri(&data->has_source_url);
        LPWSTR source_url = nullptr;
        target->get_SourceUri(&source_url);
        data->source_url = WrapComString(source_url);

        target->get_HasSourceUri(&data->has_selection);
        LPWSTR selection = nullptr;
        target->get_SelectionText(&selection);
        data->selection = WrapComString(selection);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), data));
  obj->browser->parent->SyncWaitIfNeed();
}

void WINAPI GetPoint(ContextMenuParams* obj, POINT* pt) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuParams> obj, scoped_refptr<Semaphore> sync,
         POINT* pt) {
        obj->core_menu->get_Location(pt);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), pt));
  obj->browser->parent->SyncWaitIfNeed();
}

void WINAPI SetCommandID(ContextMenuParams* obj, int command_id) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuParams> obj, int command_id) {
        obj->core_menu->put_SelectedCommandId(command_id);
      },
      scoped_refptr(obj), command_id));
}

void WINAPI GetMenuModel(ContextMenuParams* obj, DWORD* retObj) {
  scoped_refptr<ContextMenuCollection> collection = new ContextMenuCollection();
  collection->browser = obj->browser;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuParams> obj, scoped_refptr<Semaphore> sync,
         scoped_refptr<ContextMenuCollection> collection) {
        obj->core_menu->get_MenuItems(&collection->core_list);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), collection));
  obj->browser->parent->SyncWaitIfNeed();

  if (retObj) {
    collection->AddRef();
    retObj[1] = (DWORD)collection.get();
    retObj[2] = (DWORD)fnContextMenuModelTable;
  }
}

void WINAPI CreateMenuItem(ContextMenuParams* obj, LPCSTR label,
                           LPBYTE icon_data, int32_t icon_size,
                           COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND type,
                           DWORD* retObj) {
  scoped_refptr<ContextMenuItem> item = new ContextMenuItem();

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuParams> obj, scoped_refptr<Semaphore> sync,
         LPCSTR label, LPBYTE icon_data, int32_t icon_size,
         COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND type,
         scoped_refptr<ContextMenuItem> item) {
        WRL::ComPtr<IStream> is = nullptr;
        if (icon_size > 0) {
          HGLOBAL hMem = GlobalAlloc(GMEM_ZEROINIT, icon_size);
          LPVOID pMem = nullptr;
          if (!hMem) {
            is.Reset();
            goto CreateItem;
          }

          pMem = GlobalLock(hMem);
          if (!pMem) {
            is.Reset();
            goto CreateItem;
          }

          RtlCopyMemory(pMem, icon_data, icon_size);
          GlobalUnlock(hMem);
          CreateStreamOnHGlobal(hMem, TRUE, &is);
        }

      CreateItem:
        item->browser = obj->browser;
        obj->browser->parent->core_env->CreateContextMenuItem(
            Utf8Conv::Utf8ToUtf16(label).c_str(), is.Get(), type,
            &item->core_item);

        base::WeakPtr<BrowserData> weak_ptr = obj->browser;
        item->core_item->add_CustomItemSelected(
            WRL::Callback<ICoreWebView2CustomItemSelectedEventHandler>(
                [weak_ptr](ICoreWebView2ContextMenuItem* sender,
                           IUnknown* args) {
                  scoped_refptr<ContextMenuItem> item = new ContextMenuItem();
                  item->browser = weak_ptr;
                  item->core_item = sender;

                  weak_ptr->parent->PostEvent(base::BindOnce(
                      [](base::WeakPtr<BrowserData> weak_ptr,
                         scoped_refptr<ContextMenuItem> item) {
                        weak_ptr->dispatcher->OnContextMenuExecute(item);
                      },
                      weak_ptr, std::move(item)));

                  return S_OK;
                })
                .Get(),
            nullptr);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), label, icon_data,
      icon_size, type, item));
  obj->browser->parent->SyncWaitIfNeed();

  if (retObj) {
    item->AddRef();
    retObj[1] = (DWORD)item.get();
    retObj[2] = (DWORD)fnContextMenuItemTable;
  }
}

DWORD fnContextMenuParamsTable[] = {
    (DWORD)ProcessMenu,  (DWORD)GetTargetParams, (DWORD)GetPoint,
    (DWORD)SetCommandID, (DWORD)GetMenuModel,    (DWORD)CreateMenuItem,
};

uint32_t WINAPI GetCollectionSize(ContextMenuCollection* obj) {
  uint32_t size = 0;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuCollection> obj,
         scoped_refptr<Semaphore> sync, uint32_t* size) {
        obj->core_list->get_Count(size);
        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &size));
  obj->browser->parent->SyncWaitIfNeed();

  return size;
}

void WINAPI GetValueAt(ContextMenuCollection* obj, uint32_t index,
                       DWORD* retObj) {
  scoped_refptr<ContextMenuItem> async_obj = new ContextMenuItem();

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuCollection> obj,
         scoped_refptr<Semaphore> sync, uint32_t index,
         scoped_refptr<ContextMenuItem> async_obj) {
        async_obj->browser = obj->browser;
        obj->core_list->GetValueAtIndex(index, &async_obj->core_item);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), index, async_obj));
  obj->browser->parent->SyncWaitIfNeed();

  if (retObj) {
    async_obj->AddRef();
    retObj[1] = (DWORD)async_obj.get();
    retObj[2] = (DWORD)fnContextMenuItemTable;
  }
}

void WINAPI RemoveValueAt(ContextMenuCollection* obj, uint32_t index) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuCollection> obj, uint32_t index) {
        obj->core_list->RemoveValueAtIndex(index);
      },
      scoped_refptr(obj), index));
}

void WINAPI InsertValueAt(ContextMenuCollection* obj, uint32_t index,
                          ContextMenuItem* item) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuCollection> obj, uint32_t index,
         scoped_refptr<ContextMenuItem> item) {
        obj->core_list->InsertValueAtIndex(index, item->core_item.Get());
      },
      scoped_refptr(obj), index, scoped_refptr(item)));
}

DWORD fnContextMenuModelTable[] = {
    (DWORD)GetCollectionSize,
    (DWORD)GetValueAt,
    (DWORD)RemoveValueAt,
    (DWORD)InsertValueAt,
};

LPCSTR WINAPI GetName(ContextMenuItem* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuItem> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        LPWSTR url = nullptr;
        self->core_item->get_Name(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

LPCSTR WINAPI GetLabel(ContextMenuItem* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuItem> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        LPWSTR url = nullptr;
        self->core_item->get_Label(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

int WINAPI GetCommandID(ContextMenuItem* obj) {
  int cpp_url = -1;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuItem> self, scoped_refptr<Semaphore> sync,
         int* cpp_url) {
        self->core_item->get_CommandId(cpp_url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

LPCSTR WINAPI GetShortcutDesc(ContextMenuItem* obj) {
  LPSTR cpp_url = nullptr;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuItem> self, scoped_refptr<Semaphore> sync,
         LPSTR* cpp_url) {
        LPWSTR url = nullptr;
        self->core_item->get_ShortcutKeyDescription(&url);
        *cpp_url = WrapComString(url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

void WINAPI GetIcon(ContextMenuItem* obj, LPVOID* icon_data,
                    int32_t* icon_size) {
  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuItem> self, scoped_refptr<Semaphore> sync,
         LPVOID* icon_data, int32_t* icon_size) {
        WRL::ComPtr<IStream> is = nullptr;
        auto ret = SUCCEEDED(self->core_item->get_Icon(&is));

        if (!is) return sync->Notify();

        STATSTG stat;
        is->Stat(&stat, STATFLAG_NONAME);

        LARGE_INTEGER linfo;
        linfo.QuadPart = 0;
        is->Seek(linfo, STREAM_SEEK_SET, NULL);

        uint32_t size = stat.cbSize.LowPart;
        uint8_t* buf = static_cast<uint8_t*>(edgeview_MemAlloc(size));
        ULONG dummy = 0;
        is->Read(buf, size, &dummy);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), icon_data,
      icon_size));
  obj->browser->parent->SyncWaitIfNeed();
}

COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND WINAPI GetMenuType(ContextMenuItem* obj) {
  COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND cpp_url;

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuItem> self, scoped_refptr<Semaphore> sync,
         COREWEBVIEW2_CONTEXT_MENU_ITEM_KIND* cpp_url) {
        self->core_item->get_Kind(cpp_url);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), &cpp_url));
  obj->browser->parent->SyncWaitIfNeed();

  return cpp_url;
}

void WINAPI GetChildrenMenu(ContextMenuItem* obj, DWORD* retObj) {
  scoped_refptr<ContextMenuCollection> collection = new ContextMenuCollection();

  obj->browser->parent->PostUITask(base::BindOnce(
      [](scoped_refptr<ContextMenuItem> obj, scoped_refptr<Semaphore> sync,
         scoped_refptr<ContextMenuCollection> collection) {
        collection->browser = obj->browser;
        obj->core_item->get_Children(&collection->core_list);

        sync->Notify();
      },
      scoped_refptr(obj), obj->browser->parent->semaphore(), collection));
  obj->browser->parent->SyncWaitIfNeed();

  if (retObj) {
    collection->AddRef();
    retObj[1] = (DWORD)collection.get();
    retObj[2] = (DWORD)fnContextMenuModelTable;
  }
}

void WINAPI PutEnabled(ContextMenuItem* obj, BOOL enable) {
  obj->browser->parent->PostUITask(
      base::BindOnce([](scoped_refptr<ContextMenuItem> obj,
                        BOOL enable) { obj->core_item->put_IsEnabled(enable); },
                     scoped_refptr(obj), enable));
}

void WINAPI PutChecked(ContextMenuItem* obj, BOOL enable) {
  obj->browser->parent->PostUITask(
      base::BindOnce([](scoped_refptr<ContextMenuItem> obj,
                        BOOL enable) { obj->core_item->put_IsChecked(enable); },
                     scoped_refptr(obj), enable));
}

DWORD fnContextMenuItemTable[] = {
    (DWORD)GetName,         (DWORD)GetLabel,   (DWORD)GetCommandID,
    (DWORD)GetShortcutDesc, (DWORD)GetIcon,    (DWORD)GetMenuType,
    (DWORD)GetChildrenMenu, (DWORD)PutEnabled, (DWORD)PutChecked,
};

}  // namespace edgeview
