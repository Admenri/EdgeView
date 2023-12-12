#pragma once

#include "edgeview_data.h"
#include "util.h"

typedef struct _eclass_vfptr {
  DWORD dwVfTable;
  LPVOID pObject;
  LPVOID pCFuncs;
} ECLASS_VFPTR, *PECLASS_VFPTR;

enum class EClassVTable {
  VT_ENVIRONMENT = 0,
  VT_BROWSER,
  VT_NEWWINDOWDELEGATE,
  VT_SCRIPTDIALOGDELEGATE,
  VT_CONTEXTMENUPARAMS,
  VT_CONTEXTMENUCOLLECTION,
  VT_CONTEXTMENUITEM,
  VT_PERMISSIONDELEGATE,
  VT_COOKIEMANAGER,
  VT_RESOURCEREQUESTCALLBACK,
  VT_RESOURCERESPONSECALLBACK,
  VT_BASICAUTHENTICATIONCALLBACK,
  VT_DOWNLOADOPERATION,
  VT_DOWNLOADCONFIRM,
  VT_FRAME,
  VT_DOM,
};

EV_EXPORTS(RegisterClass, void)(DWORD **pNewClass, EClassVTable nType);

EV_EXPORTS(ObjectAddRef, void)(LPVOID obj, EClassVTable nType);
EV_EXPORTS(ObjectRelease, void)(LPVOID obj, EClassVTable nType);

namespace eClass {

void Copy(void);

LPVOID __stdcall Alloc(int nSize);
void __stdcall memcpy(void *dest, void *src, int size);

LPVOID SetFunctionHookAddr(LPVOID lpSrcAddr, LPVOID lpHookAddr);

// Module Part
extern DWORD m_pVfTable_Environment;
extern DWORD m_pVfTable_Browser;
extern DWORD m_pVfTable_NewWindowDelegate;
extern DWORD m_pVfTable_ScriptDialogDelegate;
extern DWORD m_pVfTable_ContextMenuParams;
extern DWORD m_pVfTable_ContextMenuCollection;
extern DWORD m_pVfTable_ContextMenuItem;
extern DWORD m_pVfTable_PermissionDelegate;
extern DWORD m_pVfTable_CookieManager;
extern DWORD m_pVfTable_ResourceRequestCallback;
extern DWORD m_pVfTable_ResourceResponseCallback;
extern DWORD m_pVfTable_BasicAuthenticationCallback;
extern DWORD m_pVfTable_DownloadOperation;
extern DWORD m_pVfTable_DownloadConfirm;
extern DWORD m_pVfTable_Frame;
extern DWORD m_pVfTable_DOM;

}  // namespace eClass

// EPL Class Struct
#define IMP_NEWECLASS(LocalName, Object, Vfptr, funcs)    \
  ECLASS_VFPTR LocalName##T, *LocalName##TT, **LocalName; \
  LocalName##T.dwVfTable = Vfptr;                         \
  LocalName##T.pObject = (LPVOID)Object;                  \
  LocalName##T.pCFuncs = (LPVOID)funcs;                   \
  LocalName##TT = &LocalName##T;                          \
  LocalName = &LocalName##TT;
