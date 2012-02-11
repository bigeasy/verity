// TODO Parameterize.
const VERITY = "http://verity:8078";
var CUBBY;

function loadScripts(Request, userScript, callback) {
  var scripts = [
        [ VERITY + "/test/boilerplate.js"
        , userScript
        ]
      ],
      seen    = { system: {}, user: {} },
      loaded  = { system: {}, user: [] };

  function dependencies(name, source) {
    var dependency, dependencies = [], lines = source.split(/\n/);
    for (var i = 0, stop = lines.length; i < stop; i++) {
      if ( /^\s*$/.test( lines[i] ) ) continue;
      var match = /^\s*\/\/(?:\s+@require\s+(\S+))/.exec(lines[i]);
      if (! match) break;
      if (dependency = match[1]) {
        dependencies.push(dependency);
      }
    }
    if (dependencies.length) {
      scripts.unshift(source);
      scripts.unshift(dependencies);
    } else if (name) {
      loaded.system[name] = source
    } else {
      loaded.user.push(source);
    }
  }

  var isSystem = new RegExp("^" + VERITY + "/test/(boilerplate)\\.js$");
  function loadScript() {
    var match, name;
    if (scripts[0].length == 0) {
      scripts.shift()
      if (scripts[0]) {
        loaded.user.push(scripts.shift());
      }
      if (scripts[0]) {
        loadScript();
      } else {
        callback(loaded);
      }
    } else {
      var url = scripts[0].shift();
      if (match = isSystem.exec(url)) { 
        name = match[1]
        if (!seen.system[name]) {
          seen.system[url] = true;
        } else {
          seen.user[url] = true;
        }
      } else {
        name = null;
      }
      if (!seen.user[url]) {
        seen.user[url] = true;
        new Request({
          url: url,
          onComplete: function (response) {
            dependencies(name, response.text);
            loadScript();
          }
        }).get();
      } else {
        loadScript();
      }
    }
  }
  loadScript();
}

function substitute(string, parameters) {
  var combined = [];
  var parts = string.split(/_X_V_X_=_X_V_X_/);
  for (var i = 0, stop = parameters.length; i < stop; i++) {
    combined.push(parts.shift());
    combined.push(parameters.shift());
  }
  combined.push(parts.shift());
  return combined.join("");
}

function indent(text, padding) {
  var space = new Array(padding + 1).join(" ");
  var lines = text.split(/\n/);
  for (var i = 0, stop = lines.length; i < stop; i++) {
    if (/\S/.test(lines[i])) lines[i] = space + lines[i];
  }
  return lines.join("\n");
}

function XHRRequest_send(method) {
    var options = this.options;
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4) sendDirectives(directives, uri, csrf, callback);
    };
    xhr.open(method, url, true);
    var headers = options.headers;
    for (var key in headers) {
      if (headers.hasOwnProperty(key)) {
        xhr.setRequestHeader(key, headers[key]);
      }
    }
    xhr.setRequestHeader("content-type", "application/x-www-form-urlencoded");
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4) {
        options.callback({
          text: xhr.responseText,
          status: xhr.status,
          statusText: xhr.statusText
        });
      }
    };
    return xhr;
}

function XHRRequest_get() {
  XHRRequest_send("GET").send();
}

function XHRRequest_post() {
  var options = this.options;
  var encoded = [];
  for (var key in options.content) {
    if (options.hasOwnProperty(key)) {
      encoded.push(escape(key) + "=" + escape(options[key]));
    }
  }
  XHRRequest_send("POST").send(encoded.join("&"));
}

function XHRRequest(options) {
  this.options = options;
  this.send = XHRRequest_send;
  this.get = XHRRequest_get;
  this.post = XHRRequest_post;
  return this;
}

function sendDirectives(Request, directives, uri, csrf, callback) {
  if (directives.length) {
    var directive = directives.shift();
    new Request({
      url: VERITY + "/test/directive",
      headers: { "x-csrf-token": csrf },
      onComplete: function () {
        sendDirectives(Request, directives, uri, csrf, callback);
      },
      content: { directive: directive, base: uri }
    }).post();
  } else {
    callback();
  }
}

function createCompiler(Request, uri, base, token, csrf, inject) {
  return function (sources) {
    var system = sources.system, expected = 0, directives = [];
    sources.user.forEach(function (source) {
      source.split(/\n+/).forEach(function (line) {
        var match = /^\s*(\/\/@\s+\S*\s+.*)$/.exec(line);
        if (match) directives.push(match[1]);
      });
    });

    var user = indent(sources.user.join("\n\n"), 2);
    system.boilerplate = substitute(system.boilerplate, [ token, user ]);

    // We used to have more than one system script to inject, but now we have
    // just the one. In time, if that's the way it stays, we'll come back and
    // simplify the callbacks.
    sendDirectives(Request, directives, base, csrf, function () {
      // TODO URL should be based on current location.
      system.boilerplate += "\n\n//@ sourceUrl=http://verity.prettyrobots.com/injected.js\n"
      source = system.boilerplate;
      inject(system.boilerplate);
    });
  }
}

function loadTest(Request, uri, text, inject) {
  var args = text.split(/\s+/),
      source = args.shift(),
      token = args.shift(),
      csrf = args.shift();
  loadScripts(Request, source, createCompiler(Request, uri, source, token, csrf, inject));
}

function shouldTest(Request, url, inject) {
  new Request({
    url: VERITY + '/test/visit?url=' + escape(url),
    onComplete: function (response) {
      var text = response.text;
      if (text && text != "" && text != "NONE") loadTest(Request, url, text, inject);
    }
  }).get();
}

if (exports) {
  exports.shouldTest = shouldTest;
}
