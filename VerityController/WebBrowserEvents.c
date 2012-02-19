#include "stdafx.h"

#include "Log.h"
#include "Jumps.h"
#include "GenericFactory.h"
#include "ComponentObjectModel.h"
#include "Pool.h"
#include <Winhttp.h>

// {73C6AB50-2BEE-4DC0-AB1C-910C255D1C23}
DEFINE_GUID(CLSID_ActiveScriptSite, 
0x73c6ab50, 0x2bee, 0x4dc0, 0xab, 0x1c, 0x91, 0xc, 0x25, 0x5d, 0x1c, 0x23);


static DWORD *pdwLockCount;

/* {42939237-42F0-4E3F-818E-FA63E4EB5A82} */
static const GUID CLSID_IOnDocument =
{ 0x42939237, 0x42f0, 0x4e3f, { 0x81, 0x8e, 0xfa, 0x63, 0xe4, 0xeb, 0x5a, 0x82 } };

typedef struct OnDocument
{
    const IDispatchVtbl *lpVtbl;
    DWORD dwCount;
}
OnDocument;

static REFIID ariidImplemented[] =
{
    &IID_IUnknown,
    &IID_IDispatch,
    NULL
};

QUERY_INTERFACE( IDispatch, ariidImplemented)
REFERENCE_COUNT(IDispatch, OnDocument, pdwLockCount, NO_DESTRUCTOR)

static HRESULT STDMETHODCALLTYPE
IDispatch_GetTypeInfoCount(   
    IDispatch * pSelf, UINT *pctinfo
) {
    *pctinfo = 0;
    Log(_T("Unexpected call to IDispatch::GetTypeInfoCount.\n"));
    return S_OK;
}

        
static HRESULT STDMETHODCALLTYPE
IDispatch_GetTypeInfo( 
    IDispatch * This, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo
) {
    Log(_T("Unexpected call to IDispatch::GetTypeInfo.\n"));
    return DISP_E_UNKNOWNNAME;
}
        
static HRESULT STDMETHODCALLTYPE
IDispatch_GetIDsOfNames(
    IDispatch * This,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId
) {
    Log(_T("Unexpected call to IDispatch::GetIDsOfNames.\n"));
    return DISP_E_UNKNOWNNAME;
}

// TODO XHR doe snot have timeout mechanism, so we're going to have to use the
// special timer ActiveX object that came along with Internet Explorer and XHR.
// TODO This needs to be called IScriptInjector. It uses the peculiar behavior
// of IXMLHttpRequest, that of accepting an IDispatch interface objet as a
// readystatechange handler, through direct assignment of the handler, but not
// through QueryInterface.
// TODO Create a QueryInterface that will assert to ensure that IXHMLHttpRequest
// does what we expect it to do, out of curiosity more than correctness. 
typedef struct CReadyStateChange IReadyStateChange;

typedef HRESULT(*OnReadyStateComplete)(IReadyStateChange* pRSC);

typedef struct CReadyStateChange {
    const IDispatchVtbl*    lpVtbl;
    IWebBrowser2*           pBrowser;
    IXMLHttpRequest*        pXHR;
    HANDLE                  hPool;
    OnReadyStateComplete    pOnReadyStateComplete;
} IReadyStateChange;

static const IDispatchVtbl IReadyStateChangeVtbl;

// Performs a HTTP request using the IXMLHttpRequest allocated at startup. If
// this method fails, it is up to the caller to release the pool.
static HRESULT
HttpRequest(
    IReadyStateChange* pRSC, BSTR bstrMethod, BSTR bstrURL, VARIANT* pvBody
) { 
    HRESULT err;
    VARIANT vAsync, vUser, vPassword, vBody;
    IXMLHttpRequest* pXHR = pRSC->pXHR;
    
    VariantInit(&vAsync);
    vAsync.vt = VT_BOOL;
    vAsync.boolVal = TRUE;
    
    VariantInit(&vUser);
    vUser.vt = VT_EMPTY;
    
    VariantInit(&vPassword);
    vUser.vt = VT_EMPTY;

    VariantInit(&vBody);
    vBody.vt = VT_EMPTY;

    err = pXHR->lpVtbl->open(pXHR, bstrMethod, bstrURL, vAsync, vUser, vPassword);
    IsOkay(err, exit);
    
    err = pXHR->lpVtbl->put_onreadystatechange(pXHR, (IDispatch*) pRSC);
    IsOkay(err, exit);

    err = pXHR->lpVtbl->send(pXHR, vBody);
exit:
    return err;
}

