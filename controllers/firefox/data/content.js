self.port.on("source", function(message) {
  var script = document.createElement("script");
  script.setAttribute("type", "text/javascript");
  script.appendChild(document.createTextNode(message));
  document.getElementsByTagName("head")[0].appendChild(script);
});
