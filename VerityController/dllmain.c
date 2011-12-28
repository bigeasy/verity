// dllmain.cpp : Defines the entry point for the DLL application.
#include "stdafx.h"
#include "VerityControllerFactory.h"

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

// Format a logging message and write it to the log file.
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
        Log(_T("Log was not open.\n"));
    }
}
#else
#define OpenLog() void(0)
#define Log(format, ...) void(0)
#endif

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
        GetModuleFileName(NULL, pszLoader, MAX_PATH);
        Log(_T("Process Attaching: %s\n"), pszLoader);
        _tcslwr_s(pszLoader, MAX_PATH);
        if (_tcsstr(pszLoader, _T("explorer.exe"))) {
            return FALSE;
        }
        break;
	case DLL_THREAD_ATTACH:
	case DLL_THREAD_DETACH:
        break;
	case DLL_PROCESS_DETACH:
        Log(_T("Process Detaching.\n"), pszLoader);
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

// Register this browser helper object. This method is invoked by regsrv32.exe
// during installation to set the Windows Registry keys that tell Internet
// Explorer that we are a Browser Helper Object server.
HRESULT __stdcall DllRegisterServer(void)
{
    HKEY hkClass, hkInproc, hkBHO, hkVerity;
    long lRet;
    LPTSTR lpzControlName = _T("Verity Controller"); 
    LPTSTR lpzLibrary     = _T("c:\\codearea\\git\\VerityController\\")
                            _T("Debug\\VerityController.dll");
    LPTSTR lpzClassKey    = _T("CLSID\\") CLSID_VERITY;
    LPTSTR lpzBHOKey      = _T("Software\\Microsoft\\Windows\\CurrentVersion\\")
                            _T("Explorer\\Browser Helper Objects");
    lRet = CreateKey(HKEY_CLASSES_ROOT, lpzClassKey, &hkClass);
    if (lRet != ERROR_SUCCESS) return SELFREG_E_CLASS;
    lRet = SetString(hkClass, NULL, lpzControlName);
    if (lRet != ERROR_SUCCESS) return SELFREG_E_CLASS;
    lRet = CreateKey(hkClass, _T("InprocServer32"), &hkInproc);
    if (lRet != ERROR_SUCCESS) return SELFREG_E_CLASS;
    lRet = SetString(hkInproc, NULL, lpzLibrary);
    if (lRet != ERROR_SUCCESS) return SELFREG_E_CLASS;
    lRet = SetString(hkInproc, _T("TreadingModel"), _T("Apartment"));
    if (lRet != ERROR_SUCCESS) return SELFREG_E_CLASS;
    lRet = CreateKey(HKEY_LOCAL_MACHINE, lpzBHOKey, &hkBHO);
    if (lRet != ERROR_SUCCESS) return SELFREG_E_CLASS;
    lRet = CreateKey(hkBHO, CLSID_VERITY, &hkVerity);
    if (lRet != ERROR_SUCCESS) return SELFREG_E_CLASS;
    lRet = SetString(hkVerity, NULL, lpzControlName);
    return S_OK;
}

// TODO: Remove Browser Helper Object Windows Registry keys and the COM object
// keys for the object.
HRESULT __stdcall DllUnregisterServer(void)
{
    return S_OK;
}

// If we're asked for Verity browser helper object, oblige by returning the
// factory that makes them. Otherwise, set the factory result to NULL and flag
// an error.
HRESULT __stdcall
DllGetClassObject(REFCLSID guidObject, REFIID guidFactory, LPVOID *ppvFactory)
{
    HRESULT  hr;
    if (IsEqualCLSID(guidObject, &CLSID_IVerityController))
    {
        hr = factory.lpVtbl->QueryInterface(
                (IClassFactory *) &factory, guidFactory, ppvFactory);
    }
    else
    {
        *ppvFactory = 0;
        hr = CLASS_E_CLASSNOTAVAILABLE;
    }

    return hr;
}

// Don't unload this library if there are outstanding COM objects or explicit
// locks on the library.
HRESULT __stdcall DllCanUnloadNow(void)
{
    HRESULT err = factory.dwLockCount ? S_FALSE : S_OK;
    Log(_T("Asking about unloading: %d\n"), S_OK);
    return err;
}
