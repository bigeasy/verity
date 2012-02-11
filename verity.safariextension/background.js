function listener (event) {
  shouldTest(event.message, function (source) {
    event.target.page.dispatchMessage("source", source);
  });
}
safari.application.addEventListener("message", listener, false);
