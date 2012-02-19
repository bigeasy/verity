#include "stdafx.h"
#include "ComponentObjectModel.h"
#include "GenericFactory.h"

static DWORD *pdwLockCount;

// {3C8C461C-5543-468A-A0D2-334C6B6E54E8}
const GUID CLSID_Observable = 
{ 0x3c8c461c, 0x5543, 0x468a, { 0xa0, 0xd2, 0x33, 0x4c, 0x6b, 0x6e, 0x54, 0xe8 } };


static REFIID ariidImplemented[] =
{
    &IID_IUnknown,
    &IID_IDispatch,
    NULL
};

typedef struct Observable
{
    const IDispatchVtbl *lpVtbl;
    DWORD dwCount;
}
Observable;

static HRESULT STDMETHODCALLTYPE
IDispatch_QueryInterface(IDispatch *pSelf, REFIID riid, void **ppv)
{
    if (IsIIDImplemented(riid, ariidImplemented))
    {
        *ppv = pSelf;
        pSelf->lpVtbl->AddRef(pSelf);
        return S_OK;
    }
    else
    {
        *ppv = NULL;
        return E_NOINTERFACE;
    }
}

static ULONG STDMETHODCALLTYPE
IDispatch_AddRef(IDispatch *pSelf)
{
    Observable *pObservable = (Observable *) pSelf;
    return ++pObservable->dwCount;
}

static ULONG STDMETHODCALLTYPE
IDispatch_Release(IDispatch *pSelf)
{
    Observable *pObservable = (Observable *) pSelf;
    DWORD dwCount;
    if ((dwCount = --pObservable->dwCount) == 0)
    {
        GlobalFree(pObservable);
    }
    return dwCount;
}

static ULONG STDMETHODCALLTYPE
IDispatch_GetTypeInfoCount(IDispatch *pSelf, UINT *pctinfo)
{
    *pctinfo = 0;
    return 0;
}

/* Type info is loaded from out type definition. */
static ULONG STDMETHODCALLTYPE
IDispatch_GetTypeInfo (IDispatch *pSelf, UINT iTInfo, LCID lcid, ITypeInfo ** ppTInfo)
{
    HRESULT hr = S_OK;
    /*
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
        if (TypeInfo)
        {
            TypeInfo->lpVtbl->AddRef(TypeInfo);
        }
    }
    */
    return hr;
}

static ULONG STDMETHODCALLTYPE
IDispatch_GetIDsOfNames (IDispatch *pSelf, REFIID refiid, LPOLESTR *rgszNames,
        UINT cNames, LCID lcid, DISPID *rgDispId)
{
    HRESULT hr = S_OK;
    /*
    if (!TypeInfo && (hr = loadTypeInfo()))
    {
        return hr; 
    }

    return DispGetIDsOfNames(TypeInfo, rgszNames, cNames, rgDispId);
    */
    return hr;
}

static ULONG STDMETHODCALLTYPE
IDispatch_Invoke (IDispatch *pSelf, DISPID dispIdNumber, REFIID refiid,
        LCID lcid, WORD wFlags, DISPPARAMS *pDispParams, VARIANT *pVarResult,
        EXCEPINFO *pExcepInfo, UINT *puArgErr)
{
    HRESULT hr = S_OK;
    /*
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
    */
    return hr;
}

IDispatchVtbl ObservableVtbl =
{
    IDispatch_QueryInterface,
    IDispatch_AddRef,
    IDispatch_Release,
    IDispatch_GetTypeInfoCount,
    IDispatch_GetTypeInfo,
    IDispatch_GetIDsOfNames,
    IDispatch_Invoke
};

static HRESULT
Observable_CreateInstance(
    REFIID guidVtbl, void **ppv
) {
    HRESULT              hr;
    Observable          *pObservable;
    
    if (!(pObservable = GlobalAlloc(GMEM_FIXED, sizeof(Observable))))
    {
        hr = E_OUTOFMEMORY;
    }
    else
    {
        pObservable->lpVtbl = &ObservableVtbl;
        pObservable->dwCount = 1;

        InterlockedIncrement(pdwLockCount);

        hr = ObservableVtbl.QueryInterface((IDispatch*) pObservable, guidVtbl, ppv);

        ObservableVtbl.Release((IDispatch*) pObservable);

        *ppv = pObservable;
    }
    return hr;
}

HRESULT
Observable_CreateFactory()
{
    return GenericFactory_CreateFactory(&CLSID_Observable, Observable_CreateInstance, &pdwLockCount);
}
