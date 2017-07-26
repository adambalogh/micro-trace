"use strict";

var express = require('express');
var app = express();
var http = require('http');

app.get('/', function(req, res) {
    http.get(
        {agent: false, hostname: "second-app-service.default", port: 8080},
        (response) => {
            var body = '';
            response.on('data', (chunk) => { body += chunk; });
            response.on('end', () => {
                res.send("sum of first 1000 natural number: " + body);
            });
        });
});

app.listen(8080, function() { console.log('listening on port 8080!') });
