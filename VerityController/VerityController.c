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

/* We respond implement `IUnknown` and `IObjectWithSite`. */
static HRESULT STDMETHODCALLTYPE
QueryInterface(IObjectWithSite *pSelf, REFIID guidVtbl, void **ppv)
{
    /* Check that we're being asked for the one of the two interaces we provide.
     * If not, clear the result pointer and send an error flag.
     */
    if (!IsEqualIID(guidVtbl, &IID_IUnknown) && 
        !IsEqualIID(guidVtbl, &IID_IObjectWithSite))
    {
        *ppv = 0;
        return E_NOINTERFACE;
    }

    /* Return the `IObjectWithSite` interface, which is the only interface we
     * support, so we can return the object passed in. Set the result to self,
     * increment the refernece count and flag success.
     */
    *ppv = pSelf;
    pSelf->lpVtbl->AddRef(pSelf);
    return NOERROR;
}

/* `AddRef` &mdash; More boilerplate COM. Increment the reference count and
 * return the updated count.
 */
static ULONG STDMETHODCALLTYPE
AddRef(IObjectWithSite *pSelf)
{
    IVerityController* pVerity = (IVerityController*) pSelf;
    ++pVerity->dwCount;
    return (pVerity->dwCount);
}

/* `Release` &mdash; Move boilerplate COM. Decrement the reference count and
 * return the updated count. If the count reaches zero, we free the object.
 */
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

/* We want to know when a page loads so we can check to see if we have a test we
 * want to run against the page. This function installs an event handler for
 * document load events. The web browser will not send us a document load event
 * specifically. It will send us a slew of events. Our callback will filter. */

/* */
static HRESULT
HookSite(IVerityController* pVerity)
{
    HRESULT err;
    IUnknown* pSite = pVerity->pSite;
    IConnectionPointContainer* pCPC;
    IConnectionPoint* pCP;
    /* Get the connection point collection. Ask for the web browser connection
     * point. Register a callback. The callback will be called for dozens of
     * different types of web browser events, but we're only interested in the
     * document load event. We can't make that distinction here, though, we make
     * it in our callback. We have to get all the events, ignoring all but the
     * documetn load event.
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

/* `SetSite` &mdash; After our Verity controller object is created, Internet
 * Explorer will call this method to set the site property, passing in the web
 * browser object, the object that is the the web browser client area, that
 * implements many different browser related COM interfaces. 
 */

/* */
static HRESULT STDMETHODCALLTYPE
SetSite(IObjectWithSite *pSelf, IUnknown *pUnknown)
{
    HRESULT err = S_OK;
    IVerityController* pVerity = (IVerityController*) pSelf;
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

/* The virtual table for our Verity controller COM object.
 *
 * Gather up the above functions into a virtual table structure and you have a
 * COM class.
 */
const IObjectWithSiteVtbl IVerityControllerVtbl =
{
    QueryInterface,
    AddRef,
    Release,
    SetSite,
    GetSite
};
