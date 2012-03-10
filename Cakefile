edify = require("./edify/lib/edify")()
edify.language "coffee"
  lexer: "coffeescript"
  docco: "#"
  ignore: [ /^#!/, /^#\s+vim/ ]
edify.language "c"
  lexer: "c"
  divider: "/* --- EDIFY DIVIDER --- */"
  docco:
    start:  /^\s*\/\*\s*(.*)/
    end:    /^(.*)\*\//
    strip:  /^\s+\*\s*/
edify.parse "c", "./code/internet-explorer/VerityController", "./internet-explorer", /.*\.[hc]$/
edify.stencil /\/.*.[ch]$/, "stencil/docco.stencil"
edify.tasks task
