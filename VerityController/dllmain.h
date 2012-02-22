DWORD*
RegisterObjectFactory(const GUID *clsid, IClassFactory *factory, GenericFactory_Finalizer finalizer);
HRESULT ConstructOnDocumentHandlerFactory();