static HRESULT
GetEngineName(LPCTSTR buffer, int length, LPCTSTR subKey)
{
    HKEY hk, hkSub;
    DWORD err, type = REG_SZ, size = length,
          read = KEY_QUERY_VALUE | KEY_READ;
    Log(L"Get engine name!\n");
    if (!RegOpenKeyEx(HKEY_CLASSES_ROOT, buffer, 0, read, &hk))
    {
        Log(L"%s\n", buffer);
        if (!RegOpenKeyEx(hk, L"CLSID", 0, read, &hkSub))
        {
            err = RegQueryValueExW(hkSub, 0, 0, &type, (LPBYTE) buffer, &size);
            RegCloseKey(hkSub);
            RegCloseKey(hk);
        }
        else if (subKey)
        {
            Log(L"HERE\n");
            if (!RegOpenKeyEx(hk, subKey, 0, read, &hkSub))
            {
                err = RegQueryValueEx(hkSub, 0, 0, &type,
                        (LPBYTE) buffer, &size);
                RegCloseKey(hkSub);
                RegCloseKey(hk);
                if (!err)
                {
                    err = GetEngineName(buffer, length, NULL);
                } 
            }
        }
    }
    return E_FAIL;
}

static HRESULT
GetEngineGUID(LPCTSTR lpzExtension, GUID *guidBuffer)
{
    HKEY hk;
    wchar_t buffer[256];
    DWORD type = REG_SZ, size, err, read = KEY_QUERY_VALUE | KEY_READ;

    Log(L"HERE %s\n", lpzExtension);
    if (!RegOpenKeyEx(HKEY_CLASSES_ROOT, lpzExtension, 0, read, &hk))
    {
        size = sizeof(buffer);
        err = RegQueryValueEx(hk, 0, 0, &type, (LPBYTE)buffer, &size);
        RegCloseKey(hk);
        if (!err)
        {
            GetEngineName(buffer, sizeof(buffer), L"ScriptEngine");
        }
        Log(L"As String %s.\n", buffer);
        if (!err)
        {
            Log(L"Hey, lady!\n");
            return CLSIDFromString(buffer, guidBuffer);
        }
    }
    return E_FAIL;
}

static HRESULT
OnDocumentComplete(IDispatch* pDispatch, BSTR bstrReferer)
{
    HRESULT hr;
    OnDocument *pOnDocument = (OnDocument *)pDispatch;
    IActiveScript *pActiveScript = NULL;
    IActiveScriptSite *pActiveScriptSite = NULL;
    IActiveScriptParse *pActiveScriptParse = NULL;
    GUID guidJavaScript;

    GetEngineGUID(L".js", &guidJavaScript);
    hr = CoCreateInstance(&guidJavaScript, 0, CLSCTX_ALL, &IID_IActiveScript, (LPVOID*)&pActiveScript);
    Log(L"Success? %d %d\n", hr == REGDB_E_CLASSNOTREG, pActiveScript); 

    pActiveScript->lpVtbl->QueryInterface(pActiveScript, &IID_IActiveScriptParse, &pActiveScriptParse);

    pActiveScriptParse->lpVtbl->InitNew(pActiveScriptParse);

    hr = CoCreateInstance(&CLSID_ActiveScriptSite, 0, CLSCTX_INPROC_SERVER, &IID_IActiveScriptSite, (LPVOID*)&pActiveScriptSite);
    pActiveScript->lpVtbl->SetScriptSite(pActiveScript, pActiveScriptSite);

    pActiveScript->lpVtbl->AddNamedItem(pActiveScript, L"verity", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_NOCODE);

    pActiveScriptParse->lpVtbl->ParseScriptText(pActiveScriptParse, L"verity.createObservable();", 0, 0, 0, 0, 0, 0, 0, 0);
    pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
    pActiveScriptParse = NULL;

    pActiveScript->lpVtbl->SetScriptState(pActiveScript, SCRIPTSTATE_CONNECTED);

    pActiveScript->lpVtbl->Close(pActiveScript);

    if (pActiveScriptParse)
    {
        pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
    }

    if (pActiveScript)
    {
        pActiveScript->lpVtbl->Release(pActiveScript);
    }

    return hr;
}

