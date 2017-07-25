var thrift = require('thrift'), Collector = require('./gen-nodejs/Collector');

const {Pool} = require('pg');
const pool = new Pool();

const UNIX_SOCKET = '/tmp/microtrace.thrift';

const SQL_INSERT = 'INSERT INTO spans(body) VALUES($1)';

var server = thrift.createServer(Collector, {
    Collect: function(log) {
        pool.query(SQL_INSERT, [log], (err, res) => {
            if (err) {
                console.err(err.stack);
            }
        });
    }
});

// Graceful shutdown for CTRL-C
process.on('SIGINT', function() {
    server.close();
    process.exit();
});

console.log('Listening on', UNIX_SOCKET);
server.listen(UNIX_SOCKET);
