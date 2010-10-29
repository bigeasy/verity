(function () {
  var configuration = null,
      reloading = false;

  function freeze (map) {
    var frozen = [];
    for (var key in map) {
      frozen.push(escape(key) + "=" + escape(map[key]));
    }
    return frozen.join("&");
  }

  function thaw (string) {
    var map = {};
    var pairs = string.split(/&/);
    for (var i = 0, stop = pairs.length; i < stop; i++) {
        var pair = pairs[i].split(/=/);
      map[unescape(pair[0])] = unescape(pair[1]);
    }
    return map;
  }

  $(function () {
    var reloading = false;
    if (!configuration && document.cookie) {
      var cookies = document.cookie.split(/;\s+/);
      for (var i = 0, stop = cookies.length; i < stop; i++) {
        var match = /^VERITY_CONFIGURATION=(.*)$/.exec(cookies[i]);
        if (match) {
          configuration = thaw(match[1]);
        }
      }
    }
    if (!configuration) {
      configuration = { status: "running", alone: true };
    }
    switch (configuration.status) {
    case "reloading":
      configuration.previous = configuration.status;
      configuration.status = "reloaded";
      document.cookie = "VERITY_CONFIGURATION=" + freeze(configuration) + "; path=/";
      window.location.reload(true);
      break;
    case "reloaded":
      configuration.previous = configuration.status;
      configuration.status = "running";
      document.cookie = "VERITY_CONFIGURATION=" + freeze(configuration) + "; path=/";
      window.location.href = window.location.href;
      break;
    case "running":
      configuration.previous = configuration.status;
      if (!configuration.alone) {
        document.cookie = "VERITY_CONFIGURATION=" + freeze(configuration) + "; path=/";
      }
    }
    if (configuration.previous == "running") {
      if (!configuration.alone) {
        configuration.tests = configuration.tests.split(",");
        var test = configuration.tests[0],
            path = window.location.pathname;
        if (path.indexOf(test) == path.length - test.length) {
          QUnit.done = function () {
          }
        }
      }
      for (var i = 0, stop = tests.length; i < stop; i++) {
        try { tests[i]() } catch (_) {}
      }
    }
  });

  var tests = [];
    
  window.Verity = function (test) {
    if (configuration) test();
    else tests.push(test);
  }
})();
