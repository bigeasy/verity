// TODO: Run only on a top script.
const VERITY = "http://verity:8078";


function loadScripts(userScript, callback) {
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
        var xhr = new XMLHttpRequest();
        xhr.onreadystatechange = function () {
          if (xhr.readyState == 4) {
            dependencies(name, xhr.responseText);
            loadScript();
          }
        }
        xhr.open("GET", url, true);
        xhr.send();
      } else {
        loadScript();
      }
    }
  }
  loadScript();
}
const CUBBY = "http://127.0.0.1:8089/cubby";

function stash(tabId, uri, sources, callback) {
  var keys = Object.keys(sources);
  var inject = function(key) {
    var request = { uri: CUBBY + "/" + key + ".js?" + escape(uri) };
    chrome.tabs.sendRequest(tabId, request, function () {
      submit();
    });
  };
  var put = function (key, token) {
    var query = "token=" + token, xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4) {
        inject(key);
      }
    }
    // http.setRequestHeader("Content-Length", sources[key].length);
    xhr.open("POST", "http://127.0.0.1:8089/cubby/data?" + query, true);
    xhr.send(sources[key]);
  };
  var token = function () {
    var key = keys.shift(), query = [], xhr = new XMLHttpRequest();
    query.push("uri=" + escape(uri));
    query.push("name=" + escape(key));
    query = query.join("&");
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4) {
        put(key, xhr.responseText);
      }
    }
    console.log("http://127.0.0.1:8089/cubby/token?" + query);
    xhr.open("GET", "http://127.0.0.1:8089/cubby/token?" + query, true);
    xhr.send();
  };
  var submit = function () {
    if (keys.length) token();
    else callback();
  };
  submit();
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

function sendInjections(injections, uri, csrf, callback) {
  if (injections.length) {
    var value = injections.shift(),
        match = /^\s*(?:'((?:[^\\']|\\.)+)'|"((?:[^\\"]|\\.)+)"|\/((?:[^\\\/]|\\.)+)\/)\s+(\S+)\s*$/.exec(value),
        specials, target, source, data;
    if (match) {
      if (match[1] || match[2]) {
        target = match[1] ? match[1] : match[2];
        target = target.replace(/\\(.)/g, "$1");
        specials = [
          '/', '.', '*', '+', '?', '|',
          '(', ')', '[', ']', '{', '}', '\\'
        ];
        specials = new RegExp(
          '(\\' + specials.join('|\\') + ')', 'g'
        );
        target = target.replace(specials, '\\$1');
      } else {
        target = match[3];
      }
      source = match[4];
    } else {
      throw new Error("Bad target.");
    }
    data = "target=" + escape(target) + "&source=" + escape(source) + "&base=" + escape(uri);
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4) sendInjections(injections, uri, csrf, callback);
    }
    xhr.open("POST", VERITY + "/test/inject", true);
    xhr.setRequestHeader("x-csrf-token", csrf);
    xhr.setRequestHeader("content-type", "application/x-www-form-urlencoded");
    xhr.send(data);
  } else {
    callback();
  }
}

function createCompiler(tabId, uri, token, csrf) {
  return function (sources) {
    var system = sources.system, expected = 0, injections = [];
    sources.user.forEach(function (source) {
      source.split(/\n+/).forEach(function (line) {
        var match = /^\s*\/\/@\s+(\S*)\s+(.*)$/.exec(line);
        if (match) {
          var directive = match[1], value = match[2];
          switch (directive) {
          case "include":
            break;
          case "when":
            injections.push(value);
            break;
          case "expect":
            expected += parseInt(value.trim(), 10);
            break;
          // TODO: Send errors to server?
          default:
            throw new Error("Unexpected directive: " + directive);
          }
        }
      });
    });
    var user = indent(sources.user.join("\n\n"), 2);
    system.boilerplate = substitute(system.boilerplate, [ token, user ]);
    sendInjections(injections, uri, csrf, function () {
      stash(tabId, uri, system, function () {});
    });
  }
}

function loadTest(tabId, uri, text) {
  var args = text.split(/\s+/),
      source = args.shift(),
      token = args.shift(),
      csrf = args.shift();
  loadScripts(source, createCompiler(tabId, uri, token, csrf));
}

chrome.webNavigation.onCompleted.addListener((function () {
  return function (details) {
    if (details.frameId == 0) {
      var xhr = new XMLHttpRequest();
      xhr.onreadystatechange = function () {
        if (xhr.readyState == 4) {
          var text = xhr.responseText;
          if (text && text != "" && text != "NONE") loadTest(details.tabId, details.url, text);
        }
      }
      var query = 'url=' + escape(details.url)
      xhr.open("GET", 'http://verity:8078/test/visit?' + query, true);
      xhr.send();
    }
  }
})());
