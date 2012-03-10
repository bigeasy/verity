// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "GenericFactory.h"
#include "dllmain.h"

HRESULT ObjectWithSite_CreateFactory();
HRESULT ScriptContext_CreateFactory();
HRESULT ActiveScriptSite_CreateFactory();
HRESULT OnDocument_CreateFactory(HMODULE hModule);
HRESULT Observable_CreateFactory();

// Functions for logging. The debugger is difficult with Internet Explorer. It
// runs slowly and appears to cause a lot of problems for IE. We've got a 
// simple sprintf based logger that writes a specific file on the filesystem.
//
// Logging is disabled in a release build.

#ifdef _DEBUG
// Generous buffer size for formatting error messages.
#define LOG_BUFFER_LENGTH (1024 * 64) 

// Handle to our log file.
static HFILE hfLog = 0;

// Convert wide characters to multibyte characters and write to the log file.
// This function was split out into a separate function because the Log
// function was getting cluttered.
static void
WriteWideCharacterToLog(const TCHAR* pwsz)
{
    DWORD       dwWritten;
    size_t      converted;
    char        psz[LOG_BUFFER_LENGTH];
    mbstate_t   mbstate;
    wcsrtombs_s(&converted, psz, LOG_BUFFER_LENGTH - 1, &pwsz,
                wcslen(pwsz) * sizeof(TCHAR), &mbstate);
    WriteFile((HANDLE) hfLog, psz, strlen(psz), &dwWritten, NULL);
}

// Format a logging message and write it to the log file. Always remember to use
// a cousin of `vsprintf`, not `sprintf` itself. Read a man page. We use
// convoluted wide character flavored `vsprintf` here.
void
Log(const TCHAR* format, ...)
{
    va_list     args;
    TCHAR       pwsz[LOG_BUFFER_LENGTH];
    size_t      sLength = LOG_BUFFER_LENGTH;
    va_start(args,format);
    _vsnwprintf_s(pwsz, LOG_BUFFER_LENGTH,
                  LOG_BUFFER_LENGTH - 1, format, args);
    va_end(args);
    WriteWideCharacterToLog(pwsz);
}

// Open a specific log file in our installation directory.
static void
OpenLog()
{
    OFSTRUCT ofstruct;
    if (! hfLog) {
        hfLog = OpenFile("C:\\codearea\\touched.txt", &ofstruct, OF_CREATE);
    }
}
#else
#define OpenLog() void(0)
#define Log(format, ...) void(0)
#endif

static HMODULE hDllModule = 0;

// Library entry point. When you register a browser helper object, it will be
// looaded by both IE and Windows Explorer. We want nothing to do with Windows
// Explorer, so we refuse to load when it calls us.
BOOL APIENTRY DllMain(
    HMODULE hModule, DWORD  dwReason, LPVOID lpReserved
) {
    TCHAR pszLoader[MAX_PATH];

    OpenLog();

	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
        hDllModule = hModule;
        DllRegisterServer();
        GetModuleFileName(NULL, pszLoader, MAX_PATH);
        _tcslwr_s(pszLoader, MAX_PATH);
        if (_tcsstr(pszLoader, _T("explorer.exe")))
        {
            return FALSE;
        }
        ActiveScriptSite_CreateFactory();
        OnDocument_CreateFactory(hModule);
        ObjectWithSite_CreateFactory();
        ScriptContext_CreateFactory();
        Observable_CreateFactory();
        hDllModule = hModule;
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
        break;
	case DLL_PROCESS_DETACH:
		break;
	}

	return TRUE;
}

// DRY up the creation of a Windows Registry key.
static LONG
CreateKey(HKEY hKey, LPTSTR lpzPath, HKEY* hkResult)
{
    DWORD dwDisposition;
    return RegCreateKeyEx(hKey, lpzPath, 0, NULL, REG_OPTION_NON_VOLATILE,
        KEY_WRITE, NULL, hkResult, &dwDisposition);
}

// DRY up the setting of a Windows Registry value.
static LONG
SetString(HKEY hKey, LPTSTR lpzPath, LPTSTR lpzValue)
{
    return RegSetValueEx(hKey, lpzPath, 0, REG_SZ,
                         (CONST BYTE *) lpzValue,
                         lstrlen(lpzValue) * sizeof(TCHAR));
}

#define CLSID_VERITY _T("{0308456E-B8AA-4267-9A3E-09010E0A6754}")

HRESULT __stdcall AddClass(LPTSTR lpzClassKey, LPTSTR lpzControlName, LPTSTR lpzLibrary)
{
    HKEY hkClass = NULL, hkInproc = NULL;
    HRESULT hr;
    if (hr = CreateKey(HKEY_CLASSES_ROOT, lpzClassKey, &hkClass)) goto exit;
    if (hr = SetString(hkClass, NULL, lpzControlName)) goto exit;
    if (hr = CreateKey(hkClass, _T("InprocServer32"), &hkInproc)) goto exit;
    if (hr = SetString(hkInproc, NULL, lpzLibrary)) goto exit;
    if (hr = SetString(hkInproc, _T("TreadingModel"), _T("Apartment"))) goto exit;
exit:
    if (hkClass)
    {
        RegCloseKey(hkClass);
    }
    if (hkInproc)
    {
        RegCloseKey(hkInproc);
    }
    return hr;
}

