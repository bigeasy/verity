function listener (event) {
  if (event.name == "source") {
    var script = document.createElement("script");
    script.setAttribute("type", "text/javascript");
    script.appendChild(document.createTextNode(event.message));
    document.getElementsByTagName("head")[0].appendChild(script);
  }
}

function loader () {
  safari.self.tab.dispatchMessage("loaded", window.location.href);
}

if (window.top === window) {
    safari.self.addEventListener("message", listener, true);
    document.addEventListener("DOMContentLoaded", loader, false);  
}
