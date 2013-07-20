
typedef struct ActiveScriptSite
{
    const IActiveScriptSiteVtbl *lpVtbl;
    DWORD dwCount;
    IScriptContext *pScriptContext;
}
ActiveScriptSite;
