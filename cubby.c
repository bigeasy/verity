#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <errno.h>
#include "mongoose.h"

 #include <unistd.h>

static const char *http_header =
  "HTTP/1.1 %d OK\r\n"
  "Cache: no-cache\r\n"
  "Content-Type: %s\r\n"
  "\r\n";

// TODO We started out with more than one script injected into a test page. Now
// we have only one page. We can return to this code and remove the name
// parameters, since we do not have to distinguish scripts with in the host
// page. We only need to distinguish by referer.
struct page {
  char token[65];
  char *name;
  char *uri; // TODO Rename referer.
  void *data;
  size_t size;
  time_t when;
};

static struct page pages[128];

static void send_javascript(struct mg_connection *conn, const void* buffer, size_t size)
{
  mg_printf(conn, http_header, 200, "text/javascript");
  mg_write(conn, buffer, size);
}

static void send_line(struct mg_connection *conn, int status, const char* line)
{
  mg_printf(conn, http_header, status, "text/plain");
  mg_printf(conn, "%s\n", line);
}

static char* dup_var(struct mg_connection *conn, const char *query, const char* name)
{
  const char *start = query, *after, *end;
  size_t length = -1;
  char *value = 0;
  int got;
  while ((start = strstr(start, name)) != NULL) {
    after = start + strlen(name);
    if (*after == '=') {
      ++after;
      if ((end = strstr(after, "&")) == NULL) {
        end = query + strlen(query);
      }
      length = end - start + 1;
      break;
    }
    start = after;
  }
  if (length != -1) {
    value = malloc(length);
    if (value) {
      got = mg_get_var(query, strlen(query), name, value, length);
    } else {
      send_line(conn, 500, "Out of memory.");
    }
  } else {
    send_line(conn, 400, "Parameter missing.");
  }
  return value;
}

struct page *free_page(struct page *page)
{
  free(page->uri);
  free(page->data);
  free(page->name);
  page->data = page->name = page->uri = 0;
  return page;
}

struct page *allocate_page()
{
  struct page *page = pages,
              *stop = pages + (sizeof(pages) / sizeof(struct page));
  while (page != stop && page->uri) page++;
  if (page == stop) {
    time_t min = time(0);
    struct page *iter = pages;
    do 
    {
      if (iter->when < min) {
        page = iter;
        min = page->when;
      }
    }
    while (++iter != stop);
  }
  return free_page(page);
}

struct page *find_page(const char *uri, const char *name)
{
  struct page *page = pages,
              *stop = pages + (sizeof(pages) / sizeof(struct page));
  for (;;) {
    if (page == stop)
      return 0;
    if (page->uri && strcmp(uri, page->uri) == 0 && strcmp(name, page->name) == 0)
      return page;
    page++;
  }
}

struct page *find_page_by_token(const char *token)
{
  int count = 0;
  struct page *page = pages,
              *stop = pages + (sizeof(pages) / sizeof(struct page));
  for (;;) {
    if (page == stop)
      return 0;
    if (page->uri && strcmp(token, page->token) == 0)
      return page;
    page++;
  }
}

char *rand_str(char *dst, int size)
{
   static const char text[] = "0123456789"
                              "abcdefghijklmnopqrstuvwxyz"
                              "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
   int i, len = size - 1;
   for ( i = 0; i < len; ++i ) {
      dst[i] = text[random() % (sizeof text - 1)];
   }
   dst[i] = '\0';
   return dst;
}

static void get_token(struct mg_connection *conn,
                      const struct mg_request_info *request_info)
{
  char *name, *uri;
  const char *query = request_info->query_string;
  struct page *page;
  name = dup_var(conn, query, "name");
  if (name) {
    uri = dup_var(conn, query, "uri");
    if (uri) {
      page = find_page(uri, name);
      if (page) free_page(page);
      else page = allocate_page();
      if (page) {
        page->name = name;
        page->uri = uri;
        page->when = time(0);
        rand_str(page->token, sizeof(page->token));
        send_line(conn, 200, page->token);
      } else {
        send_line(conn, 500, "Out of memory.");
      }
    } else {
      free(name);
    }
  }
}

struct instance {
  char shutdown[65];
  int terminated;
  pthread_mutex_t mutex;
  pthread_cond_t  cond;
};

static size_t get_content_length(const struct mg_connection *conn)
{
  const char *length = mg_get_header(conn, "Content-Length"); 
  if (length) {
    return atoi(length); 
  }
  return -1;
}

#define ECHO_DIRECTORY "/cubby/"

static char* get_name(const char *uri)
{
  const char *start = uri + strlen("/cubby/");
  const char *end = start + strlen(start) - strlen(".js");
  char *name = malloc(end - start + 1);
  if (name) {
    strncpy(name, start, end - start);
  }
  return name;
}

static char* get_uri(const char* uri)
{
  char *query_string = malloc(strlen(uri) + 3);
  if (query_string) {
  }
  return query_string;
}

