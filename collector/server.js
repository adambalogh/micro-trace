var thrift = require('thrift');
var Collector = require('./gen-nodejs/Collector');

const {Pool} = require('pg');

const PORT = 9934;

const pool = new Pool({
    host: 'horton.elephantsql.com',
    user: 'dciohjpo',
    database: 'dciohjpo',
    port: 5432,
    password: 'yjfkAOIhJwx1JE-txPs1tFsdQ547RkPo'
});

const SQL_INSERT = 'INSERT INTO spans(json) VALUES($1)';

var server = thrift.createServer(Collector, {
    Collect: function(logs) {
        console.log('Collect');
        for (var i = 0; i < logs.length; ++i) {
            pool.query(SQL_INSERT, [logs[i]], (err, res) => {
                if (err) {
                    console.log(err.stack);
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

console.log('Listening on localhost:', PORT);
server.listen(PORT);
