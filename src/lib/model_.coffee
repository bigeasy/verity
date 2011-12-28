fs = require "fs"
{parse} = require "url"

generateToken = do ->
  cache = []
  (_) ->
    if cache.length
      cache.shift()
    else
      fd = fs.open "/dev/urandom", "r", _
      buffer = new Buffer(32)
      for i in [1..128]
        fs.read fd, buffer, 0, buffer.length, 0, _
        cache.push buffer.toString("base64")
      fs.close fd
      cache.shift()

exports.createTest = (session, _) ->
  Test::boilerplate or= do ->
    indent = (string, padding) ->
      tab = new Array(padding + 1).join(" ")
      lines = []
      for line in string.split /\n/
        lines.push if /\S/.test(line) then tab + line else ""
      lines.join "\n"
    jquery     = fs.readFile "#{__dirname}/../public/javascripts/jquery.js",     "utf8", _
    syn        = fs.readFile "#{__dirname}/../public/javascripts/syn.js",        "utf8", _
    controller = fs.readFile "#{__dirname}/../public/javascripts/controller.js", "utf8", _
    """
      (function () {
        // Begin jQuery. /////////////////////////////////////////////////

        (function () {
      #{indent jquery, 4}
        })();

        // End jQuery. ///////////////////////////////////////////////////

        // Begin Syn. ////////////////////////////////////////////////////

        // TODO: Once you get this running, stash and restore FuncUnit.
        window.FuncUnit = {};
        window.FuncUnit.jquery = jQuery.noConflict(true);

      #{indent syn, 2}

        // End Syn. //////////////////////////////////////////////////////

        // Begin Runner. /////////////////////////////////////////////////

      #{indent controller, 2}

        // End Runner. ///////////////////////////////////////////////////
      (function ($) { 

      _X_V_X_=_X_V_X_

      })(window.FuncUnit.jquery);
        // End Runner. ///////////////////////////////////////////////////
      })();
    """
  if session
    test = new Test(session)
  else
    test = new Test()
    test.$.id = generateToken(_)
  test

bogusStartForNow =
  target: /jekyllrb.com/
  source: "http://dossier:8048/verity/user/landing/index.js"

class Test
  constructor: (@$) ->
    @$ or= injections: []
  createScriptInjection: (source, _) ->
    injection = new ScriptInjection(@, source)
    @$.token = injection.id = generateToken(_)
    injection
  visit: (url, _) ->
    if url
      for entry, index in @$.injections.concat(bogusStartForNow)
        if new RegExp(entry.target).test url
          injection = @createScriptInjection(entry.source, _)
          @$.injections.splice(index, 1)
          return injection
    null
  _inject: (value, base) ->
    match = ///
      ^
      \s*
      (?:
        '((?:[^\\']|\\.)+)'
        |
        "((?:[^\\"]|\\.)+)"
        |
        /((?:[^\\/]|\\.)+)/
      )
      \s+
      (\S+)
      \s*
      $
    ///.exec(value)
    if match
      if target = match[1] or match[2]
        target = target.replace(/\\(.)/g, "$1")
        specials = [
          '/', '.', '*', '+', '?', '|',
          '(', ')', '[', ']', '{', '}', '\\'
        ]
        specials = new RegExp(
          '(\\' + specials.join('|\\') + ')', 'g'
        )
        target = target.replace(specials, '\\$1')
        target = "^#{target}$"
      else
        target = match[3]
      source = require("url").resolve(base, match[4])
      @$.injections.unshift { target, source }
      @$.injections[0]
    else
      throw new Error("Bad when directive.")
  directive: (directive, base) ->
    match = ///
      ^
      \s*
      //@
      \s+
      (\S*)
      \s+
      (.*)
      $
    ///.exec(directive)
    if not match
      throw new Error("Malformed directive line.")
    [ name, value ] = match[1..]
    switch name
      when "when" then @_inject(value, base)
      when "expect" then console.log directive
      when "include" then console.log directive
      else throw new Error("Unknown directive.")

class ScriptInjection
  constructor: (@test, @source) ->
  response: (csrf) -> [ @source, @id, csrf ].join(" ")