// Register this browser helper object. This method is invoked by regsrv32.exe
// during installation to set the Windows Registry keys that tell Internet
// Explorer that we are a Browser Helper Object server.
HRESULT __stdcall DllRegisterServer(void)
{
    HKEY hkBHO = NULL, hkVerity = NULL;
    LPTYPELIB pTypeLib = NULL;
    HRESULT hr;

    WCHAR lpzLibrary[MAX_PATH];
    LPTSTR lpzControlName   = _T("Verity Controller"); 
    LPTSTR lpzClassKey      = _T("CLSID\\") CLSID_VERITY;
    LPTSTR lpzContextKey    = _T("CLSID\\{7CD57D42-C553-4B82-A52A-513082F57EE0}");
    LPTSTR lpzOnDocumentKey = _T("CLSID\\{42939237-42F0-4E3F-818E-FA63E4EB5A82}");
    LPTSTR lpzBHOKey        = _T("Software\\Microsoft\\Windows\\CurrentVersion\\")
                              _T("Explorer\\Browser Helper Objects");

    if (!GetModuleFileNameW(hDllModule, lpzLibrary, MAX_PATH)) {
        hr = E_FAIL;
        goto exit;
    }
    if (hr = AddClass(L"CLSID\\" CLSID_VERITY, L"Verity Controller", lpzLibrary))
    {
        goto exit;
    }

    if (hr = AddClass(L"CLSID\\{73C6AB50-2BEE-4DC0-AB1C-910C255D1C23}", L"Verity ActiveScriptSite", lpzLibrary))
    {
        goto exit;
    }
    if (hr = AddClass(L"CLSID\\{42939237-42F0-4E3F-818E-FA63E4EB5A82}", L"Verity OnDocument Handler", lpzLibrary))
    {
        goto exit;
    }

    if (hr = AddClass(L"CLSID\\{7CD57D42-C553-4B82-A52A-513082F57EE0}", L"Verity Script Context", lpzLibrary))
    {
        goto exit;
    }

    if (hr = CreateKey(HKEY_LOCAL_MACHINE, lpzBHOKey, &hkBHO)) goto exit;
    if (hr = CreateKey(hkBHO, CLSID_VERITY, &hkVerity)) goto exit;
    if (hr = SetString(hkVerity, NULL, lpzControlName)) goto exit;

    if (hr = LoadTypeLib(lpzLibrary, &pTypeLib)) goto exit;
    if (hr = RegisterTypeLib(pTypeLib, lpzLibrary, NULL)) goto exit;
exit:
    if (hkBHO)
    {
        RegCloseKey(hkBHO);
    }
    if (hkVerity)
    {
        RegCloseKey(hkVerity);
    }
    if (pTypeLib)
    {
        pTypeLib->lpVtbl->Release(pTypeLib);
    }
    return hr;
}

// TODO: Remove Browser Helper Object Windows Registry keys and the COM object
// keys for the object.
HRESULT __stdcall DllUnregisterServer(void)
{
    return S_OK;
}

static int dwFactoryCount = 0;
typedef struct factory {
    const GUID *clsid;
    IClassFactory *pFactory;
    DWORD dwLockCount;
    GenericFactory_Finalizer finalizer;
} IFactoryMapping;
static IFactoryMapping afmRegisteredFactories[5];

DWORD*
RegisterObjectFactory(const GUID *clsid, IClassFactory *pFactory, GenericFactory_Finalizer finalizer)
{
    IFactoryMapping *fm;
    fm = &afmRegisteredFactories[dwFactoryCount++];
    fm->clsid = clsid;
    fm->pFactory = pFactory;
    fm->finalizer = finalizer;
    return &fm->dwLockCount;
}

HRESULT
GenericFactory_GetFactory(REFCLSID guidObject, REFIID guidFactory, LPVOID *ppvFactory)
{
    IClassFactory *pFactory;
    int i;
    for (i = 0; i < dwFactoryCount; i++)
    {
        if (IsEqualCLSID(guidObject, afmRegisteredFactories[i].clsid))
        {
            pFactory = afmRegisteredFactories[i].pFactory;
            return pFactory->lpVtbl->
                QueryInterface(pFactory, guidFactory, ppvFactory);
        }
    }

    *ppvFactory = 0;
    return CLASS_E_CLASSNOTAVAILABLE;
}
// If we're asked for Verity browser helper object, oblige by returning the
// factory that makes them. Otherwise, set the factory result to NULL and flag
// an error.
HRESULT __stdcall
DllGetClassObject(REFCLSID guidObject, REFIID guidFactory, LPVOID *ppvFactory)
{
    return GenericFactory_GetFactory(guidObject, guidFactory, ppvFactory);
}

HRESULT
GenericFactory_CreateInstance(LPCTSTR clsid, REFIID riid, LPVOID *pObject)
{
    HRESULT hr;
    IClassFactory *pClassFactory;
    GUID guid;
    hr = CLSIDFromString(clsid, &guid);
    if (hr != S_OK)
    {
        return hr;
    }
    hr = GenericFactory_GetFactory(&guid, &IID_IClassFactory, (LPVOID*) &pClassFactory);
    if (hr != S_OK)
    {
        return hr;
    }
    hr = pClassFactory->lpVtbl->CreateInstance(pClassFactory, NULL, riid, pObject);
    pClassFactory->lpVtbl->Release(pClassFactory);
    return hr;
}

// Don't unload this library if there are outstanding COM objects or explicit
// locks on the library.
HRESULT __stdcall DllCanUnloadNow(void)
{
    int i;
    for (i = 0; i < dwFactoryCount; i++)
    {
        if (afmRegisteredFactories[i].dwLockCount)
        {
            return S_FALSE;
        }
    }
    return S_OK;
}
