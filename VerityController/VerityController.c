// VerityController.cpp : Defines the exported functions for the DLL
// application.

#include "stdafx.h"
#include "Log.h"
#include "Jumps.h"
#include "VerityController.h"
#include "WebBrowserEvents.h"

// {0308456E-B8AA-4267-9A3E-09010E0A6754}
const GUID CLSID_IVerityController =
{ 0x308456e, 0xb8aa, 0x4267, 
    { 0x9a, 0x3e, 0x9, 0x1, 0xe, 0xa, 0x67, 0x54 } };

// IObjectWithSite. This is the object that defines a browser helper object.
// It is created by Internet Explorer as it loads and the SetSite method is
// invoked, passing in the browser object which implements IWebBrowser2. Using
// the IWebBrowser2 interface, we can set callbacks to fire when pages are
// loaded. That is the essnece of our application here, load tests when a page
// loads.

// The first three methods implement the IUnknown interface, which is the
// interface implemented by all COM objects. We use the IUnknown interface
// to manage the object lifecycle (reference counting), and to get pointers
// to other interfaces via QueryInterface.

static HRESULT STDMETHODCALLTYPE
QueryInterface(IObjectWithSite *self, REFIID vTableGuid, void **ppv);

static ULONG STDMETHODCALLTYPE
AddRef(IObjectWithSite *self);

static ULONG STDMETHODCALLTYPE
Release(IObjectWithSite *self);

static HRESULT STDMETHODCALLTYPE
GetSite(IObjectWithSite *, REFIID riid, void **ppvSite);

static HRESULT STDMETHODCALLTYPE
SetSite(IObjectWithSite *, IUnknown *pUnkSite);

#define TO_VERITY(o) ((IVerityController*)o)

// The remaining two methods in our object implement the IObjecWithSite
// interface.
const IObjectWithSiteVtbl IVerityControllerVtbl = {
    QueryInterface,
    AddRef,
    Release,
    SetSite,
    GetSite
};

static HRESULT STDMETHODCALLTYPE
QueryInterface(IObjectWithSite *pSelf, REFIID guidVtbl, void **ppv)
{
    // Check that we're being asked for the one and only interface we provide.
    // If not, clear the result pointer and send an error flag.
    if (!IsEqualIID(guidVtbl, &IID_IUnknown) && 
        !IsEqualIID(guidVtbl, &IID_IObjectWithSite))
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }

    // Return the IObjectWithSite interface, which is the only interface we
    // support, so we can return the object passed in. Set the result to self,
    // increment the refernece count and flag success.
    *ppv = pSelf;
    pSelf->lpVtbl->AddRef(pSelf);
    return NOERROR;
}

// Increment the reference count and return the updated count.
static ULONG STDMETHODCALLTYPE
AddRef(IObjectWithSite *pSelf)
{
    IVerityController* pVerity = (IVerityController*) pSelf;
    ++pVerity->dwCount;
    return (pVerity->dwCount);
}

// Decrement the reference count and return the updated count. If the count
// reaches zero, we free the object.
static ULONG STDMETHODCALLTYPE
Release(IObjectWithSite *pSelf)
{
    IVerityController* pVerity = (IVerityController*) pSelf;
    --pVerity->dwCount;
    if (pVerity->dwCount == 0)
    {
        Log(_T("Deleting.\n"));
        InterlockedIncrement(pVerity->pdwLockCount);
        GlobalFree(pSelf);
        return(0);
    }
    return (pVerity->dwCount);
}

static HRESULT
HookSite(IVerityController* pVerity)
{
    HRESULT err;
    IUnknown* pSite = pVerity->pSite;
    IConnectionPointContainer* pCPC;
    IConnectionPoint* pCP;
    Log(_T("Setting hooks.\n"));
    err = pSite->lpVtbl->QueryInterface(pSite, &IID_IConnectionPointContainer, &pCPC);
    IsOkay(err, undoSetSite);
    Log(_T("Got IConnectionPointContainer.\n"));
    err = pCPC->lpVtbl->FindConnectionPoint(pCPC, &DIID_DWebBrowserEvents2, &pCP);
    IsOkay(err, undoSetSite);
    pVerity->pSiteConnectionPoint = pCP;
    Log(_T("Got dispatch connection point.\n"));
    err = pCP->lpVtbl->Advise(pCP, (IUnknown *) &WebBrowserEvents, &pVerity->dwSiteAdviseCookie);
    IsOkay(err, undoConnectionPoint);
    Log(_T("Connection point advised.\n"));
    Done(exit);
undoConnectionPoint:
    pCP->lpVtbl->Release(pCP);
undoSetSite:
    pVerity->pSite->lpVtbl->Release(pVerity->pSite);
    pVerity->pSite = NULL;
exit:
    return err;
}

static void
UnhookSite(IVerityController* pVerity)
{
    IUnknown* pSite = pVerity->pSite;
    IConnectionPoint* pCP = pVerity->pSiteConnectionPoint;
    Log(_T("Unhook site.\n"));
    pCP->lpVtbl->Unadvise(pCP, pVerity->dwSiteAdviseCookie);
    pCP->lpVtbl->Release(pCP);
    pSite->lpVtbl->Release(pSite);
    pVerity->pSite = NULL;
}

static HRESULT STDMETHODCALLTYPE
SetSite(IObjectWithSite *pSelf, IUnknown *pUnknown)
{
    HRESULT err = S_OK;
    IVerityController* pVerity = (IVerityController*) pSelf;
    if (pVerity->pSite)
    {
        UnhookSite(pVerity);
    }
    if (pVerity->pSite = pUnknown)
    {
        err = HookSite(pVerity);
    }
    Log(_T("Set site: %d %d.\n"), err, pUnknown);
    return err;
}

static HRESULT STDMETHODCALLTYPE
GetSite(IObjectWithSite *pSelf, REFIID riid, void **ppvSite)
{
    HRESULT err = S_FALSE;
    IVerityController* pVerity = (IVerityController*) pSelf;
    IUnknown* pSite = pVerity->pSite;
    if (pSite)
    {
        err = pSite->lpVtbl->QueryInterface(pSite, riid, ppvSite);
    }
    Log(_T("Get site: %d\n"), err);
    return err;
}
