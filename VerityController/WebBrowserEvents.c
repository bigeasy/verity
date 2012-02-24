#include "stdafx.h"

#include "Log.h"
#include "Jumps.h"
#include "GenericFactory.h"
#include "ComponentObjectModel.h"
#include "ScriptContext.h"
#include "ActiveScriptSite.h"
#include "Pool.h"
#include <Winhttp.h>

static DWORD *pdwLockCount;
static BSTR bstrSource;

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
    &DIID_DWebBrowserEvents2,
    NULL
};

QUERY_INTERFACE(IDispatch, ariidImplemented)
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
    ActiveScriptSite *pSite = NULL;

    Log(L"bstrReferer: %s\n", bstrReferer);
    Log(L"GUID TIME\n");
    GetEngineGUID(L".js", &guidJavaScript);
    Log(L"GUID GOT\n");
    hr = CoCreateInstance(&guidJavaScript, 0, CLSCTX_ALL, &IID_IActiveScript, (LPVOID*)&pActiveScript);
    Log(L"Success? %d %d\n", hr == REGDB_E_CLASSNOTREG, pActiveScript); 

    pActiveScript->lpVtbl->QueryInterface(pActiveScript, &IID_IActiveScriptParse, &pActiveScriptParse);

    pActiveScriptParse->lpVtbl->InitNew(pActiveScriptParse);

    hr = GenericFactory_CreateInstance(L"{73C6AB50-2BEE-4DC0-AB1C-910C255D1C23}", &IID_IActiveScriptSite, &pActiveScriptSite);
    pSite = (ActiveScriptSite*) pActiveScriptSite;
    pSite->pScriptContext->lpVtbl->SetURL((IScriptContext*)pSite->pScriptContext, bstrReferer);
    pActiveScript->lpVtbl->SetScriptSite(pActiveScript, pActiveScriptSite);

    pActiveScript->lpVtbl->AddNamedItem(pActiveScript, L"verity", SCRIPTITEM_ISVISIBLE|SCRIPTITEM_NOCODE);

    pActiveScriptParse->lpVtbl->ParseScriptText(pActiveScriptParse, bstrSource, 0, 0, 0, 0, 0, 0, 0, 0);
    pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
    pActiveScriptParse = NULL;

    pActiveScript->lpVtbl->SetScriptState(pActiveScript, SCRIPTSTATE_CONNECTED);

 /*   pActiveScript->lpVtbl->Close(pActiveScript);

    if (pActiveScriptParse)
    {
        pActiveScriptParse->lpVtbl->Release(pActiveScriptParse);
    }

    if (pActiveScript)
    {
        pActiveScript->lpVtbl->Release(pActiveScript);
    }*/

    return hr;
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
    if (dispIdMember == DISPID_NAVIGATECOMPLETE2) {
        // Parameters are passed in the opposite order of the documentation.
        // The IDispatch pointer is in the first argument of two arguments,
        // therefore it is the last argument here. The IDispatch pointer is
        // inside a VARIANT, so we need to go from the argument variant,
        // through a variant to get to our IDispatch.
        Log(_T("Document HOOOKED! %d\n"), GetCurrentThreadId());
        Log(L"Called: %d\n", pDispParams->cArgs);
        err = OnDocumentComplete(pDispParams->rgvarg[1].pvarVal->pdispVal,
            pDispParams->rgvarg[0].pvarVal->bstrVal);
    }
    return err;
}

static const IDispatchVtbl OnDocumentVtbl =
{
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

WCHAR *Scripts[] = { L"json2.js", L"boilerplate.js", L"background.js", L"ie.js", NULL };

struct ScriptSource
{
    CHAR *lpzSource;
    DWORD dwLength;
};

struct ScriptSource ScriptSources[4];

void
DirectoryName(WCHAR *lpzFileName)
{
    DWORD dwLength = wcslen(lpzFileName);
    while (lpzFileName[--dwLength] != '\\')
    { // Do nothing.
    }
    lpzFileName[dwLength + 1] = '\0';
}

HRESULT
LoadScripts(HMODULE hModule)
{
    DWORD dwLength, dwRead, dwMultiByteLength = 0, dwChunkSize;
    WCHAR *lpzChunk, *lpzWideSource;
    CHAR *lpzUTF8Source;
    HRESULT hr = S_OK;
    WCHAR lpzLibrary[MAX_PATH];
    HANDLE hFile;
    int i;

    GetModuleFileName(hModule, lpzLibrary, MAX_PATH);
    for (i = 0; Scripts[i]; i++)
    {
        DirectoryName(lpzLibrary);
        wcscat_s(lpzLibrary, MAX_PATH, Scripts[i]);
        hFile = CreateFile(lpzLibrary, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
        dwLength = GetFileSize(hFile, 0);
        lpzUTF8Source = (CHAR*) GlobalAlloc(GMEM_FIXED, dwLength + 1);
        ReadFile(hFile, lpzUTF8Source, dwLength, &dwRead, 0);
        dwMultiByteLength += MultiByteToWideChar(CP_UTF8, 0, lpzUTF8Source, dwLength, NULL, 0);
        ScriptSources[i].lpzSource = lpzUTF8Source;
        ScriptSources[i].dwLength = dwLength;
    }
    dwMultiByteLength++;

    dwChunkSize = dwMultiByteLength;
    lpzWideSource = lpzChunk = (WCHAR*) GlobalAlloc(GMEM_FIXED, dwMultiByteLength * sizeof(WCHAR));
    lpzWideSource[0] = '\0';
    for (i = 0; Scripts[i]; i++)
    {
        lpzUTF8Source = ScriptSources[i].lpzSource;
        dwLength = ScriptSources[i].dwLength;
        dwLength = MultiByteToWideChar(CP_UTF8, 0, lpzUTF8Source, dwLength, lpzChunk, dwChunkSize);
        lpzChunk += dwLength;
        dwChunkSize -= dwLength;
        GlobalFree(lpzUTF8Source);
    }
    *lpzChunk = '\0';
    if (wcslen(lpzWideSource) != dwMultiByteLength - 1)
    {
        return E_FAIL;
    }
    bstrSource = SysAllocString(lpzWideSource);
    GlobalFree(lpzWideSource);
    return hr;
}

void
OnDocument_Finalizer()
{
    if (bstrSource)
    {
        SysFreeString(bstrSource);
    }
}

HRESULT
OnDocument_CreateFactory(HMODULE hModule)
{
    LoadScripts(hModule);
    return GenericFactory_CreateFactory(&CLSID_IOnDocument, OnDocument_CreateInstance, GenericFactory_GenericFinalizer, &pdwLockCount);
}
