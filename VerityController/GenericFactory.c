/* A boilerplate COM object factory.
 *
 * The factory is a singleton. There is only ever one instance and it does not
 * get deallocated. Thus, the COM reference counting is a no-op. For an example
 * of COM reference in action, see the Verity controller object implementation.
 *
 * The factory does maintain state. It maintains our library lock count.
 */

/* System includes are in the precompiled header file. */
#include "stdafx.h"

/* Local includes. */
#include "Log.h"
#include "GenericFactory.h"
#include "dllmain.h"
#include "ComponentObjectModel.h"

/* Bogus lock count so we can use the COM macros used to define non-factory
 * objects. */
static int dwLockCount;

typedef struct GenericFactory {
    const IClassFactoryVtbl *lpVtbl;
    GenericFactory_Constructor constructor;
    DWORD dwCount;
    DWORD *pdwLockCount;
} GenericFactory;

// The object construction and server locking parts of the interface.

/* The IUnkown methods. Our singleton factory does not actually require an
 * reference counting, so we just ignore that. */

static REFIID ariidImplemented[] =
{
    &IID_IUnknown,
    &IID_IClassFactory,
    NULL
};

QUERY_INTERFACE(IClassFactory, ariidImplemented)
REFERENCE_COUNT(IClassFactory, GenericFactory, &dwLockCount, NO_DESTRUCTOR);

static HRESULT STDMETHODCALLTYPE
IClassFactory_CreateInstance(
    IClassFactory *pSelf, IUnknown *pOuter, REFIID guidVtbl, void **ppv
) {
    HRESULT              hr;
    GenericFactory     *pFactory = (GenericFactory*) pSelf;

    /* Assume the worst. */
    *ppv = 0;

    /* We never support aggregation. */
    if (pOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    else
    {
        hr = pFactory->constructor(guidVtbl, ppv);
    }

    Log(_T("Allocated: %d\n"), hr);
    return hr;
}

/* Increase or decrease the factory lock count. This prevents the ddl from
 * being unloaded. */
static HRESULT STDMETHODCALLTYPE
IClassFactory_LockServer(
    IClassFactory *pSelf, BOOL fLock
) {
    GenericFactory* pFactory = (GenericFactory*) pSelf;
    if (fLock) InterlockedIncrement(pFactory->pdwLockCount);
    else InterlockedDecrement(pFactory->pdwLockCount);
    return NOERROR;
}


// The factory virtual function table.
static const IClassFactoryVtbl GenericFactoryVtbl = {
    IClassFactory_QueryInterface,
    IClassFactory_AddRef,
    IClassFactory_Release,
    IClassFactory_CreateInstance,
    IClassFactory_LockServer
};

HRESULT
GenericFactory_CreateFactory(
    const GUID * clsid, GenericFactory_Constructor constructor,
    DWORD **ppdwLockCount
) {
    HRESULT hr = S_OK;
    GenericFactory *pFactory;
    if (pFactory = GlobalAlloc(GMEM_FIXED, sizeof(GenericFactory)))
    {
        dwLockCount++;
        pFactory->lpVtbl = &GenericFactoryVtbl;
        pFactory->constructor = constructor;
        pFactory->dwCount = 10;
        pFactory->pdwLockCount = RegisterObjectFactory(clsid, (IClassFactory*) pFactory);
        *ppdwLockCount = pFactory->pdwLockCount;
    }
    else
    {
        hr = E_OUTOFMEMORY;
    }
    return hr;
}
