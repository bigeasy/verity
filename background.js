// TODO Parameterize.
const VERITY = "http://verity:8078";
var CUBBY;

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

function sendDirectives(directives, uri, csrf, callback) {
  if (directives.length) {
    var directive = directives.shift();
    var data = "directive=" + escape(directive) + "&base=" + escape(uri);
    var xhr = new XMLHttpRequest();
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4) sendDirectives(directives, uri, csrf, callback);
    };
    xhr.open("POST", VERITY + "/test/directive", true);
    xhr.setRequestHeader("x-csrf-token", csrf);
    xhr.setRequestHeader("content-type", "application/x-www-form-urlencoded");
    xhr.send(data);
  } else {
    callback();
  }
}

function createCompiler(tabId, uri, base, token, csrf) {
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
    sendDirectives(directives, base, csrf, function () {
      // TODO URL should be based on current location.
      system.boilerplate += "\n\n//@ sourceUrl=http://verity.prettyrobots.com/injected.js\n"
      source = system.boilerplate;
      chrome.tabs.sendRequest(tabId, source, function () {});
    });
  }
}

function loadTest(tabId, uri, text) {
  var args = text.split(/\s+/),
      source = args.shift(),
      token = args.shift(),
      csrf = args.shift();
  loadScripts(source, createCompiler(tabId, uri, source, token, csrf));
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
      };
      var query = 'url=' + escape(details.url);
      xhr.open("GET", VERITY + '/test/visit?' + query, true);
      xhr.send();
    }
  }
})());
