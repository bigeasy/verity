/* I can't seem to find an API or other known way to store a plugin's
 * configuration. Adobe Flash offers its configuration in a popup bar, but I
 * don't know know where that gets stored. */
#include "vendor/mozilla/npfunctions.h"
#include <dlfcn.h>
#include <pthread.h>
#include <fcntl.h>
#include <poll.h>
#include <curl/curl.h>
#include "vendor/attendant/attendant.h"
#include "vendor/attendant/eintr.h"

/* *Tell me now, Muses who have your homes on Olympus &mdash; for you are
 * goddesses, and are present, and know everything, while we hear only rumor and
 * know nothing.* &mdash; Homer */

/* Declare all of the methods provded by the browser. When the plugin is
 * initialized, we will copy the functions from the structure in the browser
 * into these static variables. We favor a lowered-case, underbared naming
 * convention. */
static NPN_GetURLProcPtr npn_get_url;
static NPN_PostURLProcPtr npn_post_url;
static NPN_RequestReadProcPtr npn_request_read;
static NPN_NewStreamProcPtr npn_new_stream;
static NPN_WriteProcPtr npn_write;
static NPN_DestroyStreamProcPtr npn_destroy_stream;
static NPN_StatusProcPtr npn_status;
static NPN_UserAgentProcPtr npn_user_agent;
static NPN_MemAllocProcPtr npn_mem_alloc;
static NPN_MemFreeProcPtr npn_mem_free;
static NPN_MemFlushProcPtr npn_mem_flush;
static NPN_ReloadPluginsProcPtr npn_reload_plugins;
static NPN_GetJavaEnvProcPtr npn_get_java_env;
static NPN_GetJavaPeerProcPtr npn_get_java_peer;
static NPN_GetURLNotifyProcPtr npn_get_url_notify;
static NPN_PostURLNotifyProcPtr npn_post_url_notify;
static NPN_GetValueProcPtr npn_get_value;
static NPN_SetValueProcPtr npn_set_value;
static NPN_InvalidateRectProcPtr npn_invalidate_rect;
static NPN_InvalidateRegionProcPtr npn_invalidate_region;
static NPN_ForceRedrawProcPtr npn_force_redraw;
static NPN_GetStringIdentifierProcPtr npn_get_string_identifier;
static NPN_GetStringIdentifiersProcPtr npn_get_string_identifiers;
static NPN_GetIntIdentifierProcPtr npn_get_int_identifier;
static NPN_IdentifierIsStringProcPtr npn_identifier_is_string;
static NPN_UTF8FromIdentifierProcPtr npn_utf8_from_indentifier;
static NPN_IntFromIdentifierProcPtr npn_int_from_identifier;
static NPN_CreateObjectProcPtr npn_create_object;
static NPN_RetainObjectProcPtr npn_retain_object;
static NPN_ReleaseObjectProcPtr npn_release_object;
static NPN_InvokeProcPtr npn_invoke;
static NPN_InvokeDefaultProcPtr npn_invoke_default;
static NPN_EvaluateProcPtr npn_evaluate;
static NPN_GetPropertyProcPtr npn_get_property;
static NPN_SetPropertyProcPtr npn_set_property;
static NPN_RemovePropertyProcPtr npn_remove_property;
static NPN_HasPropertyProcPtr npn_has_property;
static NPN_HasMethodProcPtr npn_has_method;
static NPN_ReleaseVariantValueProcPtr npn_release_variant_value;
static NPN_SetExceptionProcPtr npn_set_exception;
static NPN_PushPopupsEnabledStateProcPtr npn_push_popups_enabled_state;
static NPN_PopPopupsEnabledStateProcPtr npn_pop_popups_enabled_state;
static NPN_EnumerateProcPtr npn_enumerate;
static NPN_PluginThreadAsyncCallProcPtr npn_plugin_thread_async_call;
static NPN_ConstructProcPtr npn_construct;
static NPN_GetValueForURLPtr npn_get_value_for_url;
static NPN_SetValueForURLPtr npn_set_value_for_url;
static NPN_GetAuthenticationInfoPtr npn_get_authentication_info;
static NPN_ScheduleTimerPtr npn_schedule_timer;
static NPN_UnscheduleTimerPtr npn_unschedule_timer;
static NPN_PopUpContextMenuPtr npn_pop_up_context_menu;
static NPN_ConvertPointPtr npn_convert_point;
static NPN_HandleEventPtr npn_handle_event;
static NPN_UnfocusInstancePtr npn_unfocus_instance;
static NPN_URLRedirectResponsePtr npn_url_redirect_response;

