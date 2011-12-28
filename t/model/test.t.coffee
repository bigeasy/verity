#!/usr/bin/env coffee-streamline
return if not require("streamline/module")(module)

require("./harness") 14, ({ model }, _) ->
  test = model.createTest(null, _)
  @equal test.$.id.length, 44, "test id generated"

  @ok not test.visit("http://www.google.com", _), "visit does not match"

  injection = test.visit("http://jekyllrb.com/", _)

  @ok injection, "visit match"

  @equal injection.id.length, 44, "injection id generation"

  @equal injection.response("csrf"), """
    http://dossier:8048/verity/user/landing/index.js #{injection.id} csrf
  """, "injection response text"

  response = test.directive("//@ when /^http:\\/\\/github.com\\/$/ http://tests.com/github/landing.js ", "http://www.google.com/")
  @ok response, "when regular expression"
  @ok response.target.test("http://github.com/"), "when regular expression match"
  @ok not response.target.test("http://github.com/login"), "when regular expression mismatch"
  @equal response.source, "http://tests.com/github/landing.js", "when source url"

  response = test.directive("//@ when \"http://github.com/\"  http://tests.com/github/landing.js ", "http://www.google.com/")
  @ok response, "when string"
  @ok response.target.test("http://github.com/"), "when string match"
  @ok not response.target.test("http://github.com/login"), "when string mismatch"
  @equal response.source, "http://tests.com/github/landing.js", "when source url"

  response = test.directive("//@ when \"http://github.com/\"  ./landing.js ", "http://www.google.com/search")
  @equal response.source, "http://www.google.com/landing.js", "when relative source url"
