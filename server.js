var connect = require("connect");

var app = connect()
  .use(connect.logger())
  .use(connect.static('public'))
  .listen(8078);
