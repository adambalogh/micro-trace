const express = require('express');
const app = express();

const {Pool} = require('pg');

const pool = new Pool({
    host: 'horton.elephantsql.com',
    user: 'dciohjpo',
    database: 'dciohjpo',
    port: 5432,
    password: 'yjfkAOIhJwx1JE-txPs1tFsdQ547RkPo'
});


app.get('/', function(req, res) {
    pool.query("SELECT * FROM traces", (err, query) => {
        if (err) {
            console.log(err);
            res.send(err);
            return;
        }
        var body = '<ul>';
        for (row in query.rows) {
            body +=
                '<li><a href="trace/' + row + '">Trace: ' + row + '</a></li>';
        }
        body += '</ul>';

        res.send(body);
    });
});

app.get('/traces/:traceId', function(req, res) {
    const traceId = req.params.traceId;
    res.send(traceId);
});

app.listen(
    3000, function() { console.log('Example app listening on port 3000!') });
