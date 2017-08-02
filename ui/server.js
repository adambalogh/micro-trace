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
app.use(express.static('public'));

    const html_template =
    '<html><head>%s<link rel="stylesheet" href="/main.css"/>' +
    '</head><body>%s</body></html>';

const trace_link = '<a href="traces/%1$d">#%1$d</a>';

/*
 * Index page
 */
app.get('/', function(req, res) {
    console.log('index');

    pool.query('SELECT id, num_spans, duration FROM traces ORDER BY id DESC', (err, response) => {
        if (err) {
          console.log(err);
          res.send(err);
          return;
        }

        var body = '<h1>MicroTrace</h1><hr class="partial">';
        body += '<table id="traces">';
        body +=
            '<tr><th class="trace-id">Trace ID</th><th class="no-spans">Number of Spans</th>' +
            '<th class="duration">Total Duration</th></tr>';

        response.rows.forEach(function(row) {
            const id = row.id;
            body += '<tr>';
            body += '<td>' + sprintf(trace_link, id) + '</td>';
            body += '<td>' + row.num_spans + '</td>';
            body += '<td>' + row.duration + '</td>';
            body += '</tr>';
        });
        body += '</table>';
        res.send(
            sprintf(html_template, '<title>MicroTrace</title>', body));
    });
});


function formatDate(date) {
    return date.getHours() + ":" + date.getMinutes() + ":" + date.getSeconds() +
        " on " + date.getFullYear() + "/" + date.getMonth() + "/" +
        date.getDay();
}

const LEVEL_PADDING = 8;

function round(num) {
    return parseFloat(Math.round(num * 1000) / 1000).toFixed(3);
}

function traverse(body, span, depth) {
    body += '<div class="span">';
    body += Array(depth).join('&nbsp');
    body += 'From <i>' + span.client + '</i> to <i>' + span.server + '</i>';
    body += '<br>';
    body += Array(depth).join('&nbsp');
    body += '<i>time</i>: ' + formatDate(new Date(span.time * 1000)) 
      + ', <i>duration</i>: ' + round(span.duration) + 's<br>';
    body += '</div>';


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
            const trace = response.rows[0].body;
            var body = '<h1>Trace #' + traceId + '</h1>';

            body += '<div id="trace-info">';
            body += 'Number of spans: ' + trace.num_spans;
            body += ', Duration: ' + trace.duration;
            body += '</div>';

            body += '<div id="trace-view">';
            body += '<h3>Spans:</h3><hr>';
            const start_span = trace.start;
            body = traverse(body, start_span, 0);
            body += '</div>';
            res.send(
                sprintf(
                    html_template, '<title>Trace #' + traceId + '</title/>',
                    body));
        });
});

app.listen(
    3000, function() { console.log('Example app listening on port 3000!') });

