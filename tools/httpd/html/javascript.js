(function () {
    'use strict';

    var ws = new WebSocket ("ws://" + window.location.host + "/socketserver", "json");
    ws.onerror = function (error) {
        console.error(error);
    };
    ws.onclose = function () {
        console.log ('socket closed.');
    };
    ws.onopen = function () {
        console.log ("connection open");
        ws.send("Hello server!");
    };
    ws.onmessage = function (msg) {
        $ ("#message").text(msg.data);
    };
} ());