/* Gather the plugin library state into a structure. */

/* &#9824; */
struct plugin {
  /* The file descriptor of the UNIX error log. */
  int log;
  /* The full path to the relay program. */
  char relay[PATH_MAX];
  /* The full path to the Cubby web server. */
  char cubby[PATH_MAX];
  /* The shutdown key for the Cubby web server. */
  char shutdown[256];
  /* The port of the cubby server. */
  int port;
  /* Used to perform the GET that shuts down the Cubby web server. */
  CURL *curl;
  /* Gaurd process variables referenced by both the stub functions running in
   * the plugin threads and the server process management threads. */
  pthread_mutex_t mutex;      
  /* Server process is running. */
  pthread_cond_t cond;  
/* &mdash; */
};

/* The one and only instance of the plugin library state. */
static struct plugin plugin;

/* Say something into the debugging log. */
void say(const char* format, ...) {
  char buffer[4096];
  va_list args;
  va_start(args,format);
  vsnprintf(buffer, sizeof(buffer), format, args);
  va_end(args);
  write(plugin.log, buffer, strlen(buffer));
  fsync(plugin.log);
}

/* Entry points for the plugin dynamic link library. Note that, the `main`
 * function that was once necessary for Mac OS no longer applies to OS X
 * browsers. It likes to confuse the compiler, a strange main. */

/* */
#pragma export on

/* Initialize the library with the browser functions, initialize the library. */
NPError NP_Initialize(NPNetscapeFuncs *browser);
/* Obtain the entry point funtions for the plugin. */
NPError NP_GetEntryPoints(NPPluginFuncs *class);
/* Shutdown the plugin library. */
void OSCALL NP_Shutdown();

/* */
#pragma export off

void starter(int restart) {
  char const *argv[] = { NULL };

  (void) pthread_mutex_lock(&plugin.mutex);
  plugin.port = 0;
  (void) pthread_mutex_unlock(&plugin.mutex);

  attendant.start(plugin.cubby, argv);
}

void connector(attendant__pipe_t in, attendant__pipe_t out) {
  /* Tolerance: 1024 is not a magic number. It is a kilobyte. When I see this, I
   * know what it means. It means that I'm reading output line by line, and no
   * line will ever be more that 64 characters, so a kilobyte is plenty.
   *
   * What good does it do to give this number a name? (Again, it has a name when
   * I read it; kilobyte.) Does it make it any easier to resolve the problem
   * that some day in the future, a line might be more than 80 characters? If
   * there is a bug introduced, because I've decided that, in the program we
   * launch here, instead of emitting two lines of terse strings, I'm going to
   * dump a megabyte of binary data, will a named number help me find the error
   * any faster? Or will it take longer, because to change the buffer size, I
   * have to search for the place where the name is defined? Or am I in for a
   * penny, in for a pound, and defining these in some distant build step, a
   * configuration file, our using a build tool?
   *
   * Interesting reflection on named numbers versus "magic" numbers, that it
   * made sense to record, here.
   */

  /* */
  char buffer[1024], *start = buffer, *newline;
  struct pollfd fd;
  int err, offset = 0, newlines = 0;

  fd.fd = out;
  fd.events = POLLIN;

  err = fcntl(out, F_SETFL, O_NONBLOCK);
  say("CONNECTOR! %d\n", err);

  start = buffer;
  memset(buffer, 0, sizeof(buffer));
  while (newlines != 2) {
    newlines = 0;
    fd.revents = 0;

    HANDLE_EINTR(poll(&fd, 1, -1), err);

    err = read(out, start + offset, sizeof(buffer) - offset);
    if (err == -1) {
      break;
    }
    offset += err;
    newline = buffer;
    while ((newline = strchr(newline, '\n')) != NULL) {
      newlines++;
      newline++;
    }
  }

  newline = strchr(buffer, '\n');
  strncpy(plugin.shutdown, buffer, newline- buffer);
  plugin.port = atoi(++newline);

  (void) pthread_mutex_lock(&plugin.mutex);
  (void) pthread_cond_signal(&plugin.cond);
  (void) pthread_mutex_unlock(&plugin.mutex);

  say("SHUTDOWN: %s\nPORT: %d\n", plugin.shutdown, plugin.port);
}

