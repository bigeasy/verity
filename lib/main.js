const tabs = require("tabs");
const Request = require("request").Request;
const bg = require("extension/background");
const data = require("self").data;

exports.main = function(options, callbacks) {
  tabs.on("ready", function(tab) {
    worker = tab.attach({
      contentScriptFile: data.url("content.js")
    });
    bg.shouldTest(Request, tab.url, function (source) {
      worker.port.emit("source", source);
    });
  });
};
