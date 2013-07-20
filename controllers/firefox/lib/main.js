const tabs = require("tabs");
const Request = require("request").Request;
const bg = require("extension/background");
const data = require("self").data;
const boilerplate = require("extension/boilerplate");
const storage = require("simple-storage").storage;

exports.main = function(options, callbacks) {
  var injections = [];
  tabs.on("ready", function(tab) {
    worker = tab.attach({
      contentScriptFile: data.url("content.js")
    });
    bg.shouldTest({
      url: tab.url,
      boilerplate: boilerplate,
      removeInjection: function (target) {
        injections = injections.filter(function (e) { e.target != target });
      //  localStorage["injections"] = JSON.stringify(injections);
      },
      addInjection: function (target, source) {
        injections.push({ target: target, source: source });
       // localStorage["injections"] = JSON.stringify(injections);
      },
      injections: injections.concat([
        { target: /jekyllrb.com/.toString()
        , source: "http://dossier:8048/verity/user/landing/index.js"
        }
      ]),
      createRequest: function (options) {
        return new Request(options);
      },
      injector: function (source) {
        if (source) {
          worker.port.emit("source", source);
        }
      }
    });
  });
};
