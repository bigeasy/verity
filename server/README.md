# Verity

Cross-platform UI testing grid for web applications.

    Home:           [http://bigeasy.github.com/verity](http://bigeasy.github.com/verity)
    Source:         [http://github.com/bigeasy/verity](http://bigeasy.github.com/verity)
    Documentation:  [http://github.com/bigeasy/verity/wiki](http://github.com/bigeasy/verity/wiki)

    Issues:         [http://github.com/bigeasy/verity/issues](http://github.com/bigeasy/verity/issues)

    License:        The MIT License

    Status:         Pre-Release Development

## Purpose

Verity is a patchwork of code that creates a semi-automatic cross-browser web
user interface testing apparatus.

Here's how Verity works:

 * A Verity server runs a Node.js server from an OS X or Linux host, and tests
   web applications on any platform running in IE 6/10, Firefox, Chrome or Safari.
 * A test is a series of visits to web pages where a test script is injected
   into the web page.
 * The test script is wrapped in a scaffolding that gives it access to Syn for
   event simulation, jQuery for bombing around the DOM, and some assertion
   functions to assert that the UI does what you expect of it.
 * Each supported browser has an browser add-on that will pull a test script
   from a URL of your specification, wrap it in scaffolding and inject it into
   the page targeted for testing.
 * To use Verity, you run a browser for each platform you want to test. Not
   special hosting, just any place where you can run those browsers.
 * A Verity server issues test jobs to web browsers that visit the page with the
   add-on installed and configured for that particular test server.
 * The Verity server has a WebSocket powered dashboard that shows the results of
   tests as they come in from browsers.
 * Verity works with FuncUnit/Syn so that you have a JavaScript based automation
   langauge to write your tests in JavaScript, a language that you must already
   be familair with, if you're building web applications.

When you run Verity, you create your very own Verity grid. Agents that run your
tests in parallel when you submit them for testing.

What is an agent? An agent is any of the support browsers, but with the Verity
server installed.

How do you build a Verity Grid? Use VirtualBox, VMWare, Parallels to run guest
operating systems. Install the browsers you want to test on the guest operating
systems, and the add-ons onto those browsers. Navigate to the report server.
Now, leave it alone. The browser will accept jobs, run them, and return home to
the Verity Report Server.

Verity is a recipe with apparatus for OS X and Linux that simplifies the
necessary evil of testing Interent Explorer, simplifies cross-platform testing.

Verity is meant to run continuously, as you develop, instead of at check in. It
is part of your development enviornment, so you are constantly aware of the
health of your application, so that you are  able to jump in and fix problems as
they occur.