NPError OSCALL NP_Initialize(NPNetscapeFuncs *browser) {
  const char *home;
  char logfile[PATH_MAX];//, cubby[PATH_MAX];
  const char* dir;
  Dl_info library;
  struct attendant__initializer initializer;

  sleep(10000);
  /* Create a log in the home directory of the current user. */
  home = getenv("HOME");
  snprintf(logfile, sizeof(logfile), "%s/verity.log", home);
  plugin.log = open(logfile, O_WRONLY | O_APPEND | O_CREAT, 0644);

  say("NP_Initialize\n");

  /* Copy the browser NPAPI functions into our library. */
  npn_get_url = browser->geturl;
  npn_post_url = browser->posturl;
  npn_request_read = browser->requestread;
  npn_new_stream = browser->newstream;
  npn_write = browser->write;
  npn_destroy_stream = browser->destroystream;
  npn_status = browser->status;
  npn_user_agent = browser->uagent;
  npn_mem_alloc = browser->memalloc;
  npn_mem_free = browser->memfree;
  npn_mem_flush = browser->memflush;
  npn_reload_plugins = browser->reloadplugins;
  npn_get_java_env = browser->getJavaEnv;
  npn_get_java_peer = browser->getJavaPeer;
  npn_get_url_notify = browser->geturlnotify;
  npn_post_url_notify = browser->posturlnotify;
  npn_get_value = browser->getvalue;
  npn_set_value = browser->setvalue;
  npn_invalidate_rect = browser->invalidaterect;
  npn_invalidate_region = browser->invalidateregion;
  npn_force_redraw = browser->forceredraw;
  npn_get_string_identifier = browser->getstringidentifier;
  npn_get_string_identifiers = browser->getstringidentifiers;
  npn_get_int_identifier = browser->getintidentifier;
  npn_identifier_is_string = browser->identifierisstring;
  npn_utf8_from_indentifier = browser->utf8fromidentifier;
  npn_int_from_identifier = browser->intfromidentifier;
  npn_create_object = browser->createobject;
  npn_retain_object = browser->retainobject;
  npn_release_object = browser->releaseobject;
  npn_invoke = browser->invoke;
  npn_invoke_default = browser->invokeDefault;
  npn_evaluate = browser->evaluate;
  npn_get_property = browser->getproperty;
  npn_set_property = browser->setproperty;
  npn_remove_property = browser->removeproperty;
  npn_has_property = browser->hasproperty;
  npn_has_method = browser->hasmethod;
  npn_release_variant_value = browser->releasevariantvalue;
  npn_set_exception = browser->setexception;
  npn_push_popups_enabled_state = browser->pushpopupsenabledstate;
  npn_pop_popups_enabled_state = browser->poppopupsenabledstate;
  npn_enumerate = browser->enumerate;
  npn_plugin_thread_async_call = browser->pluginthreadasynccall;
  npn_construct = browser->construct;
  npn_get_value_for_url = browser->getvalueforurl;
  npn_set_value_for_url = browser->setvalueforurl;
  npn_get_authentication_info = browser->getauthenticationinfo;
  npn_schedule_timer = browser->scheduletimer;
  npn_unschedule_timer = browser->unscheduletimer;
  npn_pop_up_context_menu = browser->popupcontextmenu;
  npn_convert_point = browser->convertpoint;
  npn_handle_event = browser->handleevent;
  npn_unfocus_instance = browser->unfocusinstance;
  npn_url_redirect_response = browser->urlredirectresponse;

  /* Create our mutex and signaling device. */
  (void) pthread_mutex_init(&plugin.mutex, NULL);
  (void) pthread_cond_init(&plugin.cond, NULL);

  dladdr(NP_Initialize, &library);
  dir = library.dli_fname + strlen(library.dli_fname);
  while (*dir != '/') {
    dir--;
  }
  strncat(plugin.cubby, library.dli_fname, dir - library.dli_fname);
  strcat(plugin.cubby, "/cubby");

  memset(&initializer, 0, sizeof(struct attendant__initializer));
  strncat(initializer.relay, library.dli_fname, dir - library.dli_fname);
  strcat(initializer.relay, "/relay");

  initializer.starter = starter;
  initializer.connector = connector;
  initializer.canary = 31;

  attendant.initialize(&initializer);

  starter(0);

  say("cubby: %s\n", plugin.cubby);
  say("relay: %s\n", initializer.relay);

  plugin.curl = curl_easy_init();

  return NPERR_NO_ERROR;
}

