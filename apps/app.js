"use strict";

var express = require('express')
var app = express()
var http = require('http');


app.get('/', function (req, res) {
    http.get("http://www.google.com", (response) => {
        response.on('data', (chunk) => {});
        response.on('end', () => {

            http.get("http://www.facebook.com", (response) => {
                response.on('data', (chunk) => {});
                response.on('end', () => {
                    res.send('done');
                });
            });

        });
    });
    

})

app.listen(3000, function () {
    console.log('listening on port 3000!')
})
