chrome.extension.onRequest.addListener(function (request, sender, sendResponse) {
  var script = document.createElement("script");
  script.setAttribute("type", "text/javascript");
  script.appendChild(document.createTextNode(request));
  document.getElementsByTagName("head")[0].appendChild(script);
  sendResponse({});
});
