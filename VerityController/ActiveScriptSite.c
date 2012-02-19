/* System includes are in the precompiled header file. */
#include "stdafx.h"
#include "Log.h"
#include "ComponentObjectModel.h"
#include "GenericFactory.h"
#include "ScriptContext.h"
#include "ActiveScriptSite.h"

// {73C6AB50-2BEE-4DC0-AB1C-910C255D1C23}
 const GUID CLSID_ActiveScriptSite =
{ 0x73c6ab50, 0x2bee, 0x4dc0, { 0xab, 0x1c, 0x91, 0xc, 0x25, 0x5d, 0x1c, 0x23 } };

static DWORD *pdwLockCount;

static REFIID ariidImplemented[] =
{
    &IID_IUnknown,
    &IID_IActiveScriptSite,
    NULL
};


QUERY_INTERFACE(IActiveScriptSite, ariidImplemented)
REFERENCE_COUNT(IActiveScriptSite, ActiveScriptSite, pdwLockCount, NO_DESTRUCTOR)

static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_GetLCID(IActiveScriptSite *site, LCID *plcid)
{
    return E_NOTIMPL;
}
        
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_GetItemInfo(
    IActiveScriptSite *pSelf, LPCOLESTR pstrName, DWORD dwReturnMask,
    IUnknown **ppiunkItem, ITypeInfo **ppti
) {
    ActiveScriptSite *pActiveScriptSite = (ActiveScriptSite*) pSelf;
    HRESULT hr = E_FAIL;
    if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
    {
        *ppiunkItem = NULL;
    }
    if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
    {
        *ppti = NULL;
    }
    if (wcscmp(L"verity", pstrName) == 0)
    {
        if (dwReturnMask & SCRIPTINFO_IUNKNOWN)
        {
            *ppiunkItem =  (IUnknown*) pActiveScriptSite->pScriptContext;
        }
        if (dwReturnMask & SCRIPTINFO_ITYPEINFO)
        {
            *ppti = NULL;
        }
    }
    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_GetDocVersionString(IActiveScriptSite *pSelf, BSTR *pbstrVersion)
{
    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnScriptTerminate(
    IActiveScriptSite *pSelf, const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo
) {
    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnStateChange(IActiveScriptSite *pSelf, SCRIPTSTATE ssScriptState)
{
    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnScriptError(IActiveScriptSite *pSelf, IActiveScriptError *pScriptError)
{
    ULONG ulLineNumber;
    BSTR bstrDesc;
    EXCEPINFO ei;

    pScriptError->lpVtbl->GetSourcePosition(pScriptError, 0, &ulLineNumber, 0);

    bstrDesc = 0;
    pScriptError->lpVtbl->GetSourceLineText(pScriptError, &bstrDesc);

    Log(L"Error on line %d: %s\n", ulLineNumber, bstrDesc);
    SysFreeString(bstrDesc);

    ZeroMemory(&ei, sizeof(EXCEPINFO));
    pScriptError->lpVtbl->GetExceptionInfo(pScriptError, &ei);

    Log(L"Error description: %s\n", ei.bstrDescription);

    SysFreeString(ei.bstrSource);
    SysFreeString(ei.bstrDescription);
    SysFreeString(ei.bstrHelpFile);

    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnEnterScript(IActiveScriptSite *pSelf)
{
    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnLeaveScript(IActiveScriptSite *pSelf)
{
    return S_OK;
}

IActiveScriptSiteVtbl ActiveScriptSiteVtbl =
{
    IActiveScriptSite_QueryInterface,
    IActiveScriptSite_AddRef,
    IActiveScriptSite_Release,
    IActiveScriptSite_GetLCID,
    IActiveScriptSite_GetItemInfo,
    IActiveScriptSite_GetDocVersionString,
    IActiveScriptSite_OnScriptTerminate,
    IActiveScriptSite_OnStateChange,
    IActiveScriptSite_OnScriptError,
    IActiveScriptSite_OnEnterScript,
    IActiveScriptSite_OnLeaveScript
};

// Create an instance of the Verity browser helper object.
static HRESULT
ActiveScriptSite_CreateInstance(
    REFIID guidVtbl, void **ppv
) {
    HRESULT              hr;
    ActiveScriptSite    *pActiveScriptSite;

    // Allocate the memory for the Verity browser helper object.
    if (!(pActiveScriptSite = GlobalAlloc(GMEM_FIXED, sizeof(ActiveScriptSite))))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Set the virtual function table and reference count.
        pActiveScriptSite->lpVtbl = &ActiveScriptSiteVtbl;
        pActiveScriptSite->dwCount = 1;
        CoCreateInstance(&CLSID_ScriptContext, NULL, CLSCTX_INPROC_SERVER, &IID_IDispatch, (LPVOID*)&pActiveScriptSite->pScriptContext);

        // Increment the library lock count.
        InterlockedIncrement(pdwLockCount);

        // Now use QueryInterface to set the caller's result pointer.
        // QueryInterface will also check that request interface is the
        // one supported by our Verity browser helper object.
        hr = ActiveScriptSiteVtbl.QueryInterface(
                (IActiveScriptSite*) pActiveScriptSite, guidVtbl, ppv);

        // If we had a problem above, then the reference count is still
        // one, so decrementing it will free the memory we just allocated,
        // otherwise, it is two and decrementing it makes it as it should
        // be.
        ActiveScriptSiteVtbl.Release((IActiveScriptSite*) pActiveScriptSite);
    }

    Log(_T("Allocated: %d\n"), hr);
    return hr;
}

HRESULT
ActiveScriptSite_CreateFactory()
{
    return GenericFactory_CreateFactory(&CLSID_ActiveScriptSite,
            ActiveScriptSite_CreateInstance, &pdwLockCount);
}
