(function () {
    'use strict';

    var ws = new WebSocket ("ws://localhost:8080/socketserver", "json");
    ws.onerror = function (error) {
        console.log (error);
    };
    ws.onclose = function () {
        console.log ('socket closed.');
    };
    ws.onopen = function () {
        console.log ("connection open");
        ws.send ("Connection established. Hello server!");
    };
    ws.onmessage = function (msg) {
        console.log ("message received:", msg);
    };
} ());
