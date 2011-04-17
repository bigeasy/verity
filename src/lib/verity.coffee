fs          = require "fs"
http        = require "http"
url         = require "url"
sys         = require "sys"
{Client}    = require "mysql"
connect     = require "connect"
eco         = require "eco"
{loadObject}= require "async-object"
{tz}        = require "timezone"
{merge}     = require("coffee-script").helpers

configuration = JSON.parse(fs.readFileSync("#{__dirname}/../configuration.json", "utf8"))
port = configuration.ports[0]

client = null

# Constants for date math.
MINUTE  = 1000 * 60
HOUR    = MINUTE * 60
DAY     = HOUR * 24
WEEK    = DAY * 7

templates = {}
render = (template, model, params) ->
  if not templates[template]
    templates[template] = fs.readFileSync("#{__dirname}/../eco/#{template}", "utf8")
  include = (name, params) -> render(name, model, params)
  eco.render templates[template], merge { params, include }, model

class Reactor
  constructor: (@queries, @request, @response, @next) ->

  sendObject: (data) ->
    @response.writeHead 200, { "Content-Type": "application/json" }
    @response.end JSON.stringify(data, ((key, value) ->
      if (this[key] instanceof Date) then this[key].getTime() else value
    ), 2)

  sendTemplate: (template, model) ->
    templateMimeType = ->
      switch /\.(\w+)\.eco/.exec(template)[1]
        when "css" then "text/css"
        else "text/html"
    output = render(template, model, {})
    @response.writeHead 200, { "Content-Type": templateMimeType() }
    @response.end output

  sendRedirect: (location, header) ->
    @response.writeHead 301, merge { "Location": location, "Content-Type": "text/plain" }, header or {}
    @response.end location

  safely: (callback) ->
    response = @response
    ->
      try
        callback.apply null, Array.prototype.slice.call(arguments, 0)
      catch err
        console.error err
        console.error err.stack
        response.writeHead 500, { "Content-Type": "text/plain" }
        response.end "Oops!"

  sql: (query, parameters, get, callback) ->
    if not callback? and get?
      callback = get
      get = null
    @_sql(query, parameters, get, callback, 0)

  _sql: (query, parameters, get, callback, attempt) ->
    if client
      client.query @queries[query], parameters, @safely (error, results, fields) =>
        if error
          callback(error)
        else
          @convert results, fields
          if get
            expanded = []
            for result in results
              expanded.push @treeify result, get
          else
            expanded = results
          callback null, expanded, fields
    else
      mysql = configuration.databases.mysql
      client            = new Client()
      client.host       = mysql.hostname
      client.user       = mysql.user
      client.password   = mysql.password
      client.database   = mysql.name
      console.log client
      client.connect (error) =>
        throw error if error
        @_sql(query, parameters, get, callback, attempt)

  convert: (results, fields) ->
    for result in results
      for field of result
        if fields[field].fieldType is 0x10 and result[field] and result[field].length is 1
          result[field] = result[field].charCodeAt(0) is 1

  treeify: (record, get) ->
    tree = {}
    for key, value of record
      parts = key.split /__/
      branch = tree
      for i in [0...parts.length - 1]
        branch = branch[parts[i]] = branch[parts[i]] or {}
      branch[parts[parts.length - 1]] = record[key]
    tree[get]

  epoch: (date) ->
    if date
      if date.getTime
        date.getTime() / 1000
      else
        date / 1000
    else
      null

  # Keeping it simple and using form encoding and query strings. These methods
  # convert string parameters into numbers and booleans so that the code isn't
  # littered with `parseInt` and `is "true"`. They are an explicit statement on
  # the server side of the expected types.
  #
  # Posting JSON would be lighter, but I don't want to get in the habit of
  # defeating RESTfulness. It's nice to be able to test an API using `curl`.
  #
  # The `convert` method will accept both an array of fields or a string of
  # space delimited fields. Field names must not contain spaces and should be
  # valid JavaScript identifiers. Fields that are null or undefined are ignored.
  parameters: (map, fields, conversion) ->
    for field in fields
      for f in field.split /\s+/
        if map[f]?
          map[f] = conversion(map[f])
    map

  # Convert fields to integers.
  integers: (map, fields...) ->
    @parameters(map, fields, (f) -> parseInt f, 10)

  # Convert fields to booleans, true if the string value is "true".
  booleans: (map, fields...) ->
    @parameters(map, fields, (f) -> f is "true")

react = (->
  queries = {}
  for file in fs.readdirSync __dirname + "/../queries"
    queries[file.replace /.sql$/, ""] = fs.readFileSync __dirname + "/../queries/" + file , "utf8"
  (method) ->
    (request, response, next) ->
      reactor = new Reactor(queries, request, response, next)
      method.call reactor
)()


