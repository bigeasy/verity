fs = require "fs"
indent = (string, padding) ->
  tab = new Array(padding + 1).join(" ")
  lines = []
  for line in string.split /\n/
    lines.push if /\S/.test(line) then tab + line else ""
  lines.join "\n"
task 'boilerplate', 'generate boilerplate', ->
  jquery = fs.readFileSync "jquery.js", "utf8" 
  controller = fs.readFileSync "controller.js", "utf8" 
  syn = fs.readFileSync "syn.js", "utf8" 
  boilerplate = """
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
  fs.writeFileSync "extension/boilerplate.js", "var boilerplate = #{JSON.stringify boilerplate};\n", "utf8"
