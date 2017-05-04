"use strict";

var express = require('express')
var app = express()
var http = require('http');


app.get('/', function (req, res) {
    http.get("http://www.google.com", (response) => {
        let rawData = '';
        response.on('data', (chunk) => { rawData += chunk; });
        response.on('end', () => {
            res.send(rawData);
        });
    });
})

app.listen(3000, function () {
    console.log('listening on port 3000!')
})