static HRESULT
_OnDocumentComplete(IDispatch* pDispatch, BSTR bstrReferer)
{
    // Made sure to add a bunch of extra lower case letters to the start of all
    // my variables so this code could have that Microsoft look and feel.
    HRESULT             err;
    IWebBrowser2*       pBrowser;
    HANDLE              hPool;
    IReadyStateChange*  pRSC;
    BSTR                bstrMethod;
    BSTR                bstrURL;
    VARIANT*            pvBody;
    IXMLHttpRequest*    pXHR;

    // The pool manages memory, strings, and interface handles, as well as
    // doubly-linked list nodes used to form arbitrary lists as needed.
    hPool = PoolAllocPool();
    IsAllocated(hPool, err, exit);

    // We create a structure to track state while making web requests for test
    // resources. This structure is also the IDispatch interface given to the
    // IXMLHttpRequest as a callback for ready state changes.
    pRSC = (IReadyStateChange*) PoolAllocMemory(hPool, sizeof(IReadyStateChange));
    IsAllocated(pRSC, err, releasePool);

    // We want to grab an IWebBrowser2 interface handle now, to increment the
    // reference count, otherwise it might get collected while we're waiting on
    // HTTP requests.
    err = PoolQueryInterface(hPool, pDispatch, &IID_IWebBrowser2, &pBrowser);
    IsOkay(err, releasePool);

    // Allocate an IXMLHttpRequest to use for this test run.
    err = PoolCoCreateInstance(hPool, &CLSID_XMLHTTPRequest, &IID_IXMLHttpRequest, &pXHR);
    IsOkay(err, exit);

    // In the future, during other callbacks, all resources allocated during
    // test processing can be collected by freeing the hPool member of the
    // IReadyStateChange structure, including the IReadyStateChange structure
    // itself. That is, the IReadyStateChange has a reference to the pool, but
    // the pool still manages and frees the memory allocated for the
    // IReadyStateChange structure itself.
    pRSC->hPool     = hPool;
    pRSC->lpVtbl    = &IReadyStateChangeVtbl;
    pRSC->pBrowser  = pBrowser;
    pRSC->pXHR      = pXHR;

    // We're going to end up handing over this variant to the send method of
    // IXMLHttpRequest. I don't know if IXMLHttpRequest will be done with the
    // variant when send returns, of if it will hold onto it and send it out
    // over the wire in chunks, so creating a variant on the heap, and
    // duplicating the string are both in defense of a delayed read.
    pvBody = (VARIANT*) PoolAllocMemory(hPool, sizeof(VARIANT));
    IsAllocated(pvBody, err, releasePool);

    VariantInit(pvBody);
    pvBody->vt = VT_BSTR;
    pvBody->bstrVal = PoolAllocString(hPool, bstrReferer);
    IsAllocated(pvBody->bstrVal, err, releasePool);

    bstrMethod = PoolAllocString(hPool, L"POST");
    IsAllocated(bstrMethod, err, releasePool);

    bstrURL = PoolAllocString(hPool, L"http://verity:8078/test/expected");
    IsAllocated(bstrURL, err, releasePool);

    err = HttpRequest(pRSC, bstrMethod, bstrURL, pvBody);
    IsOkay(err, releasePool);

    Done(exit);
releasePool:
    PoolFreePool(hPool);
exit:
    return err;
}

