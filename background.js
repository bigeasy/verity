chrome.webNavigation.onCompleted.addListener(function (details) {
  if (details.frameId == 0) {
    shouldTest(XHRRequest, details.url, function (source) {
      chrome.tabs.sendRequest(details.tabId, source, function () {});
    });
  }
});
