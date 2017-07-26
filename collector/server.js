var thrift = require('thrift'), Collector = require('./gen-nodejs/Collector');

const {Pool} = require('pg');
const pool = new Pool();

const PORT = 4138;

const SQL_INSERT = 'INSERT INTO spans(body) VALUES($1)';

var server = thrift.createServer(Collector, {
    Collect: function(logs) {
        for (var i = 0; i < logs.length; ++i) {
            pool.query(SQL_INSERT, [logs[i]], (err, res) => {
                if (err) {
                    console.err(err.stack);
                }
            });
        }
    }
});

// Graceful shutdown for CTRL-C
process.on('SIGINT', function() {
    server.close();
    process.exit();
});

console.log('Listening on', PORT);
server.listen(PORT, 'localhost');
