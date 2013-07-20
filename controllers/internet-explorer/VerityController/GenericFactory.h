typedef HRESULT (*GenericFactory_Constructor) (REFIID guidVtbl, LPVOID *ppv);
typedef void (*GenericFactory_Finalizer) ();

HRESULT
GenericFactory_CreateFactory(
    const GUID *clsid, GenericFactory_Constructor createInstance,
    GenericFactory_Finalizer finalizer,
    DWORD **pdwLockCount
);

void
GenericFactory_GenericFinalizer();

HRESULT
GenericFactory_CreateInstance(LPCTSTR clsid, REFIID riid, LPVOID *pObject);
