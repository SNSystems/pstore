(function () {
    'use strict';

    var uptime = new WebSocket ("ws://" + window.location.host + "/uptime");
    uptime.onerror = function (error) {
        console.error(error);
    };
    uptime.onclose = function () {
        console.log("uptime websocket closed.");
    };
    uptime.onopen = function () {
        console.log("uptime websocket open.");
    };
    uptime.onmessage = function (msg) {
        var obj = JSON.parse(msg.data);
        var el = document.getElementById("message");
        if (el !== null) {
            el.textContent = obj.uptime !== undefined ? obj.uptime : "Unknown";
        }
    };
} ());
