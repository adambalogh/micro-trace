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


const trace_link = '<li><a href="traces/%1$d">Trace: %1$d</a></li>';

/*
 * Index page
 */
app.get('/', function(req, res) {
    console.log('index');

    var body = '<html><head><title>MicroTrace</title></head>';
    body += '<body><h1>MicroTrace traces</h1>';
    body += '<ul>';

    pool.query('SELECT id FROM traces', (err, response) => {
        response.rows.forEach(function(row) {
            const id = row.id;
            body += sprintf(trace_link, id);
        });
        body += '</ul';
        body += '</body></html>'
        res.send(body);
    });
});

const LEVEL_DEPTH = 5;

function traverse(body, span, depth) {
    body += Array(depth).join('&nbsp');
    body += 'Call at ' + new Date(span.time * 1000) + '<br>';
    body += Array(depth).join('&nbsp');
    body += 'from: ' + span.client + ', to: ' + span.server + '<br>';
    body += '<hr>';


    for (var i = 0; i < span.callees.length; ++i) {
        body = traverse(body, span.callees[i], depth + LEVEL_DEPTH);
    }
    return body;
};

/*
 * Trace view page
 */
app.get('/traces/:traceId', function(req, res) {
    const traceId = req.params['traceId'];
    console.log('/traces/' + traceId);

    var body = '<html><head><title>Trace #' + traceId + '</title></head>';
    body += '<body>';
    body += '<h1>Trace #' + traceId + '</h1>';

    pool.query(
        'SELECT * FROM traces WHERE id = $1', [traceId], (err, response) => {
            const start_span = response.rows[0].body.start;
            body = traverse(body, start_span, 0);

            body += '</body></html>';
            res.send(body);
        });
});

app.listen(
    3000, function() { console.log('Example app listening on port 3000!') });

