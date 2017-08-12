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

var sending = false;
var queue = [];

function send() {
    sending = true;
    var sql = format(SQL_INSERT, queue);
    queue = [];
    pool.query(sql, (err, res) => {
        sending = false;
        if (err) {
            console.log(err);
        } else {
            console.log(res);
        }

        if (queue.length) {
            send();
        }
    });
}

var server = thrift.createServer(Collector, {
    Collect: function(logs) {
        // convert each item into into array
        for (var i = 0; i < logs.length; ++i) {
            logs[i] = [logs[i]];
        }

        queue = queue.concat(logs);

        if (!sending) {
            send();
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
