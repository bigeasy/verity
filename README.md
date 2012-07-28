# Verity NPAPI

This is the home of the Verity NPAPI plugin. For your entertainment, and for the
time being, let me entertain you with a programmer's journal.

## Getting Started

I chose CMake as my build configurator. You'll need CMake to build the project.

On OS X, install cmake using homebrew. Then

```console
$ cd build && cmake .. && make
```

I'm building using the XCode command line tools, GNU make, CMake. I'm having a
go at debugging this using logging, since I'm not sure I want to configure an
IDE for every platform on which I want to test this.

I've written a logging server in Node.js. Install Node.js and run it.

```console
node tracer.js
```

It will bind to a port on the localhost and the plugin will send UDP packets to
that port, if it is configured to debug. Configuring for debugging is done by
passing a port number to the plugin when it is created.

Logging messages are timestamped in the browser, they may arrive out of order,
but that is unlikely.

On OS X you install the plugin for development by symlinking a bundle.

```console
$ ln -s buildex/projects/FBTestPlugin/Debug/FBTestPlugin.plugin ~/Library/Internet Plug-Ins/
```

Surprises during OS X development: Output files put [way off in a corner
somewhere](http://stackoverflow.com/questions/5331270/why-doesnt-xcode-4-create-any-products).
Debugging wasn't a simple matter of attaching to Safari, because plug-ins are
run in a separate process. The FireBreath project did the hard work of sorting
out how to [launch a debugger on all the different platforms](o

Possibly useful for next steps.

 * [Sending and Receiving Packets](http://gafferongames.com/networking-for-game-programmers/sending-and-receiving-packets/).
 * [Using the Document Object Model from Objective-C](https://developer.apple.com/library/mac/#documentation/Cocoa/Conceptual/DisplayWebContent/Tasks/DOMObjCBindings.html).
 * [Creating Plug-ins with the Netscape API](https://developer.apple.com/library/mac/#documentation/InternetWeb/Conceptual/WebKit_PluginProgTopic/Tasks/NetscapePlugins.html#//apple_ref/doc/uid/30001250-BAJGJJAH)

 * [Hacker's guide to WebKit/GTK+](http://trac.webkit.org/wiki/HackingGtk).
