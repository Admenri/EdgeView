
#include "struct_class.h"

namespace eClass {

DWORD m_pVfTable_Environment;
DWORD m_pVfTable_Browser;
DWORD m_pVfTable_NewWindowDelegate;
DWORD m_pVfTable_ScriptDialogDelegate;
DWORD m_pVfTable_ContextMenuParams;
DWORD m_pVfTable_ContextMenuCollection;
DWORD m_pVfTable_ContextMenuItem;
DWORD m_pVfTable_PermissionDelegate;
DWORD m_pVfTable_CookieManager;
DWORD m_pVfTable_ResourceRequestCallback;
DWORD m_pVfTable_ResourceResponseCallback;
DWORD m_pVfTable_BasicAuthenticationCallback;
DWORD m_pVfTable_DownloadOperation;
DWORD m_pVfTable_DownloadConfirm;
DWORD m_pVfTable_Frame;
DWORD m_pVfTable_DOM;
DWORD m_pVfTable_BrowserExtension;

}  // namespace eClass

EV_EXPORTS(RegisterClass, void)(DWORD **pNewClass, EClassVTable nType) {
  DWORD dwVfptr = **pNewClass;
  if (dwVfptr != NULL) {
    switch (nType) {
      case EClassVTable::VT_ENVIRONMENT:
        eClass::m_pVfTable_Environment = dwVfptr;
        break;
      case EClassVTable::VT_BROWSER:
        eClass::m_pVfTable_Browser = dwVfptr;
        break;
      case EClassVTable::VT_NEWWINDOWDELEGATE:
        eClass::m_pVfTable_NewWindowDelegate = dwVfptr;
        break;
      case EClassVTable::VT_SCRIPTDIALOGDELEGATE:
        eClass::m_pVfTable_ScriptDialogDelegate = dwVfptr;
        break;
      case EClassVTable::VT_CONTEXTMENUPARAMS:
        eClass::m_pVfTable_ContextMenuParams = dwVfptr;
        break;
      case EClassVTable::VT_CONTEXTMENUCOLLECTION:
        eClass::m_pVfTable_ContextMenuCollection = dwVfptr;
        break;
      case EClassVTable::VT_CONTEXTMENUITEM:
        eClass::m_pVfTable_ContextMenuItem = dwVfptr;
        break;
      case EClassVTable::VT_PERMISSIONDELEGATE:
        eClass::m_pVfTable_PermissionDelegate = dwVfptr;
        break;
      case EClassVTable::VT_COOKIEMANAGER:
        eClass::m_pVfTable_CookieManager = dwVfptr;
        break;
      case EClassVTable::VT_RESOURCEREQUESTCALLBACK:
        eClass::m_pVfTable_ResourceRequestCallback = dwVfptr;
        break;
      case EClassVTable::VT_RESOURCERESPONSECALLBACK:
        eClass::m_pVfTable_ResourceResponseCallback = dwVfptr;
        break;
      case EClassVTable::VT_BASICAUTHENTICATIONCALLBACK:
        eClass::m_pVfTable_BasicAuthenticationCallback = dwVfptr;
        break;
      case EClassVTable::VT_DOWNLOADOPERATION:
        eClass::m_pVfTable_DownloadOperation = dwVfptr;
        break;
      case EClassVTable::VT_DOWNLOADCONFIRM:
        eClass::m_pVfTable_DownloadConfirm = dwVfptr;
        break;
      case EClassVTable::VT_FRAME:
        eClass::m_pVfTable_Frame = dwVfptr;
        break;
      case EClassVTable::VT_DOM:
        eClass::m_pVfTable_DOM = dwVfptr;
        break;
      case EClassVTable::VT_BROWSEREXTENSION:
        eClass::m_pVfTable_BrowserExtension = dwVfptr;
        break;

      default:
        break;
    }

    dwVfptr += sizeof(DWORD);
    DWORD dwSrcAddr = *(DWORD *)dwVfptr;
    dwSrcAddr += 6;
    BYTE *pCoder = (BYTE *)dwSrcAddr;
    if (*pCoder != 0xE9)
      eClass::SetFunctionHookAddr((LPVOID)dwSrcAddr, eClass::Copy);
  }
}

EV_EXPORTS(ObjectAddRef, void)(LPVOID obj, EClassVTable nType) {
  if (!obj) return;

  switch (nType) {
    case EClassVTable::VT_ENVIRONMENT:
      static_cast<edgeview::EnvironmentData *>(obj)->AddRef();
      break;
    case EClassVTable::VT_BROWSER:
      static_cast<edgeview::BrowserData *>(obj)->AddRef();
      break;
    case EClassVTable::VT_NEWWINDOWDELEGATE:
      static_cast<edgeview::NewWindowDelegate *>(obj)->AddRef();
      break;
    case EClassVTable::VT_SCRIPTDIALOGDELEGATE:
      static_cast<edgeview::ScriptDialogDelegate *>(obj)->AddRef();
      break;
    case EClassVTable::VT_CONTEXTMENUPARAMS:
      static_cast<edgeview::ContextMenuParams *>(obj)->AddRef();
      break;
    case EClassVTable::VT_CONTEXTMENUCOLLECTION:
      static_cast<edgeview::ContextMenuCollection *>(obj)->AddRef();
      break;
    case EClassVTable::VT_CONTEXTMENUITEM:
      static_cast<edgeview::ContextMenuItem *>(obj)->AddRef();
      break;
    case EClassVTable::VT_PERMISSIONDELEGATE:
      static_cast<edgeview::PermissionDelegate *>(obj)->AddRef();
      break;
    case EClassVTable::VT_COOKIEMANAGER:
      static_cast<edgeview::CookieManagerData *>(obj)->AddRef();
      break;
    case EClassVTable::VT_RESOURCEREQUESTCALLBACK:
      static_cast<edgeview::ResourceRequestCallback *>(obj)->AddRef();
      break;
    case EClassVTable::VT_RESOURCERESPONSECALLBACK:
      static_cast<edgeview::ResourceResponseCallback *>(obj)->AddRef();
      break;
    case EClassVTable::VT_BASICAUTHENTICATIONCALLBACK:
      static_cast<edgeview::BasicAuthenticationCallback *>(obj)->AddRef();
      break;
    case EClassVTable::VT_DOWNLOADOPERATION:
      static_cast<edgeview::DownloadOperation *>(obj)->AddRef();
      break;
    case EClassVTable::VT_DOWNLOADCONFIRM:
      static_cast<edgeview::DownloadConfirm *>(obj)->AddRef();
      break;
    case EClassVTable::VT_FRAME:
      static_cast<edgeview::FrameData *>(obj)->AddRef();
      break;
    case EClassVTable::VT_DOM:
      static_cast<edgeview::DOMOperation *>(obj)->AddRef();
      break;
    case EClassVTable::VT_BROWSEREXTENSION:
      static_cast<edgeview::ExtensionData *>(obj)->AddRef();
      break;

    default:
      break;
  }
}

