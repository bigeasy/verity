/* {7CD57D42-C553-4B82-A52A-513082F57EE0} */
DEFINE_GUID(CLSID_ScriptContext, 0x7cd57d42, 0xc553, 0x4b82, 0xa5, 0x2a, 0x51, 0x30, 0x82, 0xf5, 0x7e, 0xe0);

#undef  INTERFACE
#define INTERFACE IScriptContext
DECLARE_INTERFACE_ (INTERFACE, IDispatch)
{
   // IUnknown functions
   STDMETHOD  (QueryInterface)         (THIS_ REFIID, void **) PURE;
   STDMETHOD_ (ULONG, AddRef)          (THIS) PURE;
   STDMETHOD_ (ULONG, Release)         (THIS) PURE;
   // IDispatch functions
   STDMETHOD_ (ULONG, GetTypeInfoCount)(THIS_ UINT *) PURE;
   STDMETHOD_ (ULONG, GetTypeInfo)     (THIS_ UINT, LCID, ITypeInfo **) PURE;
   STDMETHOD_ (ULONG, GetIDsOfNames)   (THIS_ REFIID, LPOLESTR *, 
                                        UINT, LCID, DISPID *) PURE;
   STDMETHOD_ (ULONG, Invoke)          (THIS_ DISPID, REFIID, 
                                        LCID, WORD, DISPPARAMS *,
                                        VARIANT *, EXCEPINFO *, UINT *) PURE;
   // Extra functions
   STDMETHOD  (SetString)              (THIS_ BSTR) PURE;
   STDMETHOD  (GetString)              (THIS_ BSTR*) PURE;
   STDMETHOD  (CreateXHR)              (THIS_ IDispatch**) PURE;
   STDMETHOD  (CreateObservable)       (THIS_ IDispatch**) PURE;
};

struct ScriptContext;

typedef struct ProvideMultipleClassInfo
{
    IProvideMultipleClassInfoVtbl *lpVtbl;
    IScriptContext *pScriptContext;
}
ProvideMultipleClassInfo;

typedef struct ScriptContext
{
    IScriptContextVtbl *lpVtbl;
    ProvideMultipleClassInfo PMCI;
    DWORD dwCount;
    BSTR bstr;
}
ScriptContext;

