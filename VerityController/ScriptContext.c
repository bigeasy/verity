#include "stdafx.h"

#include "Log.h"
#include "GenericFactory.h"
#include "ComponentObjectModel.h"
#include "ScriptContext.h"

// {7CD57D42-C553-4B82-A52A-513082F57EE0}
// DEFINE_GUID(CLSID_ScriptContext,
// 0x7cd57d42, 0xc553, 0x4b82, 0xa5, 0x2a, 0x51, 0x30, 0x82, 0xf5, 0x7e, 0xe0);

// {7CD57D42-C553-4B82-A52A-513082F57EE0}
const GUID CLSID_ScriptContext = 
{ 0x7cd57d42, 0xc553, 0x4b82,
    { 0xa5, 0x2a, 0x51, 0x30, 0x82, 0xf5, 0x7e, 0xe0 } };

// {06ADED5A-6532-4112-96EC-22FD8BA69BCF}
static const GUID IID_IScriptContext = 
{ 0x6aded5a, 0x6532, 0x4112, { 0x96, 0xec, 0x22, 0xfd, 0x8b, 0xa6, 0x9b, 0xcf } };


// {9753946F-58D1-46DC-BAB1-321C00ABD037}
static const GUID CLSID_TypeLibrary = 
{ 0x9753946f, 0x58d1, 0x46dc,
    { 0xba, 0xb1, 0x32, 0x1c, 0x0, 0xab, 0xd0, 0x37 } };

struct InjectionMap
{
    CRITICAL_SECTION csInjections;
    BSTR bstrInjections;
};
static struct InjectionMap InjectionMap;

static DWORD *pdwLockCount;

static REFIID ariidImplemented[] =
{
    &IID_IUnknown,
    &IID_IDispatch,
    &IID_IScriptContext,
    NULL
};


REFERENCE_COUNT(IScriptContext, ScriptContext, pdwLockCount, NO_DESTRUCTOR)

