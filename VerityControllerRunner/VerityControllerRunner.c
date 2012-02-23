// VerityControllerRunner.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include <ExDispid.h>
/* Obtains a pointer to the factory singleton. */
static HRESULT STDMETHODCALLTYPE
QueryInterface(IDispatch *pSelf, REFIID guidVtbl, void **ppv)
{
    return NOERROR;
}

static ULONG STDMETHODCALLTYPE
AddRef(IDispatch *pSelf)
{
    return 1;
}

static ULONG STDMETHODCALLTYPE
Release(IDispatch *pSelf)
{
    return 1;
}

static HRESULT STDMETHODCALLTYPE
GetTypeInfoCount(   
    IDispatch * pSelf, UINT *pctinfo
) {
    *pctinfo = 0;
    return S_OK;
}

        
static HRESULT STDMETHODCALLTYPE
GetTypeInfo( 
    IDispatch * This, UINT iTInfo, LCID lcid, ITypeInfo **ppTInfo
) {
    return DISP_E_UNKNOWNNAME;
}
        
static HRESULT STDMETHODCALLTYPE
GetIDsOfNames(
    IDispatch * This,
    REFIID riid,
    LPOLESTR *rgszNames,
    UINT cNames,
    LCID lcid,
    DISPID *rgDispId
) {
    return DISP_E_UNKNOWNNAME;
}

static HRESULT STDMETHODCALLTYPE
Invoke(
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
    return S_OK;
}

static IDispatchVtbl IBogusDispatchVtbl = {
    QueryInterface,
    AddRef,
    Release,
    GetTypeInfoCount,
    GetTypeInfo,
    GetIDsOfNames,
    Invoke    
};

static IDispatch BogusDispatch = { &IBogusDispatchVtbl };

int _tmain(int argc, _TCHAR* argv[])
{
    HRESULT hr = -1073741819;
    GUID guidVerityController, guidOnDocument;
    IObjectWithSite *pVerityController;
    IDispatch *pOnDocument;
    VARIANT avargs[2];
    VARIANT vDispatch;
    DISPPARAMS params;

    CoInitialize(NULL);

    hr = CLSIDFromString(L"{0308456E-B8AA-4267-9A3E-09010E0A6754}", &guidVerityController);
    if (hr != S_OK)
    {
        return 1;
    }
    hr = CoCreateInstance(&guidVerityController, NULL, CLSCTX_INPROC_SERVER, &IID_IObjectWithSite, &pVerityController);
    if (hr != S_OK)
    {
        return 1;
    }
    pVerityController->lpVtbl->Release(pVerityController);

    hr = CLSIDFromString(L"{42939237-42F0-4E3F-818E-FA63E4EB5A82}", &guidOnDocument);
    if (hr != S_OK)
    {
        return 1;
    }
    hr = CoCreateInstance(&guidOnDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IDispatch, &pOnDocument);
    if (hr != S_OK)
    {
        return 1;
    }
    VariantInit(&vDispatch);
    vDispatch.vt = VT_DISPATCH;
    vDispatch.pdispVal = &BogusDispatch;
    VariantInit(&avargs[1]);
    avargs[1].vt = VT_VARIANT;
    avargs[1].pvarVal = &vDispatch;
    avargs[0].vt = VT_BSTR;
    avargs[0].bstrVal = SysAllocString(L"http://jekyllrb.com/");
    params.rgvarg = avargs;

    pOnDocument->lpVtbl->Invoke(pOnDocument, DISPID_DOCUMENTCOMPLETE, NULL, 0, 0, &params, NULL, NULL, NULL);

    SysFreeString(avargs[0].bstrVal);

    pOnDocument->lpVtbl->Release(pOnDocument);
	return 0;
}