/* Our plugin has a scriptable object. This is it. */
struct verity_object {
  NPClass *class;
  uint32_t reference_count;
};

struct NPClass verity_class;

NPObject* Verity_Allocate(NPP npp, NPClass *class) {
  struct verity_object *object;
  say("Verity_Allocate\n");
  object = (struct verity_object*) malloc(sizeof(struct verity_object));
  object->class = &verity_class; 
  object->reference_count = 1;
  return (NPObject*) object;
}

void Verity_Deallocate(NPObject *object) {
  say("Verity_Dellocate\n");
}

void Verity_Invalidate(NPObject *object) {
}

bool Verity_HasMethod(NPObject *object, NPIdentifier name) {
  return false;
}

bool Verity_Invoke(NPObject *object, NPIdentifier name, const NPVariant *argv,
    uint32_t argc, NPVariant *result) {
  return true;
}

bool Verity_InvokeDefault(NPObject *object, const NPVariant *argv,
    uint32_t argc, NPVariant *result) {
  return true;
}

bool Verity_HasProperty(NPObject *object, NPIdentifier name) {
  bool exists = false;
  if (npn_identifier_is_string(name)) {
    NPUTF8* string = npn_utf8_from_indentifier(name);
    exists = strcmp(string, "port") == 0;
    npn_mem_free(string);
  }
  return exists;
}

bool Verity_GetProperty(NPObject *object, NPIdentifier name, NPVariant *result) {
  bool exists = false;
//  Dl_info library;
  if (npn_identifier_is_string(name)) {
    NPUTF8* string = npn_utf8_from_indentifier(name);
    say("Verity_GetProperty: %s\n", string);
    exists = strcmp(string, "port") == 0;
    if (exists) {
      result->type = NPVariantType_Int32;
      (void) pthread_mutex_lock(&plugin.mutex);
      while (plugin.port == 0) {
        pthread_cond_wait(&plugin.cond, &plugin.mutex);
      }
      result->value.intValue = plugin.port;
      (void) pthread_mutex_unlock(&plugin.mutex);
    }
    npn_mem_free(string);
  }
  return exists;
}

bool Verity_SetProperty(NPObject *object, NPIdentifier name, const NPVariant *value) {
  return false;
}

bool Verity_RemoveProperty(NPObject *object, NPIdentifier name) {
  return false;
}

bool Verity_Enumeration(NPObject *object, NPIdentifier **value, uint32_t *count) {
  return false;
}

