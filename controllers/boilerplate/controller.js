// TODO Parameterize server.
var verity = (function (previous) {
  var identifier = "_X_V_X_=_X_V_X_",
      server = "http://verity:8078",
      queue = [],
      running = false;

  function submit(record, callback) {
    var src = server + "/test/progress.js?callback=?", data =
    { identifier: identifier
    , record: escape(JSON.stringify(record))
    };
    Syn.jquery().getJSON(src, data).complete(callback);
  }

  function clone(args) {
    return Array.prototype.slice.call(args, 0);
  }

  function dequeue() {
    var pending = 1, completed = 0;
    var next = function () {
      if (++completed == pending) poll();
    };
    var send = function (message) {
      ++pending;
      submit(message, next);
    };
    try {
      running = true;
      queue.shift()(send);
      next();
    } catch (e) {
      // TODO Report error to server, restart tests.
      throw e;
    }
  }

  function assert(test) {
    return function (send) {
      var ok = function (condition, message) {
        send({ type: "assertion", status: condition ? "pass" : "fail", message: message });
      };
      var assert = { ok: ok };
      test(assert);
    }
  }

  function run(tests) {
    queue = queue.concat(tests);
    if (!running) dequeue();
  }

  function test(tests) {
    for (var i = 0, stop = tests.length; i < stop; i++) {
      tests[i] = assert(tests[i]);
    }
    run(tests);
  }

  function poll() {
    if (queue.length) dequeue();
    else running = false;
  }

  run(function (send) {
    send({ type: "visit", url: window.location.href });
  });

  // This source will be wrapped in a closure during generation. The closure will
  // expect a verity function return value.
  return function () { test(clone(arguments)) };
})();
