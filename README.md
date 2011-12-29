# Verity Cubby

This server is run by a NPAPI plugin so that the Verity testing add-ons can
inject the tests into the context of the page using a script element.

This server is implemented using
[mongoose.c](http://code.google.com/p/mongoose/). The NPAPI runs this server a a
separate process.

When the process starts, it emits a 64 character shutdown key to standard out.

```
$ bin/cubby &
OfhOd0zhOjpmgRBD0cH8J9SNAXBOlwr7MIVXItevKBR0tsCtDjAmrq71nJNGfeLZ
```

Before you can put something into a cubby, you need a token. This is an HTTP get
request that returns a 64 character token as `text/plain`. Verity extensions
call this from the extension code. If it is invoked from browser JavaScript,
in an iframe for example, token cannot be read due to the same origin policy.

When creating a token the extension specifies a script name without the `.js`
suffix, the uri of the page that will request the script.

```
$ curl -s 'http://localhost:8089/cubby/token?uri=http://foo.com/login&name=verity'
vP9uN6AQT3Lf58huf2iCByz5RKMhyUQ4HZyu46kX73cc9sFmsXY3vv7kgTAMOqQv
```

Once it has a token, the extension can put JavaScript into the cubby.

```
$ curl -d 'alert("Hello, World!");' \
    -s 'http://localhost:8089/cubby/data?vP9uN6AQT3Lf58huf2iCByz5RKMhyUQ4HZyu46kX73cc9sFmsXY3vv7kgTAMOqQv'
Stashed.
```

The extension then adds a script element to the page being tested. The cubby
server will serve the stashed content as `text/javascript`.

```
$ curl -e "http://foo.com/login" -s "http://localhost:8089/cubby/verity.js"
alert("Hello, World!");
```

When the browser closes, the NPAPI plugin will trigger an orderly shutdown of
the cubby server with the shutdown key.

```
$ curl -s "http://localhost:8089/echo/shutdown?"$key
Goodbye.
```

### Notes

The cubby server runs two threads in addition to the main tread of execution. A
listener thread and a worker thread.

The cubby server sets the number of workers to one worker, instead of the
Mongoose default of 10 threads. Multiple workers would require the cubby server
to use mutexes to be thread-safe. A single worker thread means that the cubby
worker does not need to synchronize its list of pages. One thread should be
enough. The request volume on the cubby server will be trivial.

Cubby will store 128 JavaScripts. When the 129th JavaScript is stashed in the
cubby server, the server will evict the least recently requested script, freeing
the memory allocated to store the script and metadata.
