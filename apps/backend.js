"use strict";

var express = require('express');
var app = express();
var http = require('http');


app.get('/', function(req, res) {
    http.get("http://localhost:5000/", (response) => {
        var body = '';
        response.on('data', (chunk) => { body += chunk; });
        response.on('end', () => { res.send(body); });
    });
});

app.listen(3131, function() { console.log('listening on port 3131!') });
