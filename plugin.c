#include "vendor/mozilla/npfunctions.h"


struct plugin {
  NPNetscapeFuncs *browser;
  int log;
};

static struct plugin plugin;

struct verity {
};

void say(const char* format, ...) {
  char buffer[4096];
  va_list args;
  va_start(args,format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  write(plugin.log, buffer, strlen(buffer));
  fsync(plugin.log);
}

#pragma export on
NPError NP_Initialize(NPNetscapeFuncs *browser);
NPError NP_GetEntryPoints(NPPluginFuncs *class);
void OSCALL NP_Shutdown();
#pragma export off

NPError OSCALL NP_Initialize(NPNetscapeFuncs *browser) {
  plugin.browser = browser;
  plugin.log = open("/Users/alan/verity.log", O_WRONLY | O_APPEND | O_CREAT, 0644);
  say("NP_Initialize\n");
  return NPERR_NO_ERROR;
}

NPError NP_LOADDS Verity_New(NPMIMEType mime, NPP instance, uint16_t mode,
  int16_t argc, char* argn[], char* argv[], NPSavedData* saved) {
  return NPERR_NO_ERROR;
}

NPError NP_LOADDS Verity_Destroy(NPP instance, NPSavedData** save) {
  say("NPP_Destory\n");
  return NPERR_NO_ERROR;
}

NPError NP_LOADDS Verity_SetWindow(NPP instance, NPWindow* window) {
  say("NPP_SetWindow\n");
  return NPERR_NO_ERROR;
}

NPError NP_LOADDS Verity_NewStream(NPP instance, NPMIMEType type,
    NPStream *stream, NPBool seekable, uint16_t* stype) {
  say("NPP_NewStream\n");
  return NPERR_NO_ERROR;
}

NPError NP_LOADDS Verity_DestroyStream(NPP instance, NPStream* stream,
    NPReason reason) {
  return NPERR_NO_ERROR;
}

int32_t NP_LOADDS Verity_WriteReady(NPP instance, NPStream *stream) {
  return 0;
}

int32_t NP_LOADDS Verity_Write(NPP instance, NPStream *stream, int32_t offset,
    int32_t len, void *buffer) {
  return 0;
}

void NP_LOADDS Verity_StreamAsFile(NPP instance, NPStream *stream,
    const char *fname) {
}

void NP_LOADDS Verity_Print(NPP instance, NPPrint *print) {
}

int16_t NP_LOADDS Verity_HandleEvent(NPP instance, void *event) {
  return 0;
}

void NP_LOADDS Verity_URLNotify(NPP instance, const char *url,
    NPReason reason, void *data) {
}

NPError NP_LOADDS Verity_GetValue(NPP instance, NPPVariable variable,
    void *value) {
  return NPERR_NO_ERROR;
}

NPError NP_LOADDS Verity_SetValue(NPP instance, NPNVariable variable,
    void *value) {
  return NPERR_NO_ERROR;
}

NPBool NP_LOADDS Verity_GotFocus(NPP instance, NPFocusDirection direction) {
  say("NPP_GotFocus\n");
  return NPERR_NO_ERROR;
}

void NP_LOADDS Verity_LostFocus(NPP instance) {
  say("NPP_LostFocus\n");
}

void NP_LOADDS Verity_URLRedirectNotify(NPP instance, const char *url,
    int32_t status, void *data) {
}

NPError NP_LOADDS Verity_ClearSiteData(const char *site, uint64_t flags, uint64_t maxAge) {
  return NPERR_NO_ERROR;
}

char** NP_LOADDS Verity_GetSitesWithData() {
  return NULL;
}

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *class) {
  say("NP_GetEntryPoints\n");
  class->size = sizeof(NPPluginFuncs);
  class->version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
  class->newp = Verity_New;
  class->destroy = Verity_Destroy;
  class->setwindow = Verity_SetWindow;
  class->newstream = Verity_NewStream;
  class->destroystream = Verity_DestroyStream;
  class->writeready = Verity_WriteReady;
  class->write = Verity_Write;
  class->print = Verity_Print;
  class->urlnotify = Verity_URLNotify;
  class->getvalue = Verity_GetValue;
  class->setvalue = Verity_SetValue;
  class->gotfocus = Verity_GotFocus;
  class->lostfocus = Verity_LostFocus;
  class->urlredirectnotify = Verity_URLRedirectNotify;
  class->clearsitedata = Verity_ClearSiteData;
  class->getsiteswithdata = Verity_GetSitesWithData;
  return NPERR_NO_ERROR;
}

void OSCALL NP_Shutdown() {
  say("NP_Shutdown\n");
  close(plugin.log);
}

char* OSCALL NP_GetMIMEDescription() {
  return "application/mozilla-npruntime-scriptable-plugin:.foo:Scriptability";
}
