// ==UserScript==
// @include http://*/*
// @include https://*/*
// ==/UserScript==

if (window.top == window) {
  window.addEventListener("DOMContentLoaded", function () {
    opera.extension.postMessage({ url: window.location.href });    
  });
}

opera.extension.onmessage = function(event) {
  var script = document.createElement("script");
  script.setAttribute("type", "text/javascript");
  script.appendChild(document.createTextNode(event.data));
  document.getElementsByTagName("head")[0].appendChild(script);
};