static void get_data(struct mg_connection *conn,
                     const struct mg_request_info *request_info)
{
// The URL is the entire query string. Mongoose gives us a decode function, but
// it only works on x-www-form-urlencoded key/value pairs, so we have to prepend
// a key to the query string in order decode the value. When we alloc strings
// for this, we don't bother to report out of memory here, since the caller can
// do nothing about it. Just skip it.
  size_t length     = strlen(request_info->query_string);
  char *paired      = malloc(length + 3);
  char *uri         = malloc(length * 2);
  char *name        = get_name(request_info->uri);
  const char *noop  = "(function () {})()";
  struct page *page;
  if (paired && uri && name) {
    strcpy(paired, "u=");
    strcat(paired, request_info->query_string);
    mg_get_var(paired, length + 3, "u", uri, length * 2); 
    page = find_page(uri, name);
  }
  if (page) {
    send_javascript(conn, page->data, page->size);
  } else {
    send_javascript(conn, noop, strlen(noop));
  }
  free(name);
  free(uri);
  free(paired);
}

static void post_data(struct mg_connection *conn,
                      const struct mg_request_info *request_info)
{
  char token[65];
  int got;
  const char *query = request_info->query_string;
  size_t query_length = strlen(query);
  size_t content_length = get_content_length(conn);
  struct page *page = 0; 

  if (content_length == -1) {
    send_line(conn, 411, "Length required.");
  } else {
    got = mg_get_var(query, query_length, "token", token, sizeof(token));
    if (got != -1) {
      page = find_page_by_token(token);
    }
    if (page) {
      page->size  = content_length;
      page->data  = malloc(content_length);
      if (page->name && page->uri && page->data) {
        mg_read(conn, page->data, page->size);
        send_line(conn, 200, "Stashed.");
      } else {
        free_page(page);
        send_line(conn, 500, "Out of memory.");
      }
    } else {
      send_line(conn, 403, "Invalid token.");
    }
  }
}

int is_shutdown(struct instance *instance, const struct mg_request_info *request_info) {
  return (strcmp(request_info->uri, "/cubby/shutdown") == 0)
      && request_info->query_string
      && (strcmp(request_info->query_string, instance->shutdown) == 0);
}

static void shutdown(struct instance *instance,
                     struct mg_connection *conn,
                     const struct mg_request_info *request_info)
{
  if (request_info->query_string
      && (strcmp(request_info->query_string, instance->shutdown) == 0)) {
    send_line(conn, 200, "Goodbye.");
    (void) pthread_mutex_lock(&instance->mutex);
    instance->terminated = 1;
    (void) pthread_cond_signal(&instance->cond);
    (void) pthread_mutex_unlock(&instance->mutex);
  } else {
    send_line(conn, 403, "Forbidden.");
  }
}

static int is_javascript(const char* uri) {
  const char *dir = "/cubby/";
  if (strstr(uri, dir)) {
    size_t offset = strlen(uri) - 3;
    if (offset > 0) {
      const char *suffix = uri + offset;
      if (strstr(suffix, ".js") == suffix) {
        return strchr(uri + strlen(dir), '/') == NULL;
      }
    }
  }
  return 0;
}

static void *callback(enum mg_event event,
                      struct mg_connection *conn,
                      const struct mg_request_info *request_info) {
  if (event == MG_NEW_REQUEST) {
    struct instance *instance = (struct instance*) request_info->user_data;
    if (strcmp(request_info->uri, "/cubby/token") == 0) {
      get_token(conn, request_info);
    } else if (strcmp(request_info->uri, "/cubby/data") == 0
               && request_info->query_string) {
      post_data(conn, request_info);
    } else if (is_javascript(request_info->uri)) {
      get_data(conn, request_info);
    } else if (strcmp(request_info->uri, "/cubby/shutdown") == 0) {
      shutdown(instance, conn, request_info);
    } else {
      send_line(conn, 404, "Not found.");
    }
    return "";  // Mark as processed
  } else {
    return NULL;
  }
}

int main(void)
{
  struct mg_context *ctx;
  int port = 49151, err = EADDRINUSE, count = 0;
  char listening_port[64];
  const char *options[] = {"listening_ports", "8089", "num_threads", "1", NULL};
  struct instance instance;

  srandomdev();

  (void) pthread_mutex_init(&instance.mutex, NULL);
  (void) pthread_cond_init(&instance.cond, NULL);

  instance.terminated = 0;

  /* Print the shutdown secret to standard out. */
  rand_str(instance.shutdown, sizeof(instance.shutdown));
  printf("%s\n", instance.shutdown);
  fflush(stdout);

  options[1] = listening_port;
  while (err == EADDRINUSE) {
    sprintf(listening_port, "%d", ++port);
    ctx = mg_start(&callback, &instance, options);
    err = errno;
    errno = 0;
    count++;
  }

  /* Print the port number to standard out. */
  printf("%d\n", port);
  fflush(stdout);

  // TODO: Build on Windows.

  // Wait for signal to shutdown server.
  (void) pthread_mutex_lock(&instance.mutex);
  while (!instance.terminated) {
    (void) pthread_cond_wait(&instance.cond, &instance.mutex);
  }
  (void) pthread_mutex_unlock(&instance.mutex);

  mg_stop(ctx);

  (void) pthread_mutex_destroy(&instance.mutex);
  (void) pthread_cond_destroy(&instance.cond);

  return 0;
}
