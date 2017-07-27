const express = require('express');
const app = express();
const sprintf = require('sprintf-js').sprintf;

const {Pool} = require('pg');

const pool = new Pool({
    host: 'horton.elephantsql.com',
    user: 'dciohjpo',
    database: 'dciohjpo',
    port: 5432,
    password: 'yjfkAOIhJwx1JE-txPs1tFsdQ547RkPo'
});


const trace_link = '<li><a href="trace/%1$d">Trace: %1$d</a></li>';

app.get('/', function(req, res) {
    var body = '<ul>';
    pool.query('SELECT id FROM traces', (err, response) => {
        response.rows.forEach(function(row) {
            const id = row.id;
            body += sprintf(trace_link, id);
        });
        body += '</ul';
        res.send(body);
    });
});

app.get('/traces/:traceId', function(req, res) {});

app.listen(
    3000, function() { console.log('Example app listening on port 3000!') });
