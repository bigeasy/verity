
typedef struct ActiveScriptSite
{
    const IActiveScriptSiteVtbl *lpVtbl;
    DWORD dwCount;
    ScriptContext *pScriptContext;
}
ActiveScriptSite;