EV_EXPORTS(ObjectRelease, void)(LPVOID obj, EClassVTable nType) {
  if (!obj) return;

  switch (nType) {
    case EClassVTable::VT_ENVIRONMENT:
      static_cast<edgeview::EnvironmentData *>(obj)->Release();
      break;
    case EClassVTable::VT_BROWSER:
      static_cast<edgeview::BrowserData *>(obj)->Release();
      break;
    case EClassVTable::VT_NEWWINDOWDELEGATE:
      static_cast<edgeview::NewWindowDelegate *>(obj)->Release();
      break;
    case EClassVTable::VT_SCRIPTDIALOGDELEGATE:
      static_cast<edgeview::ScriptDialogDelegate *>(obj)->Release();
      break;
    case EClassVTable::VT_CONTEXTMENUPARAMS:
      static_cast<edgeview::ContextMenuParams *>(obj)->Release();
      break;
    case EClassVTable::VT_CONTEXTMENUCOLLECTION:
      static_cast<edgeview::ContextMenuCollection *>(obj)->Release();
      break;
    case EClassVTable::VT_CONTEXTMENUITEM:
      static_cast<edgeview::ContextMenuItem *>(obj)->Release();
      break;
    case EClassVTable::VT_PERMISSIONDELEGATE:
      static_cast<edgeview::PermissionDelegate *>(obj)->Release();
      break;
    case EClassVTable::VT_COOKIEMANAGER:
      static_cast<edgeview::CookieManagerData *>(obj)->Release();
      break;
    case EClassVTable::VT_RESOURCEREQUESTCALLBACK:
      static_cast<edgeview::ResourceRequestCallback *>(obj)->Release();
      break;
    case EClassVTable::VT_RESOURCERESPONSECALLBACK:
      static_cast<edgeview::ResourceResponseCallback *>(obj)->Release();
      break;
    case EClassVTable::VT_BASICAUTHENTICATIONCALLBACK:
      static_cast<edgeview::BasicAuthenticationCallback *>(obj)->Release();
      break;
    case EClassVTable::VT_DOWNLOADOPERATION:
      static_cast<edgeview::DownloadOperation *>(obj)->Release();
      break;
    case EClassVTable::VT_DOWNLOADCONFIRM:
      static_cast<edgeview::DownloadConfirm *>(obj)->Release();
      break;
    case EClassVTable::VT_FRAME:
      static_cast<edgeview::FrameData *>(obj)->Release();
      break;
    case EClassVTable::VT_DOM:
      static_cast<edgeview::DOMOperation *>(obj)->Release();
      break;
    case EClassVTable::VT_BROWSEREXTENSION:
      static_cast<edgeview::ExtensionData *>(obj)->Release();
      break;

    default:
      break;
  }
}

__declspec(naked) void eClass::Copy(void) {
  __asm {
		mov esi, [esp];
		call Alloc;
		mov edi, eax;
		pop ecx;
		xchg ecx, esi;
		push ecx;
		push esi;
		push edi;
		call memcpy;
		push edi;
		lea eax, [esp];
		push esi;
		push eax;
		call[ebx + 8];
		pop eax;
		retn;
  }
}

LPVOID eClass::SetFunctionHookAddr(LPVOID lpSrcAddr, LPVOID lpHookAddr) {
  DWORD dwOldProtect;
  char *lSrcAddr = (char *)lpSrcAddr;
  ULONG nOffset = (*(ULONG *)&lSrcAddr[1]);
  ULONG pAddr = (DWORD)lpSrcAddr + nOffset;
  pAddr += 5;
  LPVOID lpSrcFunction = (LPVOID)pAddr;
  DWORD dwOffset = (DWORD)lpSrcAddr - (DWORD)lpHookAddr;
  dwOffset = ~dwOffset;
  dwOffset -= 4;
  if (VirtualProtectEx((HANDLE)-1, lSrcAddr, 5, PAGE_EXECUTE_READWRITE,
                       &dwOldProtect)) {
    lSrcAddr[0] = 0xE9;
    *(DWORD *)&lSrcAddr[1] = dwOffset;
    VirtualProtectEx((HANDLE)-1, lSrcAddr, 5, dwOldProtect, NULL);
  }
  return lpSrcFunction;
}

LPVOID __stdcall eClass::Alloc(int nSize) {
  return HeapAlloc(GetProcessHeap(), 0, nSize);
}

void __stdcall eClass::memcpy(void *dest, void *src, int size) {
  ::memcpy(dest, src, size);
}