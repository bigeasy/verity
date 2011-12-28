{Twinkie}       = require "./vendor/twinkie/lib/twinkie"

twinkie = new Twinkie
twinkie.ignore  "public/stylesheets", "public/javascripts", "configuration.json", "bin/*.js", "lib/*"
twinkie.source  /(\.swp|~)$/, "src", "vendor", "stencil"
twinkie.coffee  "src/lib",            "lib"
twinkie.coffee  "src/bin",            "bin"
twinkie.coffee  "src/javascripts",    "public/javascripts"
twinkie.coffee  "vendor/reactor/lib", "lib"
twinkie.less    "src/stylesheets",    "public/stylesheets"
twinkie.copy    "src/lib", "lib", /\.js$/
twinkie.copy    "src/javascripts", "public/javascripts", /\.js$/
twinkie.tasks task, "compile", "idl", "docco", "gitignore", "watch"
