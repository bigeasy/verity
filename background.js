// TODO Parameterize.
const VERITY = "http://verity:8078";
var CUBBY;

function createLoader(options, source, compiler) {
  return function (token) {
    var scripts = [ [ source ] ], seen = {}, loaded = []; 

    function dependencies(source) {
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
      } else {
        loaded.push(source);
      }
    }

    function loadScript() {
      var match, name;
      if (scripts[0].length == 0) {
        scripts.shift()
        if (scripts[0]) {
          loaded.push(scripts.shift());
        }
        if (scripts[0]) {
          loadScript();
        } else {
          compiler(loaded, token);
        }
      } else {
        var url = scripts[0].shift();
        if (!seen[url]) {
          seen[url] = true;
          var request = options.createRequest({
            url: url,
            onComplete: function (response) {
              dependencies(response.text);
              loadScript();
            }
          })
          request.get();
        } else {
          loadScript();
        }
      }
    }
    loadScript();
  }
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
    xhr.open(method, options.url, true);
    var headers = options.headers;
    for (var key in headers) {
      if (headers.hasOwnProperty(key)) {
        xhr.setRequestHeader(key, headers[key]);
      }
    }
    xhr.setRequestHeader("content-type", "application/x-www-form-urlencoded");
    xhr.onreadystatechange = function () {
      if (xhr.readyState == 4) {
        options.onComplete({
          text: xhr.responseText,
          status: xhr.status,
          statusText: xhr.statusText
        });
      }
    };
    return xhr;
}

function XHRRequest_get() {
  this.send("GET").send();
}

function XHRRequest_post() {
  var content = this.options.content;
  var encoded = [];
  for (var key in content) {
    if (content.hasOwnProperty(key)) {
      encoded.push(escape(key) + "=" + escape(content[key]));
    }
  }
  this.send("POST").send(encoded.join("&"));
}

function XHRRequest(options) {
  this.options = options;
  this.send = XHRRequest_send;
  this.get = XHRRequest_get;
  this.post = XHRRequest_post;
  return this;
}

function createXHRRequest(options) {
  return new XHRRequest(options);
}

// Via: [Yaffle](https://gist.github.com/1088850)
function parseURI(url) {
  var m = String(url).replace(/^\s+|\s+$/g, '').match(/^([^:\/?#]+:)?(\/\/(?:[^:@]*(?::[^:@]*)?@)?(([^:\/?#]*)(?::(\d*))?))?([^?#]*)(\?[^#]*)?(#[\s\S]*)?/);
  // authority = '//' + user + ':' + pass '@' + hostname + ':' port
  return (m ? {
    href     : m[0] || '',
    protocol : m[1] || '',
    authority: m[2] || '',
    host     : m[3] || '',
    hostname : m[4] || '',
    port     : m[5] || '',
    pathname : m[6] || '',
    search   : m[7] || '',
    hash     : m[8] || ''
  } : null);
}

function absolutizeURI(base, href) {// RFC 3986

  function removeDotSegments(input) {
    var output = [];
    input.replace(/^(\.\.?(\/|$))+/, '')
         .replace(/\/(\.(\/|$))+/g, '/')
         .replace(/\/\.\.$/, '/../')
         .replace(/\/?[^\/]*/g, function (p) {
      if (p === '/..') {
        output.pop();
      } else {
        output.push(p);
      }
    });
    return output.join('').replace(/^\//, input.charAt(0) === '/' ? '/' : '');
  }

  href = parseURI(href || '');
  base = parseURI(base || '');

  return !href || !base ? null : (href.protocol || base.protocol) +
         (href.protocol || href.authority ? href.authority : base.authority) +
         removeDotSegments(href.protocol || href.authority || href.pathname.charAt(0) === '/' ? href.pathname : (href.pathname ? ((base.authority && !base.pathname ? '/' : '') + base.pathname.slice(0, base.pathname.lastIndexOf('/') + 1) + href.pathname) : base.pathname)) +
         (href.protocol || href.authority || href.pathname ? href.search : (href.search || base.search)) +
         href.hash;
}

// TODO Error messaging.
function addInjection(options, base, value) {
    var match = /^\s*(?:'((?:[^\\']|\\.)+)'|"((?:[^\\"]|\\.)+)"|\/((?:[^/\\]|\\.)+)\/)\s+(\S+)\s*$/.exec(value);
    var target = match[1] || match[2], specials;
    if (target) {
      target = target.replace(/\\(.)/g, "$1")
      specials = [
        '/', '.', '*', '+', '?', '|',
        '(', ')', '[', ']', '{', '}', '\\'
      ]
      specials = new RegExp(
        '(\\' + specials.join('|\\') + ')', 'g'
      )
      target = target.replace(specials, '\\$1')
      target = "^" + target + "$"
    } else {
      target = match[3];
    }
    var source = absolutizeURI(base, match[4]);
    options.addInjection(target, source);
}

function createCompiler(options, base) {
  return function (sources, token) {
    var source = sources.join("\n\n"), directives = [], boilerplate, when;

    source.split(/\n+/).forEach(function (line) {
      var match = /^\s*\/\/@\s+(\S*)\s+(.*)$/.exec(line);
      if (match) {
        directives.push({ name: match[1], value: match[2] });
      }
    });

    // TODO How to report errors?
    directives.forEach(function (directive) {
      switch (directive.name) {
      case "when": 
        addInjection(options, base, directive.value);
        break;
      case "expect":
        when = parseInt(directive.value.trim());
        break;
      case "include":
        break;
      default:
      }
    });

    source = indent(source, 2);
    boilerplate = substitute(options.boilerplate, [ token, source ]);
    boilerplate += "\n\n//@ sourceUrl=http://verity.prettyrobots.com/injected.js?" + options.url + "\n"
    options.injector(boilerplate);
  }
}

// TODO Need to get a reporting token from the server, but not quite yet.
function fetchReportingToken(options, callback) { callback("X") }

function shouldTest(options) {
  var entry;
  for (var i = 0, stop = options.injections.length; i < stop && !entry; i++) {
    var re = new RegExp(options.injections[i].target);
    if (re.test(options.url)) {
      entry = options.injections[i];
    }
  }
  if (entry) {
    options.removeInjection(entry.target);

    var source = entry.source;
    var compiler = createCompiler(options, source)
    var loader = createLoader(options, source, compiler);

    fetchReportingToken(options, loader);
  } else {
    options.injector(null);
  }
}

if (typeof exports != "undefined") {
  exports.shouldTest = shouldTest;
}
