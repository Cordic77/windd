#ifndef WINDD_COMSUPP_H_
#define WINDD_COMSUPP_H_

#include <objbase.h>              /* COM (Component Object Model): IID_PPV_ARGS macro [C++ only] */
#include <initguid.h>             /* struct GUID, DEFINE_GUID() */
#include <oleauto.h>              /* OLE Automation, SysAllocString(), SysFreeString() */

#ifdef __cplusplus
  #include <comdef.h>             /* _com_error */
  #include <comutil.h>            /* _bstr_t, _variant_t */
  #include <comip.h>              /* _com_ptr_t */

  /* C++ COM interface definitions: */
  #define COM_CPP_INTERFACE 1

  #define CLSID_REF
  #define IID_REF

  #define COM_TRY try
  #define COM_CATCH(what) catch (what)

  /* COM object macros: */
  #define COM_OBJ(obj) obj
  #define COM_OBJ_THIS_(obj)
  #define COM_OBJ_THIS(obj)

  /* DONBOX_61: */
  #define IID_PPV_ARG(iface, ppiface) IID_##iface, reinterpret_cast<void**> (static_cast <iface **> (ppiface))

  /* DONBOX_56: void SafeRelease (IUnknown * &rpUnk) */
  #define CoReleaseObject(pcom_obj) (CoReleaseObject) (&pcom_obj)

  template <class T> void (CoReleaseObject) (T **com_obj)
  {
    if (com_obj && *com_obj)
    {
      (*com_obj)->Release ();
      *com_obj = NULL;
    }
  }
#else
  /* DONBOX_43: */
  #define FACILITY_ITF_RESERVED  0x200  /* values>=0x200: avoid collisions with existing HRESULTs */

  /* ANSI C COM interface definitions: */
  #define COM_CPP_INTERFACE 0

  #define CLSID_REF &
  #define IID_REF &

  #define COM_TRY
  #define COM_CATCH(what)

  /* COM object macros: */
  #define COM_OBJ(obj) obj->lpVtbl
  #define COM_OBJ_THIS_(obj) obj,
  #define COM_OBJ_THIS(obj) obj

  /* DONBOX_61: */
  #define IID_PPV_ARG(iface, ppiface) IID_REF IID_##iface, (void **)((iface **)(ppiface))

  /* DONBOX_56: void SafeRelease (IUnknown * &rpUnk) */
  #define CoReleaseObject(pcom_obj) (CoReleaseObject) (&(IUnknown *)pcom_obj)

  static inline void (CoReleaseObject) (IUnknown **com_obj)
  {
    if (com_obj && *com_obj)
    {
      IUnknown *p_this = *com_obj;

      #ifdef __cplusplus
      p_this->Release ();
      #else
      p_this->lpVtbl->Release (p_this);
      #endif

     *com_obj = NULL;
    }
  }

  /* */
  #define CoReleaseString(bstr) (CoReleaseString) (&bstr)

  static inline void (CoReleaseString) (BSTR *bstr)
  {
    if (bstr && *bstr)
    {
      SysFreeString (*bstr);
     *bstr = NULL;
    }
  }
#endif

/* */
extern bool
  InitializeCOM (void);
extern bool
  COMAvailable (void);
extern TCHAR *
  BSTR2TCHAR (BSTR bstr);
extern TCHAR *
  VAR2TCHAR (VARIANT const *var);
extern TCHAR *
  GUID2TCHAR (GUID const *guid);
extern void
  UninitializeCOM (void);

#endif
