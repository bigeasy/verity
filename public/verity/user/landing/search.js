verity(function () {
  $(function () {
  Syn.click( {}, $('input.text') )
     .type( 'Hello World' )
     .click( {}, $('button.classy'), function () {
        $('button.classy').click();
     });
  });
});
