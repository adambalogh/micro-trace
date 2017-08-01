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

// App settings
app.use(express.static('public'))

const html_template = '<html><head>%s<link rel="stylesheet" href="/main.css"/>'
  + '</head><body>%s</body></html>';

const trace_link = '<a href="traces/%1$d">#%1$d</a>';

/*
 * Index page
 */
app.get('/', function(req, res) {
    console.log('index');

    pool.query('SELECT id FROM traces', (err, response) => {
        var body = '<h1>MicroTrace</h1><hr>';
        body += '<table id="traces">';
        body += '<tr><th class="trace-id">Trace ID</th><th class="no-spans">Number of Spans</th>'
            + '<th class="duration">Total Duration</th></tr>';

        response.rows.forEach(function(row) {
            const id = row.id;
            body += '<tr>';
            body += '<td>' + sprintf(trace_link, id) + '</td>';
            body += '<td>5</td>';
            body += '<td>1s</td>';
            body += '</tr>';
        });
        body += '</table>';
        body += '</body></html>'
        res.send(sprintf(html_template, '<title>MicroTrace</title>', body));
    });
});

const LEVEL_PADDING = 8;

function formatDate(date) {
  return date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds()
    + " on " + date.getFullYear() + "/" + date.getMonth() + "/" + date.getDay();
}

function traverse(body, span, depth) {
    body += Array(depth).join('&nbsp');
    body += 'Call at ' + formatDate(new Date(span.time * 1000)) + '<br>';
    body += Array(depth).join('&nbsp');
    body += 'from: ' + span.client + ', to: ' + span.server + '<br>';
    body += '<hr>';


    for (var i = 0; i < span.callees.length; ++i) {
        body = traverse(body, span.callees[i], depth + LEVEL_PADDING);
    }
    return body;
};

/*
 * Trace view page
 */
app.get('/traces/:traceId', function(req, res) {
    const traceId = req.params['traceId'];
    console.log('/traces/' + traceId);

    pool.query(
        'SELECT * FROM traces WHERE id = $1', [traceId], (err, response) => {
            var body = '<h1>Trace #' + traceId + '</h1>';
            const start_span = response.rows[0].body.start;
            body = traverse(body, start_span, 0);
            res.send(sprintf(html_template,
                '<title>Trace #' + traceId + '</title/>', body));
        });
});

app.listen(
    3000, function() { console.log('Example app listening on port 3000!') });

