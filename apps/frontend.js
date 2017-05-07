"use strict";

var express = require('express')
var app = express()
var http = require('http');


app.get('/', function (req, res) {
    http.get("http://localhost:3131", (response) => {
        var body = '';
        response.on('data', (chunk) => { body += chunk; });
        response.on('end', () => {
            res.send("sum of first 1000 natural number: " + body);
        });
    });
})

app.listen(3000, function () {
    console.log('listening on port 3000!')
})
