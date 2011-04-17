fs              = require("fs")
{exec, spawn}   = require("child_process")
path            = require("path")

currentBranch = (callback) ->
  branches =        ""
  git =             spawn "git", [ "branch" ]
  git.stdout.on     "data", (buffer) -> branches += buffer.toString()
  git.stderr.on     "data", (buffer) -> process.stdout.write buffer.toString()
  git.on            "exit", (status) ->
    process.exit(1) if status != 0
    branch = /\*\s+(.*)/.exec(branches)[1]
    callback(branch)

task "gitignore", "create a .gitignore for node-ec2 based on git branch", ->
  currentBranch (branch) ->
    gitignore = '''
                .gitignore
                .DS_Store
                **/.DS_Store
                configuration.json
                src/stylesheets/screen.less
                lib
                
                '''

    if branch is "gh-pages"
      gitignore += '''
                   '''
    else if branch is "ecma"
      gitignore += '''
                   documentation
                   index.html
                   '''
    fs.writeFile(".gitignore", gitignore)

task "index", "rebuild the Node IDL landing page.", ->
  idl     = require("idl")
  package = JSON.parse fs.readFileSync "package.json", "utf8"
  idl.generate "#{package.name}.idl", "index.html"

task "docco", "rebuild the CoffeeScript docco documentation.", ->
  exec "rm -rf documentation && docco src/*.coffee && cp -rf docs documentation && rm -r docs", (err) ->
    throw err if err

# Cheap make. Why not use real make? Because real make solves a more complicated
# problem, building an artifact that has multiple dependencies, which in turn
# have dependencies. Here we build artifacts that each have a single dependency,
# that is we build a JavaScript file from a single CoffeeScript file.
coffeeSearch = (from, to, commands) ->
  # Gather up the CoffeeScript files and directories in the source directory.
  files = []
  dirs = []
  for file in fs.readdirSync from
    if match = /^(.*).coffee$/.exec(file)
      file = match[1]
      source = "#{from}/#{file}.coffee"
      try
        if fs.statSync(source).mtime > fs.statSync("#{to}/#{file}.js").mtime
          files.push source
      catch e
        files.push source
    else
      try
        stat = fs.statSync "#{from}/#{file}"
        if stat.isDirectory()
          dirs.push file
      catch e
        console.log "Gee Wilikers."

  # Create the destination directory if it does not exist.
  if files.length
    try
      fs.statSync to
    catch e
      fs.mkdirSync to, parseInt(755, 8)
    commands.push [ "coffee", "-c -o #{to}".split(/\s/).concat(files) ]

  for dir in dirs
    coffeeSearch "#{from}/#{dir}",  "#{to}/#{dir}", commands

isDirty = (source, output) ->
  try
    fs.statSync(source).mtime > fs.statSync(output).mtime
  catch e
    true

ejsSearch = (from, to) ->
  dirs = []
  for file in fs.readdirSync from
    if match = /^(.*).ui$/.exec(file)
      dirty = false
      file = match[1]
      listing = "#{from}/#{file}.ui"
      output = "#{to}/#{file}_ui.js"
      if not dirty = isDirty(listing, output)
        for line in fs.readFileSync(listing, "utf8").split(/\n/)
          continue unless line = line.trim()
          source = "#{from}/#{line}"
          if dirty = isDirty("#{from}/#{line}", output)
            break
      if dirty
        try
          fs.statSync to
        catch e
          fs.mkdirSync to, parseInt(755, 8)
        templates = {}
        for line in fs.readFileSync(listing, "utf8").split(/\n/)
          continue unless line = line.trim()
          templates[/([^\/]+).ejs$/.exec(line)[1]] = fs.readFileSync("#{from}/#{line}", "utf8")
        fs.writeFileSync output, "var templates = #{JSON.stringify(templates, null, 2)};", "utf8"
    else
      try
        stat = fs.statSync "#{from}/#{file}"
        if stat.isDirectory()
          dirs.push file
      catch e
        console.warn "Cannot stat: #{from}/#{file}"
        throw e if e.number != process.binding("net").ENOENT
        console.warn "File disappeared: #{from}/#{file}"

  for dir in dirs
    ejsSearch "#{from}/#{dir}",  "#{to}/#{dir}"

lessSearch = (from, to, commands) ->
  # Gather up the CoffeeScript files and directories in the source directory.
  files = []
  dirs = []
  for file in fs.readdirSync from
    if match = /^(.*).less$/.exec(file)
      file = match[1]
      source = "#{from}/#{file}.less"
      try
        if fs.statSync(source).mtime > fs.statSync("#{to}/#{file}.css").mtime
          files.push source
      catch e
        files.push source
    else
      try
        stat = fs.statSync "#{from}/#{file}"
        if stat.isDirectory()
          dirs.push file
      catch e
        console.warn "Cannot stat: #{from}/#{file}"
        throw e if e.number != process.binding("net").ENOENT
        console.warn "File disappeared: #{from}/#{file}"

  # Create the destination directory if it does not exist.
  if files.length
    try
      fs.statSync to
    catch e
      fs.mkdirSync to, parseInt(755, 8)
    for file in files
      [ path, base ] = /^(.*)\/(.*).less$/.exec(file).slice(1)
      commands.push [ "lessc", [ source, "#{to}/#{base}.css" ] ]

  # Compile the files, then move onto the child directories.
  for dir in dirs
    coffeeSearch "#{from}/#{dir}",  "#{to}/#{dir}", commands

