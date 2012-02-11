opera.extension.onmessage = function (event) {
  opera.postError(event.data.url);
  shouldTest(XHRRequest, event.data.url, function (source) {
    event.source.postMessage(source);
  });
};
