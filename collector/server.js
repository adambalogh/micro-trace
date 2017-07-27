var thrift = require('thrift');
var Collector = require('./gen-nodejs/Collector');

var format = require('pg-format');
const {Pool} = require('pg');

const PORT = 9934;

const pool = new Pool({
    host: 'horton.elephantsql.com',
    user: 'dciohjpo',
    database: 'dciohjpo',
    port: 5432,
    password: 'yjfkAOIhJwx1JE-txPs1tFsdQ547RkPo'
});

const SQL_INSERT = 'INSERT INTO spans(json) VALUES %L';

var server = thrift.createServer(Collector, {
    Collect: function(logs) {
        for (var i = 0; i < logs.length; ++i) {
            logs[i] = [logs[i]];
        }
        var sql = format(SQL_INSERT, logs);
        pool.query(sql, (err, res) => {
            if (err) {
                console.log(err);
            } else {
                console.log(res);
            }
        });
    }
});

// Graceful shutdown for CTRL-C
process.on('SIGINT', function() {
    server.close();
    process.exit();
});

console.log('Listening on localhost:', PORT);
server.listen(PORT);