static HRESULT
ExecTest(IWebBrowser2* pBrowser)
{
    HRESULT err;
    IHTMLDocument2* pDoc;
    IHTMLWindow2*   pWindow;
    IDispatch*      pDispatch;
    
    BSTR            script = NULL, type = NULL;

    VARIANT         v;
    
    VariantInit(&v);
    v.vt = VT_EMPTY;

    err = pBrowser->lpVtbl->get_Document(pBrowser, &pDispatch);
    IsOkay(err, releaseBrowser); 

    err = pDispatch->lpVtbl->QueryInterface(pDispatch, &IID_IHTMLDocument2, &pDoc);
    IsOkay(err, releaseBrowser);

    err = pDoc->lpVtbl->get_parentWindow(pDoc, &pWindow);
    IsOkay(err, releaseDoc);

    script = SysAllocString(_T("alert('Hello, World!');"));
    type = SysAllocString(_T("javascript"));

    if (script && type)
    {
        err = pWindow->lpVtbl->execScript(pWindow, script, type, &v);
        if (err == E_INVALIDARG)
        {
            Log(_T("Invalid arg.\n"));
        }
    }

    if (script)
    {
        SysFreeString(script);
    }
    if (type)
    {
        SysFreeString(type);
    }
    Log(_T("IDispatch::Invoke Got Document. %d\n"), err);
releaseDoc:
    pDoc->lpVtbl->Release(pDoc);
releaseBrowser:
    pBrowser->lpVtbl->Release(pBrowser);
    return err;
}
        
static HRESULT STDMETHODCALLTYPE
IDispatch_Invoke(
    IDispatch * This,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr
) {
    HRESULT err = S_OK;
    if (dispIdMember == DISPID_DOCUMENTCOMPLETE) {
        // Parameters are passed in the opposite order of the documentation.
        // The IDispatch pointer is in the first argument of two arguments,
        // therefore it is the last argument here. The IDispatch pointer is
        // inside a VARIANT, so we need to go from the argument variant,
        // through a variant to get to our IDispatch.
        err = OnDocumentComplete(pDispParams->rgvarg[1].pvarVal->pdispVal,
                                 pDispParams->rgvarg[0].bstrVal);
        Log(_T("Document HOOOKED! %d\n"), GetCurrentThreadId());
    }
    return err;
}

HRESULT
SplitBoilerPlate(HANDLE hPool, LISTNODE* pNode)
{
    return S_OK;
}

static HRESULT
OnBoilerplateComplete(IReadyStateChange* pRSC)
{
    HRESULT err;
    IXMLHttpRequest* pXHR = pRSC->pXHR;
    HANDLE hPool = pRSC->pXHR;
    BSTR bstrResponseText, bstrMethod, bstrURL;
    VARIANT vBody;

    err = pXHR->lpVtbl->get_responseText(pXHR, &bstrResponseText);
    if (wcsstr(bstrResponseText, L"NONE"))
    {
        goto terminate;
    }
    else
    {
        bstrMethod = PoolAllocString(hPool, L"GET");
        IsAllocated(bstrMethod, err, terminate);

        bstrURL = PoolAllocString(hPool, bstrResponseText);
        IsAllocated(bstrMethod, err, terminate);

        VariantInit(&vBody);
        vBody.vt = VT_EMPTY;

        err = HttpRequest(pRSC, bstrMethod, bstrURL, &vBody);
        IsOkay(err, terminate);
    }
    Done(exit);
terminate:
    PoolFreePool(hPool);
exit:
    return err;
}

