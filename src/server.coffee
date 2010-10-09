connect     = require "connect"
url         = require "url"
fs          = require "fs"
Client      = require("mysql").Client
nun         = require "nun"

port        = parseInt(process.argv[4] || "8086", 10)
mountPoint  = process.argv[3] || ""
domain      = process.argv[2] || "http://localhost#{if port is 80 then "" else port}"

createClient = ->
  client            = new Client()
  client.user       = "verity"
  client.password   = "verity"
  client.database   = "verity"

  client

select = (query, parameters, callback) ->
  client = createClient()
  client.connect ->
    client.query query, parameters, (error, results, fields) ->
      throw error if error
      callback results, fields
      client.end()

treeify = (record, get, split) ->
  split or= /__/
  tree = {}
  for key, value of record
    parts = key.split split
    branch = tree
    for i in [0...parts.length - 1]
      branch = branch[parts[i]] = branch[parts[i]] or {}
    branch[parts[parts.length - 1]] = record[key]
  tree[get]

templateMimeType = (template) ->
  switch /\.(\w+)\.nun/.exec(template)[1]
    when "css" then "text/css"
    else "text/html"

sendTemplate = (response, template, model) ->
  model.url = domain + mountPoint
  response.writeHead 200, { "Content-Type": templateMimeType(template) }
  nun.render __dirname + "/../templates/nun" + template, model, {}, (error, output) ->
    throw error if error
    output.on "data", (data) ->
      response.write data
    output.on "end", ->
      response.end ""

routes = (app) ->
  app.get "/verity", (request, response) ->
    select queries.configuration, [ 1 ], (results) ->
      console.log results
      configuration = treeify results[0], "configuration"
      sendTemplate response, "/verity.html.nun", configuration

provider = connect.staticProvider __dirname + "/../public"
staticProvider = (req, res, next) ->
  actual = req.url
  if req.method is "GET"
    parsed = url.parse actual
    if parsed.pathname.indexOf(mountPoint) == 0
      parsed.pathname = parsed.pathname.substring mountPoint.length
      req.url = url.format parsed

  result = provider req, res, next
  req.url = actual
  result

server = connect.createServer(
  connect.logger(),
  connect.bodyDecoder(),
  connect.router(routes),
  staticProvider
)

queries = {}
for file in fs.readdirSync __dirname + "/../queries"
  queries[file] = fs.readFileSync __dirname + "/../queries/" + file , "utf8"

server.listen port
