chrome.extension.onRequest.addListener(function (request, sender, sendResponse) {
  var script = document.createElement("script");
  script.setAttribute("type", "text/javascript");
  script.setAttribute("src", request.uri);
  document.getElementsByTagName("head")[0].appendChild(script);
  sendResponse({});
});