copySearch = (from, to, include, exclude, commands) ->
  if not commands? and (exclude instanceof Array)
    commands = exclude
    exclude = null
  # Gather up the CoffeeScript files and directories in the source directory.
  files = []
  dirs = []
  for file in fs.readdirSync from
    source = "#{from}/#{file}"
    if include.test(source) and not (exclude and exclude.test(source))
      try
        if fs.statSync(source).mtime > fs.statSync("#{to}/#{file}").mtime
          files.push source
      catch e
        files.push source
    else
      try
        stat = fs.statSync "#{from}/#{file}"
        if stat.isDirectory()
          dirs.push file # Create the destination directory if it does not exist.
      catch e
        console.warn "Cannot stat: #{from}/#{file}"
        throw e if e.number != process.binding("net").ENOENT
        console.warn "File disappeared: #{from}/#{file}"
  if files.length
    try
      fs.statSync to
    catch e
      fs.mkdirSync to, parseInt(755, 8)
    commands.push [ "cp", files.concat(to) ]

  for dir in dirs
    copySearch "#{from}/#{dir}",  "#{to}/#{dir}", include, exclude, commands

mkdir = (dirs...)  ->
  for dir in dirs
    path = ""
    for part in dir.split /\//
      path += part
      try
        fs.statSync(path)
      catch e
        fs.mkdirSync(path, 0775)
      path += "/"

compile = (callback) ->
  commands = []
  try
    stat = fs.statSync "src/stylesheets/screen.less"
  catch e
    commands.push [ "ln", [ "-s", "../../vendor/blueprint-css/blueprint/screen.css", "src/stylesheets/screen.less" ] ]
  mkdir "lib", "public/javascripts", "public/stylesheets", "eco", "queries"
  coffeeSearch "src/lib", "lib", commands
  coffeeSearch "src/javascripts", "public/javascripts", commands
  ejsSearch "src/javascripts", "public/javascripts"
  lessSearch "src/stylesheets", "public/stylesheets", commands
  copySearch "vendor/jquery/dist", "public/javascripts", /\.js$/, /\.min\.js$/, commands
  copySearch "src/javascripts", "public/javascripts", /\.js$/, commands
  copySearch "src/stylesheets", "public/stylesheets", /\.css$/, commands
  copySearch "src/stylesheets", "public/stylesheets", /\.png$/, commands
  index = 0
  next = ->
    if commands.length is 0
      callback() if callback?
    else
      command = commands.shift()
      command.push { customFds: [ 0, 1, 2 ] }
      less = spawn.apply null, command
      less.on "exit", (code) ->
        process.exit(code) unless code is 0
        next()
  next()

task "compile", "compile the CoffeeScript into JavaScript", ->
  compile()

sourceSearch = (update, exclude, splat...) ->
  [ dirs, files, sources ] = [ [], [], [] ]
  for dir in splat
    for file in fs.readdirSync dir
      try
        full = "#{dir}/#{file}"
        stat = fs.statSync(full)
        if stat.isDirectory()
          dirs.push full
        else if not exclude.test(full)
          files.push full
      catch e
        console.warn "Cannot stat: #{from}/#{file}"
        throw e if e.number != process.binding("net").ENOENT
        console.warn "File disappeared: #{from}/#{file}"
  for file in files
    fs.watchFile file, update
  if dirs.length
    sourceSearch.apply null, [ update, exclude ].concat(dirs)

server = null
process.on "exit", ->
  server.kill() if server

task "watch", "watch for changes, compile and restart server", ->
  status = invoke "compile"
  restart = ->
    server = spawn "node", [ "server.js" ], { customFds: [ 0, 1, 2 ] }
    server.on "exit", (code) ->
      server = null
      compile ->
        restart()
        process.stderr.write "\u0007### RESTARTED #{new Array(70).join("#")}\n"
  compile ->
    restart()
    update = (current, previous) ->
      if current.mtime.getTime() != previous.mtime.getTime()
        process.stderr.write "#{current.mtime} != #{previous.mtime} inode #{current.ino} #{previous.ino}\n"
        server.kill()
    sourceSearch update, /(\.swp|~)$/, "src", "queries", "eco"

task "clean", "rebuild the CoffeeScript docco documentation.", ->
  currentBranch (branch) ->
    if branch is "ecma"
      exec "rm -rf documentation lib _site", (err) ->
        throw err if err
