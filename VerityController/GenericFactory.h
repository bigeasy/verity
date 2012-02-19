typedef HRESULT (*GenericFactory_Constructor) (REFIID guidVtbl, LPVOID *ppv);

HRESULT
GenericFactory_CreateFactory(
    const GUID *clsid, GenericFactory_Constructor constructor,
    DWORD **pdwLockCount
);

HRESULT
GenericFactory_CreateInstance(LPCTSTR clsid, REFIID riid, LPVOID *pObject);