# You could use a flexible notion of this here, routes could call with a new
# reactor...
routes = (app) ->
  app.get "/verity", react ->
    @sendTemplate "welcome.html.eco", {}

  app.get "/users.js", react ->
    loadObject { people: @people() }, @safely (error, {people}) =>
      throw error if error
      @sendObject { people }

  app.get "/schedule/woof/:id", react ->
    params = [ @request.params.id ]
    @sql "getPersonByLegacyId", params, "person", @safely (error, people) =>
      throw error if error
      person = people.shift()
      if person
        @sendRedirect "/schedule/#{person.id}"
      else
        @next()

  app.get "/schedule/:id", react ->
    params = { personId: @request.params.id }
    start = @sundayOrLastSunday().getTime()
    params.start = @epoch(@sundayOrLastSunday().getTime())
    params.end = @epoch(start + WEEK)
    @collect params, "person, schedule", @safely (model) =>
      @sendTemplate "schedule.html.eco", merge model, { tz }

  app.get "/calendar", react ->
    if not @request.cookies.authenticated
      @sendRedirect "/login"
    else
      today = @sundayOrLastSunday()
      weeks = []
      for i in [0..1]
        week = []
        for j in [0..6]
          week.push today
          today = new Date(+(today) + DAY)
        weeks.push week
      @sendTemplate "calendar.html.eco", { weeks, tz }

  app.get "/login", react ->
    @sendTemplate "login.html.eco", {}

  app.get "/shifts.js", react ->
    query = url.parse(@request.url, true).query
    query = @integers(query, "from to")
    query.from or= @sundayOrLastSunday().getTime()
    query.to or= query.from + (5 * WEEK)

    @sql "getShifts", [ @epoch(query.from), @epoch(query.to) ], "shift", @safely (error, shifts) =>
      throw error if error
      @sendObject({ shifts })

  app.get "/now.js", react ->
    now = new Date()
    now.setUTCMilliseconds(0)
    now.setUTCSeconds(0)
    now.setUTCMinutes(0)
    now.setUTCHours(0)
    @sendObject({ now: now.getTime() })

  app.get "/prototypes", react ->
    @sql "getPrototypes", [], "prototype", @safely (error, prototypes) =>
      throw error if error
      @sendObject { prototypes }

  app.get "/prototype-shifts", react ->
    @sql "getShiftsByPrototype", [], "shift", @safely (error, shifts) =>
      throw error if error
      @sendObject { shifts }

  app.post "/authenticate", react ->
    if @request.body.password is "justice1998"
      @sendRedirect "/calendar", { "Set-Cookie": "authenticated=true" }
    else
      @sendRedirect "/login"

  app.post "/shift", react ->
      body = @request.body
      body = @integers(body, "shiftStart actualStart shiftLength personId prototypeId")

      shiftStart = body.shiftStart
      shiftEnd = shiftStart + (12 * HOUR)

      if not body.shiftLength
        body.shiftLength = 12 * HOUR

      if body.actualStart
        actualStart = body.actualStart
        actualEnd = actualStart + body.shiftLength
      else
        actualStart = shiftStart
        actualEnd = shiftEnd

      body.prototypeId or= 1
      console.log body

      @sql "deleteShift", [ body.prototypeId, @epoch(shiftStart), body.personId ], @safely (error, results) =>
        throw error if error
        params = [
          body.prototypeId
          @epoch(shiftStart)
          @epoch(shiftEnd)
          @epoch(actualEnd)
          body.personId
          body.note
          body.style
        ]
        @sql "insertShift", params, @safely (error, results) =>
          throw error if error
          if results.affectedRows is 0
            console.log "Unable to insert shift."
            console.log params
            process.exit 1
          @sendObject({ success: true })

  app.post "/shift-delete", react ->
    body = @request.body
    body = @integers(body, "shiftStart personId prototypeId")
    body.prototypeId or= 1

    @sql "deleteShift", [ body.prototypeId, @epoch(body.shiftStart), body.personId ], @safely (error, results) =>
      throw error if error
      @sendObject({ success: true })

  app.post "/prototype", react ->
    body = @request.body
    body = @integers(body, "id")
    if body.id
      @sql "updatePrototype", [ body.name, body.id ], @safely (error, results) =>
        throw error if error
        @sendObject({ success: true })
    else
      @sql "insertPrototype", [ body.name ], @safely (error, results) =>
        throw error if error
        @sendObject({ success: true, prototypeId: results.insertId })

server = connect.createServer(
  connect.logger(),
  connect.cookieParser(),
  connect.bodyParser(),
  connect.router(routes),
  connect.static(__dirname + "/../public"),
  (err, request, response, next) ->
    console.error err
    console.error err.stack
    response.writeHead 500, { "Content-Type": "text/plain" }
    response.end "Oops!"
)

module.exports.server = ->
  server.listen(port)
