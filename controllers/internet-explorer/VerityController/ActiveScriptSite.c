/* System includes are in the precompiled header file. */
#include "stdafx.h"

/* Common project includes. */
#include "Log.h"
#include "ComponentObjectModel.h"
#include "GenericFactory.h"

/* The `ActiveScriptSite` object contains a `ScriptContext`. */
#include "ScriptContext.h"
#include "ActiveScriptSite.h"

/* The class GUID for `ActiveScriptSite` our implementation of
 * `IActiveScriptSite`. */
// {73C6AB50-2BEE-4DC0-AB1C-910C255D1C23}
 const GUID CLSID_ActiveScriptSite =
{ 0x73c6ab50, 0x2bee, 0x4dc0, { 0xab, 0x1c, 0x91, 0xc, 0x25, 0x5d, 0x1c, 0x23 } };

/* A count of outstanding objects used to track whether the library can be
 * unloaded. */
static DWORD *pdwLockCount;

/* Interfaces implemented by the `ActiveScriptSite` object. */
static REFIID ariidImplemented[] =
{
    &IID_IUnknown,
    &IID_IActiveScriptSite,
    NULL
};

/* Boilerplate implementation if `IUnknown`. */
QUERY_INTERFACE(IActiveScriptSite, ariidImplemented)
REFERENCE_COUNT(IActiveScriptSite, ActiveScriptSite, pdwLockCount, NO_DESTRUCTOR)

/* We don't really have a need i18n support in this plugin. When the time comes,
 * we'll implement it using HTML and JavaScript. */
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_GetLCID(IActiveScriptSite *site, LCID *plcid)
{
    return E_NOTIMPL;
}
        
/* We expose a `verity` object in the global namespace of the script. The
 * `verity` object is an instance of the `ScriptContext` object. Implemented
 * according to the fabulous instruction in [COM In Plain C, Part
 * 7](http://www.codeproject.com/Articles/15037/COM-in-plain-C-Part-7#REG) by
 * Jeff Glatt. */
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
        
/* Used to track changes to the script source for recompilation. */
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_GetDocVersionString(IActiveScriptSite *pSelf, BSTR *pbstrVersion)
{
    return E_NOTIMPL;
}
        
/* Used this at one point to harvest the generated test script, but the Active
 * Scripting engines do not terminate when the last instruction is executed, nor
 * when the last outstanding callback is invoked, ala Node.js. They count on the
 * host to terminate the script. */
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnScriptTerminate(
    IActiveScriptSite *pSelf, const VARIANT *pvarResult, const EXCEPINFO *pexcepinfo
) {
    Log(L"TERMINATE SCRIPT\n");
    return S_OK;
}
        
/* State changes. Not interested. */
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnStateChange(IActiveScriptSite *pSelf, SCRIPTSTATE ssScriptState)
{
    return S_OK;
}
        
/* Our scripts are static. Errors raised are bugs that need to be filed and
 * fixed. There is nothing for the user to do. We log our errors in an error log
 * in the DLL directory. */
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
        
/* More uninteresting events. */
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnEnterScript(IActiveScriptSite *pSelf)
{
    return S_OK;
}
        
/* More uninteresting events. */
static HRESULT STDMETHODCALLTYPE
IActiveScriptSite_OnLeaveScript(IActiveScriptSite *pSelf)
{
    return S_OK;
}

/* Virtual table for our `IActiveScriptSite` implementation. */
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

/* Constructor for our `IActiveScriptSite` implementation. */
static HRESULT
ActiveScriptSite_CreateInstance(
    REFIID guidVtbl, void **ppv
) {
    HRESULT              hr;
    ActiveScriptSite    *pActiveScriptSite;

    /* Allocate memory */
    if (!(pActiveScriptSite = GlobalAlloc(GMEM_FIXED, sizeof(ActiveScriptSite))))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        /* Set the virtual function table and reference count. */
        pActiveScriptSite->lpVtbl = &ActiveScriptSiteVtbl;
        pActiveScriptSite->dwCount = 1;

        /* Create an instance of `ScriptContext`. */
        CoCreateInstance(&CLSID_ScriptContext, NULL, CLSCTX_INPROC_SERVER,
                &IID_IDispatch, (LPVOID*)&pActiveScriptSite->pScriptContext);

        /* Increment the library lock count. */
        InterlockedIncrement(pdwLockCount);

        /* Now use QueryInterface to set the caller's result pointer.
         * QueryInterface will also check that request interface is the one
         * supported by our Verity browser helper object. */
        hr = ActiveScriptSiteVtbl.QueryInterface(
                (IActiveScriptSite*) pActiveScriptSite, guidVtbl, ppv);

        /* If we had a problem above, then the reference count is still
         * one, so decrementing it will free the memory we just allocated,
         * otherwise, it is two and decrementing it makes it as it should
         * be. */
        ActiveScriptSiteVtbl.Release((IActiveScriptSite*) pActiveScriptSite);

        /* */
    }

    return hr;
}

/* Register our factory with the DLL entry point that serves up factories. */
HRESULT
ActiveScriptSite_CreateFactory()
{
    return GenericFactory_CreateFactory(&CLSID_ActiveScriptSite,
            ActiveScriptSite_CreateInstance, GenericFactory_GenericFinalizer, &pdwLockCount);
}
