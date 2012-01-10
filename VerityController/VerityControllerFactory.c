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
#include "VerityController.h"
#include "VerityControllerFactory.h"

/* The IUnkown methods. Our singleton factory does not actually require an
 * reference counting, so we just ignore that. */
static ULONG STDMETHODCALLTYPE
AddRef(IClassFactory *pSelf);

static ULONG STDMETHODCALLTYPE
Release(IClassFactory *pSelf);

static HRESULT STDMETHODCALLTYPE
QueryInterface(IClassFactory *pSelf, REFIID factoryGuid, void **ppv);

// The object construction and server locking parts of the interface.
static HRESULT STDMETHODCALLTYPE
CreateInstance(IClassFactory *pSelf, IUnknown *pUnknown,
               REFIID guidVtbl, void **ppv);

static HRESULT STDMETHODCALLTYPE
LockServer(IClassFactory *pSelf, BOOL fLock);

// The factory virtual function table.
static const IClassFactoryVtbl IVerityControllerFactoryVtbl = {
    QueryInterface,
    AddRef,
    Release,
    CreateInstance,
    LockServer
};

// The singleton factory instance.
IVerityControllerFactory factory = { &IVerityControllerFactoryVtbl, 0 };


/* `QueryInterface` &mdash;  Obtains a pointer to the factory singleton. */
static HRESULT STDMETHODCALLTYPE
QueryInterface(
    IClassFactory *pSelf, REFIID guidVtbl, void **ppv
) {
    /* If we do not implement the requested interface, set the result to null
     * and retrun an error flag. 
     */
    if (!IsEqualIID(guidVtbl, &IID_IUnknown) && 
        !IsEqualIID(guidVtbl, &IID_IClassFactory))
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }

    /* Set the result to our factory singleton and indicate success. */
    *ppv = pSelf;
    return NOERROR;
}
/* `AddRef` &mdash; We never create more than one instace of the factory so
 * adjusting the count always returns one.
 */
static ULONG STDMETHODCALLTYPE
AddRef(IClassFactory *pSelf)
{
    return(1);
}

/* `Release` &mdash; We never create more than one instace of the factory so
 * adjusting the count always returns one. We do not deallocate our factory, it
 * is a static variable.
 */
static ULONG STDMETHODCALLTYPE
Release(IClassFactory *pSelf)
{
    return(1);
}

// Create an instance of the Verity browser helper object.
static HRESULT STDMETHODCALLTYPE
CreateInstance(
    IClassFactory *pSelf, IUnknown *pOuter, REFIID guidVtbl, void **ppv
) {
    HRESULT              hr;
    IVerityController   *pVerity;

    // Assume the worst.
    *ppv = 0;

    // We don't support aggregation in IVerityController.
    if (pOuter)
    {
        hr = CLASS_E_NOAGGREGATION;
    }
    else
    {
        // Allocate the memory for the Verity browser helper object.
        if (!(pVerity = GlobalAlloc(GMEM_FIXED, sizeof(IVerityController))))
        {
            hr = E_OUTOFMEMORY;
        }
        else
        {
            // Set the virtual function table and reference count.
            pVerity->lpVtbl = &IVerityControllerVtbl;
            pVerity->dwCount = 1;
            pVerity->pdwLockCount = &((IVerityControllerFactory*)pSelf)->dwLockCount;
            pVerity->pSite = NULL;
            pVerity->pSiteConnectionPoint = NULL;

            // Increment the library lock count.
            InterlockedIncrement(pVerity->pdwLockCount);

            // Now use QueryInterface to set the caller's result pointer.
            // QueryInterface will also check that request interface is the
            // one supported by our Verity browser helper object.
            hr = IVerityControllerVtbl.QueryInterface(
                    (IObjectWithSite*) pVerity, guidVtbl, ppv);

            // If we had a problem above, then the reference count is still
            // one, so decrementing it will free the memory we just allocated,
            // otherwise, it is two and decrementing it makes it as it should
            // be.
            IVerityControllerVtbl.Release((IObjectWithSite*) pVerity);
        }
    }

    Log(_T("Allocated: %d\n"), hr);
    return hr;
}

// Increase or decrease the factory lock count. This prevents the ddl from
// being unloaded.
static HRESULT STDMETHODCALLTYPE
LockServer(
    IClassFactory *pSelf, BOOL fLock
) {
    IVerityControllerFactory* pFactory = (IVerityControllerFactory*) pSelf;
    if (fLock) InterlockedIncrement(&pFactory->dwLockCount);
    else InterlockedDecrement(&pFactory->dwLockCount);
    return NOERROR;
}
