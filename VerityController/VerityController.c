// Implementation of the Verity controller Browser Helper Object.

/* A Browser Helper Object is an implemention of the `IObjectWithSite`
 * interface. Once registered, an instance of this object is created by Internet
 * Explorer when it creates a new browser window or tab. The `SetSite` method is
 * invoked, passing in the browser object which implements the `IWebBrowser2`
 * interface. Using the `IWebBrowser2` interface, we can set callbacks to fire
 * when pages are loaded. 
 *
 * That is the essence of our application, to load tests when a page loads.
 *
 * The first three methods implement the IUnknown interface, which is the
 * interface implemented by all COM objects. We use the IUnknown interface
 * to manage the object lifecycle (reference counting), and to get pointers
 * to other interfaces via QueryInterface.
 */

/* System includes are in the precompiled header file. */
#include "stdafx.h"

/* Local includes. */
#include "Log.h"
#include "Jumps.h"
#include "VerityController.h"
#include "WebBrowserEvents.h"
#include "ComponentObjectModel.h"
#include "GenericFactory.h"

static DWORD *pdwLockCount;

#pragma once
// The Verity browser helper object data.
typedef struct ObjectWithSite
{
    const IObjectWithSiteVtbl* lpVtbl;
    DWORD               dwCount;
    IConnectionPoint*   pSiteConnectionPoint;
    DWORD               dwSiteAdviseCookie;
    IUnknown*           pSite;
}
ObjectWithSite;

// Class GUID for the Verity controller Browser Helper Object. 

// String value is `"{0308456E-B8AA-4267-9A3E-09010E0A6754}"`.
const GUID CLSID_IVerityController =
{ 0x308456e, 0xb8aa, 0x4267, 
    { 0x9a, 0x3e, 0x9, 0x1, 0xe, 0xa, 0x67, 0x54 } };

/* To create a COM object class, we define the functions, then at the end of the
 * source file, we'll define a static structure that contains the functions.
 *
 * The Verity controller object first defines the `IUnknown` interface as all
 * COM objects. The `IObjectWithSite` interface contains the three methods
 * common to all COM objects, the `IUnknown` interface functions. It ads two
 * methods `GetSite` and `SetSite`, which define a site property for the object
 */

/* `QueryInterface` &mdash; COM boilerplate that clients use to obtain pointers
 * to virtual tables containing the methods that compose the different
 * interfaces implemented by the COM object. This is [basic
 * COM](http://msdn.microsoft.com/en-us/library/ms809982.aspx).
 *
 */

static REFIID ariidImplemented[] =
{
    &IID_IUnknown,
    &IID_IObjectWithSite,
    NULL
};

QUERY_INTERFACE(IObjectWithSite, ariidImplemented)
REFERENCE_COUNT(IObjectWithSite, ObjectWithSite, pdwLockCount, NO_DESTRUCTOR)

/* We want to know when a page loads so we can check to see if we have a test we
 * want to run against the page. This function installs an event handler for
 * document load events. The web browser will not send us a document load event
 * specifically. It will send us a slew of events. Our callback will filter. */

/* */
static HRESULT
HookSite(ObjectWithSite* pVerity)
{
    HRESULT err;
    IUnknown* pSite = pVerity->pSite;
    IConnectionPointContainer* pCPC;
    IConnectionPoint* pCP;

    /* Get the connection point collection. Ask for the web browser connection
     * point. Register a callback. The callback will be called for dozens of
     * different types of web browser events, but we're only interested in the
     * document load event. We can't make that distinction here, though, we
     * make it in our callback. We have to get all the events, ignoring all but
     * the document load event.
     */

    /* Obtain the [connection points
     * collection](http://www.codeproject.com/KB/COM/connectionpoint.aspx), a
     * collection of objects that register callbacks for events.
     */
    Log(_T("Setting hooks.\n"));
    err = pSite->lpVtbl->QueryInterface(pSite, &IID_IConnectionPointContainer, &pCPC);
    IsOkay(err, undoSetSite);
    Log(_T("Got IConnectionPointContainer.\n"));
    /* We get a connection point taht will inform us of [web browser
     * events](http://msdn.microsoft.com/en-us/library/aa768283%28v=vs.85%29.aspx).
     */
    err = pCPC->lpVtbl->FindConnectionPoint(pCPC, &DIID_DWebBrowserEvents2, &pCP);
    IsOkay(err, undoSetSite);
    pVerity->pSiteConnectionPoint = pCP;
    Log(_T("Got dispatch connection point.\n"));

    /* Register out web browser events callback function. */
    err = pCP->lpVtbl->Advise(pCP, (IUnknown *) &WebBrowserEvents, &pVerity->dwSiteAdviseCookie);
    IsOkay(err, undoConnectionPoint);
    Log(_T("Connection point advised.\n"));

    /* Jump to exit. */
    Done(exit);

    /* Or else cleanup on error. */
undoConnectionPoint:
    pCP->lpVtbl->Release(pCP);
undoSetSite:
    pVerity->pSite->lpVtbl->Release(pVerity->pSite);
    pVerity->pSite = NULL;
exit:
    /* Done. */
    return err;
}

