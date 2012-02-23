BOOLEAN
IsIIDImplemented(REFIID riid, REFIID ariidImplemented[]);

void NO_DESTRUCTOR(LPVOID pObject);

void COM_Destroying(void *pClass, char *name);

#define QUERY_INTERFACE(Interface, iids)                    \
static HRESULT STDMETHODCALLTYPE                            \
Interface ## _QueryInterface                                  \
    ( Interface *pSelf, REFIID riid, void **ppv)     \
{                                                           \
    if (IsIIDImplemented(riid, iids ))                      \
    {                                                       \
        *ppv = pSelf;                                       \
        pSelf->lpVtbl->AddRef(pSelf);                       \
        return S_OK;                                        \
    }                                                       \
    else                                                    \
    {                                                       \
        Log(L"NO INTERFACE\n"); \
        *ppv = NULL;                                        \
        return E_NOINTERFACE;                               \
    }                                                       \
}

#define REFERENCE_COUNT(Interface, Class, LockCount, Destructor)        \
static ULONG STDMETHODCALLTYPE                              \
Interface ## _AddRef( Interface *pSelf)                        \
{                                                           \
    Class *pClass = ( Class *) pSelf;                         \
    return ++pClass->dwCount;                               \
}                                                           \
                                                     \
static ULONG STDMETHODCALLTYPE                              \
Interface ## _Release( Interface *pSelf)                       \
{                                                           \
    Class *pClass = ( Class *) pSelf;                         \
    DWORD dwCount;                         \
    if ((dwCount = --pClass->dwCount) == 0)                        \
    {                                                       \
        COM_Destroying( pClass, #Interface );                \
        Destructor(pClass);                                 \
        InterlockedDecrement(LockCount);                    \
        GlobalFree(pClass);                                 \
    }                                                       \
    return dwCount;                                         \
}
