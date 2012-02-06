#include "vendor/mozilla/npfunctions.h"

struct plugin {
  NPNetscapeFuncs *browser;
};

static struct plugin plugin;

#pragma export on
NPError NP_Initialize(NPNetscapeFuncs *browser);
NPError NP_GetEntryPoints(NPPluginFuncs *class);
char* OSCALL NP_GetMIMEDescription();
void OSCALL NP_Shutdown();
#pragma export off

NPError OSCALL NP_Initialize(NPNetscapeFuncs *browser) {
  plugin.browser = browser;
  exit(1);
  return NPERR_NO_ERROR;
}

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *class) {
  return NPERR_NO_ERROR;
}

void OSCALL NP_Shutdown() { }

char* OSCALL NP_GetMIMEDescription() {
  return "application/mozilla-npruntime-scriptable-plugin:.foo:Scriptability";
}
