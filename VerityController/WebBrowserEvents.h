// At this point, I'm assuming that this is a singleton.
typedef struct CWebBrowserEvents {
    const IDispatchVtbl *lpVtbl;
} IWebBrowserEvents;

IWebBrowserEvents WebBrowserEvents;
IActiveScriptSite ActiveScriptSite;
