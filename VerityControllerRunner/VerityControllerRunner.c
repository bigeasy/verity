// VerityControllerRunner.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"


int _tmain(int argc, _TCHAR* argv[])
{
    HRESULT hr;
    GUID guidIE7Pro, guidVerityController;
    IObjectWithSite* pIE7Pro, *pVerityController;

    CoInitialize(NULL);

    hr = CLSIDFromString(L"{00011268-E188-40DF-A514-835FCD78B1BF}", &guidIE7Pro);
    if (hr != S_OK)
    {
        return 1;
    }
    hr = CoCreateInstance(&guidIE7Pro, NULL, CLSCTX_INPROC_SERVER, &IID_IObjectWithSite, &pIE7Pro);
    if (hr != S_OK)
    {
        return 1;
    }
    pIE7Pro->lpVtbl->Release(pIE7Pro);
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
	return 0;
}

