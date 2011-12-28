#pragma once
// The Verity browser helper object data.
typedef struct SVerityController {
    const IObjectWithSiteVtbl* lpVtbl;
    DWORD               dwCount;
    DWORD*              pdwLockCount;
    IConnectionPoint*   pSiteConnectionPoint;
    DWORD               dwSiteAdviseCookie;
    IUnknown*           pSite;
} IVerityController;