static HRESULT STDMETHODCALLTYPE
IScriptContext_QueryInterface(IScriptContext *pSelf, REFIID riid, void **ppv)
{
    ScriptContext *pScriptContext = (ScriptContext*) pSelf;
    if (IsIIDImplemented(riid, ariidImplemented))
    {
        *ppv = pSelf;
        pSelf->lpVtbl->AddRef(pSelf);
        return S_OK;
    }
    else if (IsEqualIID(riid, &IID_IProvideMultipleClassInfo))
    {
        *ppv = &pScriptContext->PMCI;
        pSelf->lpVtbl->AddRef(pSelf);
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

/* Does not provide an actual count. Return a value of `1` through the integer
 * pointer to indicate that type info is provided. (Does not provide an actual
 * count](http://msdn.microsoft.com/en-us/library/aa910538.aspx) of any kind.
 */
static ULONG STDMETHODCALLTYPE
IScriptContext_GetTypeInfoCount(IScriptContext *pSelf, UINT *pctinfo)
{
    *pctinfo = 1;
    return 0;
}

static LPTYPEINFO TypeInfo;

static HRESULT loadTypeInfo()
{
    HRESULT hr;
    LPTYPELIB pTLib;
    BSTR rgNames[128];
    UINT cNames = 128, cActualNames;

    if (!(hr = LoadRegTypeLib(&CLSID_TypeLibrary, 1, 0, 0, &pTLib)))
    {
        hr = pTLib->lpVtbl->GetTypeInfoOfGuid(pTLib,
            &IID_IScriptContext, &TypeInfo);
        /* Release the temporary type info now owned by the default
         * type info. */
        pTLib->lpVtbl->Release(pTLib);
    }
    TypeInfo->lpVtbl->GetNames(TypeInfo, 1, rgNames, cNames, &cActualNames);
    return hr;
}

/* Type info is loaded from out type definition. */
static ULONG STDMETHODCALLTYPE
IScriptContext_GetTypeInfo (IScriptContext *pSelf, UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
{
    HRESULT hr;

    *ppTInfo = NULL;

    if (iTInfo)
    {
        hr = ResultFromScode(DISP_E_BADINDEX);
    }
    else
    {
        if (!TypeInfo)
        {
            hr = loadTypeInfo();
        }

        /* Increment the reference count to the type library we've loaded,
         * because the caller will release it once. */ 
        if (TypeInfo)
        {
            TypeInfo->lpVtbl->AddRef(TypeInfo);
        }
    }

    return hr;
}

static ULONG STDMETHODCALLTYPE
IScriptContext_GetIDsOfNames (IScriptContext *pSelf, REFIID refiid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HRESULT hr;

    if (!TypeInfo && (hr = loadTypeInfo()))
    {
        return hr; 
    }

    return DispGetIDsOfNames(TypeInfo, rgszNames, cNames, rgDispId);
}

static ULONG STDMETHODCALLTYPE
IScriptContext_Invoke (IScriptContext *pSelf, DISPID dispIdNumber, REFIID refiid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT hr;
    VARIANT vResult;

    VariantInit(&vResult);

    if (!IsEqualIID(refiid, &IID_NULL))
    {
        return DISP_E_UNKNOWNINTERFACE;
    }

    if (!TypeInfo && (hr = loadTypeInfo()))
    {
        return hr;
    }

    return DispInvoke(pSelf, TypeInfo, dispIdNumber, wFlags, pDispParams, pVarResult, pExcepInfo, puArgErr);
}

static HRESULT STDMETHODCALLTYPE
IScriptContext_SetURL (IScriptContext *pSelf, BSTR bstrURL)
{
    ScriptContext *pScriptContext = (ScriptContext *) pSelf;
    Log(L"SetURL %s\n", bstrURL);
    if (pScriptContext->bstrURL)
    {
        SysFreeString(pScriptContext->bstrURL);
    }
    if (bstrURL)
    {
        pScriptContext->bstrURL = SysAllocString((OLECHAR*) bstrURL);
    }
    else
    {
        pScriptContext->bstrURL = NULL;
    }
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IScriptContext_GetURL (IScriptContext *pSelf, BSTR *pbstrURL)
{
    ScriptContext *pScriptContext = (ScriptContext *) pSelf;
    if (pScriptContext->bstrURL)
    {
        *pbstrURL = SysAllocString((OLECHAR*) pScriptContext->bstrURL);
    }
    else
    {
        *pbstrURL = NULL;
    }
    Log(L"GetURL %s\n", *pbstrURL);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IScriptContext_SetInjections (IScriptContext *pSelf, BSTR bstrInjections)
{
    Log(L"SetInjections %s\n", bstrInjections);
    EnterCriticalSection(&InjectionMap.csInjections);
    if (InjectionMap.bstrInjections)
    {
        SysFreeString(InjectionMap.bstrInjections);
    }
    if (bstrInjections)
    {
        InjectionMap.bstrInjections = SysAllocString((OLECHAR*) bstrInjections);
    }
    else
    {
        InjectionMap.bstrInjections = NULL;
    }
    LeaveCriticalSection(&InjectionMap.csInjections);
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IScriptContext_GetInjections (IScriptContext *pSelf, BSTR *pbstrInjections)
{
    EnterCriticalSection(&InjectionMap.csInjections);
    if (InjectionMap.bstrInjections)
    {
        *pbstrInjections = SysAllocString((OLECHAR*) InjectionMap.bstrInjections);
    }
    else
    {
        *pbstrInjections = NULL;
    }
    LeaveCriticalSection(&InjectionMap.csInjections);
    Log(L"GetInjections %s\n", *pbstrInjections);
    return S_OK;
}

/* Testing with a bogus object shows that the caller will decrement the
 * reference once, without incrementing it, so if we do not increase the
 * reference count, the script engine will send the reference count to zero,
 * thereby releasing the XHR. */
static HRESULT STDMETHODCALLTYPE
IScriptContext_CreateXHR (IScriptContext *pSelf, IDispatch** ppIDispatch)
{
    Log(L"CreateXHR.\n");
    return CoCreateInstance(&CLSID_XMLHTTPRequest, NULL, CLSCTX_INPROC_SERVER,
        &IID_IDispatch, ppIDispatch);
}

static HRESULT STDMETHODCALLTYPE
IScriptContext_Injector (IScriptContext *pSelf, BSTR bstrInjector)
{
    Log(L"Injector.\n");
    return S_OK;
}

static HRESULT STDMETHODCALLTYPE
IScriptContext_CreateObservable(IScriptContext *pSelf, IDispatch** ppIDispatch)
{
    return GenericFactory_CreateInstance(L"{3C8C461C-5543-468A-A0D2-334C6B6E54E8}", &IID_IDispatch, ppIDispatch);
}

IScriptContextVtbl ScriptContextVtbl =
{
    IScriptContext_QueryInterface,
    IScriptContext_AddRef,
    IScriptContext_Release,
    IScriptContext_GetTypeInfoCount,
    IScriptContext_GetTypeInfo,
    IScriptContext_GetIDsOfNames,
    IScriptContext_Invoke,
    IScriptContext_SetURL,
    IScriptContext_GetURL,
    IScriptContext_SetInjections,
    IScriptContext_GetInjections,
    IScriptContext_CreateXHR,
    IScriptContext_Injector,
    IScriptContext_CreateObservable
};

static HRESULT STDMETHODCALLTYPE
IProvideMultipleClassInfo_QueryInterface(IProvideMultipleClassInfo *pSelf, REFIID riid, void **ppv)
{
    ProvideMultipleClassInfo *pPMCI = (ProvideMultipleClassInfo*) pSelf;
    return IScriptContext_QueryInterface(pPMCI->pScriptContext, riid, ppv);
}

static ULONG STDMETHODCALLTYPE
IProvideMultipleClassInfo_AddRef(IProvideMultipleClassInfo *pSelf)
{
    ProvideMultipleClassInfo *pPMCI = (ProvideMultipleClassInfo*) pSelf;
    return IScriptContext_AddRef(pPMCI->pScriptContext);
}

static ULONG STDMETHODCALLTYPE
IProvideMultipleClassInfo_Release(IProvideMultipleClassInfo *pSelf)
{
    ProvideMultipleClassInfo *pPMCI = (ProvideMultipleClassInfo*) pSelf;
    return IScriptContext_Release(pPMCI->pScriptContext);
}

static HRESULT STDMETHODCALLTYPE
IProvideMultipleClassInfo_GetClassInfo (IProvideMultipleClassInfo *pSelf, ITypeInfo **ppTI)
{
    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IProvideMultipleClassInfo_GetGUID (IProvideMultipleClassInfo *pSelf, DWORD dwGuidKind, GUID *pGUID)
{
    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IProvideMultipleClassInfo_GetMultiTypeInfoCount (IProvideMultipleClassInfo *pSelf, ULONG *pcti)
{
    return S_OK;
}
        
static HRESULT STDMETHODCALLTYPE
IProvideMultipleClassInfo_GetInfoOfIndex (IProvideMultipleClassInfo * pSelf, ULONG iti, DWORD dwFlags,
        ITypeInfo **pptiCoClass, DWORD *pdwTIFlags, ULONG *pcdispidReserved,
        IID *piidPrimary, IID *piidSource)
{
    return S_OK;
}

IProvideMultipleClassInfoVtbl ProvideMultipleClassInfoVtbl =
{
    IProvideMultipleClassInfo_QueryInterface,
    IProvideMultipleClassInfo_AddRef,
    IProvideMultipleClassInfo_Release,
    IProvideMultipleClassInfo_GetClassInfo,
    IProvideMultipleClassInfo_GetGUID,
    IProvideMultipleClassInfo_GetMultiTypeInfoCount,
    IProvideMultipleClassInfo_GetInfoOfIndex
};

// Create an instance of the Verity browser helper object.
static HRESULT
ScriptContext_CreateInstance(
    REFIID guidVtbl, void **ppv
) {
    HRESULT              hr;
    ScriptContext       *pScriptContext;

    // Allocate the memory for the Verity browser helper object.
    if (!(pScriptContext = GlobalAlloc(GMEM_FIXED, sizeof(ScriptContext))))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        // Set the virtual function table and reference count.
        pScriptContext->lpVtbl = &ScriptContextVtbl;
        pScriptContext->PMCI.lpVtbl = &ProvideMultipleClassInfoVtbl;
        pScriptContext->PMCI.pScriptContext = (IScriptContext*) pScriptContext;
        pScriptContext->dwCount = 1;
        pScriptContext->bstrURL = NULL;

        // Increment the library lock count.
        InterlockedIncrement(pdwLockCount);

        // Now use QueryInterface to set the caller's result pointer.
        // QueryInterface will also check that request interface is the
        // one supported by our Verity browser helper object.
        hr = ScriptContextVtbl.QueryInterface(
                (IScriptContext*) pScriptContext, guidVtbl, ppv);

        // If we had a problem above, then the reference count is still
        // one, so decrementing it will free the memory we just allocated,
        // otherwise, it is two and decrementing it makes it as it should
        // be.
        ScriptContextVtbl.Release((IScriptContext*) pScriptContext);

        *ppv = pScriptContext;
    }

    Log(_T("Allocated: %d\n"), hr);
    return hr;
}

void
ScriptContext_Finalize()
{
    EnterCriticalSection(&InjectionMap.csInjections);
    if (InjectionMap.bstrInjections)
    {
        SysFreeString(InjectionMap.bstrInjections);
    }
    LeaveCriticalSection(&InjectionMap.csInjections);
    DeleteCriticalSection(&InjectionMap.csInjections);
}

HRESULT
ScriptContext_CreateFactory()
{
    InitializeCriticalSection(&InjectionMap.csInjections);
    return GenericFactory_CreateFactory(&CLSID_ScriptContext, ScriptContext_CreateInstance, ScriptContext_Finalize, &pdwLockCount);
}
