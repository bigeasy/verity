chrome.webNavigation.onCompleted.addListener(function (details) {
  var injections = JSON.parse(localStorage["injections"] || "[]");
  if (details.frameId == 0) {
    shouldTest({
      url: details.url,
      boilerplate: boilerplate,
      removeInjection: function (target) {
        injections = injections.filter(function (e) { e.target != target });
        localStorage["injections"] = JSON.stringify(injections);
      },
      addInjection: function (target, source) {
        injections.push({ target: target, source: source });
        localStorage["injections"] = JSON.stringify(injections);
      },
      injections: injections.concat([
        { target: /jekyllrb.com/.toString()
        , source: "http://dossier:8048/verity/user/landing/index.js"
        }
      ]),
      createRequest: createXHRRequest(function () { return new XMLHttpRequest() }),
      injector: function (source) {
        if (source) {
          chrome.tabs.sendRequest(details.tabId, source, function () {});
        }
      }
    });
  }
});
