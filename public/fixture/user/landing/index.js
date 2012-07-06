//@ include ./alert.js
//@ expect  0
//@ when    "https://github.com/search" ./search.js
verity(function () {
  window.location.href = "https://github.com/search";
});
