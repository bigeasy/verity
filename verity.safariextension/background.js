function listener (event) {
  shouldTest(XHRRequest, event.message, function (source) {
    event.target.page.dispatchMessage("source", source);
  });
}
safari.application.addEventListener("message", listener, false);
