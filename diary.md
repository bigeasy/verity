# Verity Diary

Time to revisit.

Rendevnous server will sit and listen to you poll every five seconds. You tell
it where your local server is running. A listening browser will get the address
then start running the commands from the local server.

The local server runs for the duration of a test, then it dies.

The controller will notice a death and go back to the rendevous server and wait.

You get jQuery and Syn. If you want anything else, you're going to have to find
a way to define it. Maybe, if you've AMD wrapped your code, that might not be
that difficult, because we can load that for you.

The function you pass is parsed. Sent to the server.

There are wait conditions. These are functions that run once a second in the
host page. Controller injects code when the page lands.
