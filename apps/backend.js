"use strict";

var express = require('express')
var app = express()
var http = require('http');


app.get('/', function (req, res) {
    var sum = 0;
    for (var i = 1; i <= 1000; ++i) {
        sum += i;
    }
    res.send(sum.toString());
})

app.listen(3131, function () {
    console.log('listening on port 3131!')
})