/* When `SetSite` is called more than once, we make sure to remove our web
 * browser event handlers from the previous site that was set.
 */
static void
UnhookSite(ObjectWithSite* pVerity)
{
    IUnknown* pSite = pVerity->pSite;
    IConnectionPoint* pCP = pVerity->pSiteConnectionPoint;
    Log(_T("Unhook site.\n"));
    pCP->lpVtbl->Unadvise(pCP, pVerity->dwSiteAdviseCookie);
    pCP->lpVtbl->Release(pCP);
    pSite->lpVtbl->Release(pSite);
    pVerity->pSite = NULL;
}

/* `SetSite` &mdash; After our Verity controller object is created, Internet
 * Explorer will call this method to set the site property, passing in the web
 * browser object, the object that is the the web browser client area, that
 * implements many different browser related COM interfaces. 
 */

/* */
static HRESULT STDMETHODCALLTYPE
IObjectWithSite_SetSite(IObjectWithSite *pSelf, IUnknown *pUnknown)
{
    HRESULT err = S_OK;
    ObjectWithSite* pVerity = (ObjectWithSite*) pSelf;
    /* Unset the prevoius web browser object if any. Remove the document oad
     * event handler. */
    if (pVerity->pSite)
    {
        UnhookSite(pVerity);
    }
    /* Set the web browser object if any. */
    if (pVerity->pSite = pUnknown)
    {
        err = HookSite(pVerity);
    }
    Log(_T("Set site: %d %d.\n"), err, pUnknown);
    return err;
}

/* `GetSite` &mdash; Get the site set by set site. The other half of a property
 * getter/setter pair. You might not think this method would be called, because
 * Internet Explorer wouldn't count on us to keep track of which web browser
 * object is in its window, but it does.
 */
static HRESULT STDMETHODCALLTYPE
IObjectWithSite_GetSite(IObjectWithSite *pSelf, REFIID riid, void **ppvSite)
{
    HRESULT err = S_FALSE;
    ObjectWithSite* pVerity = (ObjectWithSite*) pSelf;
    IUnknown* pSite = pVerity->pSite;
    if (pSite)
    {
        err = pSite->lpVtbl->QueryInterface(pSite, riid, ppvSite);
    }
    Log(_T("Get site: %d\n"), err);
    return err;
}

/* The virtual table for our Verity controller COM object.
 *
 * Gather up the above functions into a virtual table structure and you have a
 * COM class.
 */
const IObjectWithSiteVtbl ObjectWithSiteVtbl =
{
    IObjectWithSite_QueryInterface,
    IObjectWithSite_AddRef,
    IObjectWithSite_Release,
    IObjectWithSite_SetSite,
    IObjectWithSite_GetSite
};

// Create an instance of the Verity browser helper object.
static HRESULT
ObjectWithSite_CreateInstance(
    REFIID guidVtbl, void **ppv
) {
    HRESULT              hr;
    ObjectWithSite   *pVerity;

    // Allocate the memory for the Verity browser helper object.
    if (!(pVerity = GlobalAlloc(GMEM_FIXED, sizeof(ObjectWithSite))))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Set the virtual function table and reference count.
        pVerity->lpVtbl = &ObjectWithSiteVtbl;
        pVerity->dwCount = 1;
        pVerity->pSite = NULL;
        pVerity->pSiteConnectionPoint = NULL;

        // Increment the library lock count.
        InterlockedIncrement(pdwLockCount);

        // Now use QueryInterface to set the caller's result pointer.
        // QueryInterface will also check that request interface is the
        // one supported by our Verity browser helper object.
        hr = ObjectWithSiteVtbl.QueryInterface(
                (IObjectWithSite*) pVerity, guidVtbl, ppv);

        // If we had a problem above, then the reference count is still
        // one, so decrementing it will free the memory we just allocated,
        // otherwise, it is two and decrementing it makes it as it should
        // be.
        ObjectWithSiteVtbl.Release((IObjectWithSite*) pVerity);
    }

    Log(_T("Allocated: %d\n"), hr);
    return hr;
}

HRESULT
ObjectWithSite_CreateFactory()
{
    return GenericFactory_CreateFactory(&CLSID_IVerityController,
            ObjectWithSite_CreateInstance, &pdwLockCount);
}
