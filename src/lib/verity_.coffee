# TODO Send a favicon.ico.
# TODO Send 404.
# TODO Send 403.
# TODO Parameterize server.
connect       = require "connect"
{merge,extend}      = require("coffee-script").helpers
fs            = require "fs"
{parse}       = require "url"
model         = require "./model"
{Relatable}   = require "relatable"
{tz}          = require "timezone"

#settings = JSON.parse(fs.readFileSync("#{__dirname}/../settings.json", "utf8"))

die = (splat...) ->
  console.log.apply null, splat if splat.length
  process.exit 1
say = (splat...) -> console.log.apply null, splat

handlers = []

# Setup expected by connect middleware.
handlers.push (request, response, callback) ->
  request.originalUrl = request.url
  callback null

reactor       = require("./reactor").define()
# Create a connect based logger.
handlers.push connect.logger()
handlers.push connect.cookieParser()
handlers.push connect.session({ secret: "we are all correct and right-tighty" })
handlers.push connect.bodyParser()
handlers.push reactor.reaction (_) ->
  console.log { session: @request.session, body: @request.body }
handlers.push connect.csrf()

# Internal rewriting.
handlers.push reactor.reaction (_) ->
  # Parse the url and get the path name.
  @reaction.parsed = parse(@request.originalUrl, true)
  @reaction.path = @reaction.parsed.pathname

  # Create test object wrapped around session stored data.
  @reaction.test = model.createTest(@request.session.test, _)
  @request.session.test = @reaction.test.$

  # Continue.
  false

# Application page templates. These are pages, forms, addtional applications, as
# opposed to sections above, which aare templates for CMS database content.
handlers.push reactor.reaction (_)  ->
  if template = @findTemplate "/stencil", @reaction.path
    @sendTemplate template, @model({}), _
    true

handlers.push reactor.router (app) ->
  app.get "/test/boilerplate.js", (_) ->
    @sendText @reaction.test.boilerplate
    true

  app.post "/test/directive", (_) ->
    try
      { directive, base } = @request.body
      @reaction.test.directive(directive, base)
      @sendText "OK"
    catch e
      @sendText 400, "FAILURE #{e.message}"
    true

  app.get "/test/visit", (_) ->
    console.log @reaction.test.$.injections
    if injection = @reaction.test.visit(@reaction.parsed.query.url, _)
      console.log { injection }
      @sendText injection.response @request.session._csrf
    else
      @sendText "NONE"
    true

  app.get "/test/progress.js", (_) ->
    console.log query = parse(@request.url, true).query
    if (query.token is @reaction.test.$.token)
      console.log "VALID, WILL STORE"
    else
      console.log "INVALID, SEND 403"
    @sendJSONP query.callback, {}
    true

do ->
  send = (request, response, root, path, _callback) ->
    callback = (error, found) ->
      if not error? or
         error.message is "Cannot Transfer Directory" or
         error.code is "ENOENT"
        _callback null, found
      else
        _callback error
    connect.static.send(request, response, null, { root, path, callback })
    console.log response.statusCode

  statics = (prefix, suffix) ->
    reactor.reaction (_) ->
      root = if suffix? then "#{prefix}/#{@site(_).host}#{suffix}" else prefix
      path = @reaction.parsed.pathname
      send(@request, @response, root, path, _)

  # Common static resources.
  handlers.push statics("#{__dirname}/../public")

handlers.push (request, response, _) ->
  response.writeHead 200, { "Content-Type": "text/plain" }
  response.end "Hello, World!"
  true

http = require "http"

handler = (request, response, _) ->
  for attempt, i in handlers
    if attempt(request, response, _)
      return true
  return false

server = http.createServer (request, response) ->
  handler request, response, (error, handled) ->
    if error
      pounds = new Array(74).join "#"
      console.error "### ERROR #{pounds}"
      console.error "#{request.headers.host}#{request.url}"
      console.error error.stack
      console.error request
      response.writeHead 500, { "Content-Type": "text/plain" }
      response.end "X Oops!"

exports.server = -> server.listen(8078)
