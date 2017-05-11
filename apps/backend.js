"use strict";

var express = require('express');
var app = express();
var http = require('http');


app.get('/', function(req, res) {
    var total = 0;
    for (var i = 0; i < 10000; ++i) {
        total += i;
    }
    res.send(total.toString());
});

app.listen(3131, function() { console.log('listening on port 3131!') });