static HRESULT
OnExpectComplete(IReadyStateChange* pRSC)
{
    HRESULT err;
    IXMLHttpRequest* pXHR = pRSC->pXHR;
    HANDLE hPool = pRSC->hPool;
    BSTR bstrResponseText;
    BSTR bstrMethod, bstrURL;
    VARIANT vBody;

    err = pXHR->lpVtbl->get_responseText(pXHR, &bstrResponseText);
    if (wcsstr(bstrResponseText, L"NONE"))
    {
        goto terminate;
    }
    else
    {
        bstrMethod = PoolAllocString(hPool, L"GET");
        IsAllocated(bstrMethod, err, terminate);

        bstrURL = PoolAllocString(hPool, bstrResponseText);
        IsAllocated(bstrMethod, err, terminate);

        VariantInit(&vBody);
        vBody.vt = VT_EMPTY;

        err = HttpRequest(pRSC, bstrMethod, bstrURL, &vBody);
        IsOkay(err, terminate);
    }
    Done(exit);
terminate:
    PoolFreePool(hPool);
exit:
    return err;
}

static HRESULT STDMETHODCALLTYPE
IReadyStateChange_Invoke(
    IDispatch* pSelf,
    DISPID dispIdMember,
    REFIID riid,
    LCID lcid,
    WORD wFlags,
    DISPPARAMS *pDispParams,
    VARIANT *pVarResult,
    EXCEPINFO *pExcepInfo,
    UINT *puArgErr
) {
    long lState;
    HRESULT err             = S_OK;
    IReadyStateChange* pRSC = (IReadyStateChange*) pSelf;
    IXMLHttpRequest* pXHR   = pRSC->pXHR;
    Log(_T("Ready State Has Changed! %d\n"), GetCurrentThreadId());
    // If this fails, we're in a bad place, because freeing the
    // IXMLHttpRequest when it is this disfunctional, how can you know that
    // it is going actually cancel?
    err = pXHR->lpVtbl->get_readyState(pXHR, &lState);
    IsOkay(err, teriminate);
    if (lState == READYSTATE_COMPLETE)
    {
        err = pRSC->pOnReadyStateComplete(pRSC);
        IsOkay(err, teriminate);
    }
    Done(exit);
teriminate:
    pXHR->lpVtbl->abort(pXHR);
    PoolFreePool(pRSC->hPool);
exit:
    return err;
}
        
static const IDispatchVtbl OnDocumentVtbl = {
    IDispatch_QueryInterface,
    IDispatch_AddRef,
    IDispatch_Release,
    IDispatch_GetTypeInfoCount,
    IDispatch_GetTypeInfo,
    IDispatch_GetIDsOfNames,
    IDispatch_Invoke    
};

// Create an instance of the Verity browser helper object.
static HRESULT
OnDocument_CreateInstance(REFIID guidVtbl, void **ppv)
{
    HRESULT              hr;
    OnDocument         *pOnDocument;

    // Allocate the memory for the Verity browser helper object.
    if (!(pOnDocument = GlobalAlloc(GMEM_FIXED, sizeof(OnDocument))))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Set the virtual function table and reference count.
        pOnDocument->lpVtbl = &OnDocumentVtbl;
        pOnDocument->dwCount = 1;

        // Increment the library lock count.
        InterlockedIncrement(pdwLockCount);

        // Now use QueryInterface to set the caller's result pointer.
        // QueryInterface will also check that request interface is the
        // one supported by our Verity browser helper object.
        hr = OnDocumentVtbl.QueryInterface((IDispatch*) pOnDocument, guidVtbl, ppv);

        // If we had a problem above, then the reference count is still
        // one, so decrementing it will free the memory we just allocated,
        // otherwise, it is two and decrementing it makes it as it should
        // be.
        OnDocumentVtbl.Release((IDispatch*) pOnDocument);
    }

    Log(_T("Allocated: %d\n"), hr);
    return hr;
}

HRESULT
OnDocument_CreateFactory()
{
    return GenericFactory_CreateFactory(&CLSID_IOnDocument, OnDocument_CreateInstance, &pdwLockCount);
}