bool Verity_Construct(NPObject *object, const NPVariant *argv, uint32_t argc,
    NPVariant *result) {
  return false;
}

struct NPClass verity_class = {
  NP_CLASS_STRUCT_VERSION, Verity_Allocate, Verity_Deallocate,
  Verity_Invalidate, Verity_HasMethod, Verity_Invoke, Verity_InvokeDefault,
  Verity_HasProperty, Verity_GetProperty, Verity_SetProperty,
  Verity_RemoveProperty, Verity_Enumeration, Verity_Construct
};

NPError NP_LOADDS Verity_New(NPMIMEType mime, NPP instance, uint16_t mode,
  int16_t argc, char* argn[], char* argv[], NPSavedData* saved) {
  say("NPP_New\n");
  return NPERR_NO_ERROR;
}

NPError NP_LOADDS Verity_Destroy(NPP instance, NPSavedData** save) {
  say("NPP_Destroy\n");
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
  say("NPP_DestroyStream\n");
  return NPERR_NO_ERROR;
}

int32_t NP_LOADDS Verity_WriteReady(NPP instance, NPStream *stream) {
  say("NPP_WriteReady\n");
  return 0;
}

int32_t NP_LOADDS Verity_Write(NPP instance, NPStream *stream, int32_t offset,
    int32_t len, void *buffer) {
  say("NPP_Write\n");
  return 0;
}

void NP_LOADDS Verity_Print(NPP instance, NPPrint *print) {
  say("NPP_Print\n");
}

int16_t NP_LOADDS Verity_HandleEvent(NPP instance, void *event) {
  say("NPP_HandleEvent\n");
  return 0;
}

void NP_LOADDS Verity_URLNotify(NPP instance, const char *url,
    NPReason reason, void *data) {
  say("NPP_URLNotify\n");
}

NPError NP_LOADDS Verity_GetValue(NPP instance, NPPVariable variable,
    void *value) {
  switch (variable) {
    case NPPVpluginScriptableNPObject:
      (*(NPObject**)value) = npn_create_object(instance, &verity_class);  
    default:
      break;
  }
  say("NPP_GetValue %d\n", variable);
  return NPERR_NO_ERROR;
}

NPError NP_LOADDS Verity_SetValue(NPP instance, NPNVariable variable,
    void *value) {
  say("NPP_SetValue\n");
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
  say("NPP_URLRedirectNotify\n");
}

NPError NP_LOADDS Verity_ClearSiteData(const char *site, uint64_t flags, uint64_t maxAge) {
  say("NPP_ClearSiteData\n");
  return NPERR_NO_ERROR;
}

char** NP_LOADDS Verity_GetSitesWithData() {
  say("NPP_GetSitesWithData\n");
  return NULL;
}

NPError OSCALL NP_GetEntryPoints(NPPluginFuncs *class) {
  say("NP_GetEntryPoints\n");
  class->size = sizeof(NPPluginFuncs);
  class->version = (NP_VERSION_MAJOR << 8) + NP_VERSION_MINOR;
  /* TODO Lower case these. */
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
  char url[4096];
  attendant.shutdown();
  if (plugin.curl) {
    sprintf(url, "http://127.0.0.1:%d/cubby/shutdown?%s", plugin.port, plugin.shutdown);
    say("Requesting shutdown: %s.\n", url);
    curl_easy_setopt(plugin.curl, CURLOPT_URL, url);
    curl_easy_perform(plugin.curl);
    curl_easy_cleanup(plugin.curl);
  }
  if (!attendant.done(250)) {
    say("Scram Cubby!\n");
    attendant.scram();
    if (!attendant.done(500)) {
      say("Cubby still running!\n");
    }
  }
  say("Shutdown.\n");
  close(plugin.log);
}

char* OSCALL NP_GetMIMEDescription() {
  return "application/mozilla-npruntime-scriptable-plugin:.foo:Scriptability";
}
