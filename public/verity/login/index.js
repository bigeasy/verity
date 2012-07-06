verity(3, "jquery", function (doc) {
  verity.expect("twitter.com", "twitter.js");
  $("#twitter", doc).val("click");
})()
