(function () {
    'use strict';

    //var localhost = 'localhost:8080';
    var localhost = window.location.host;
    var new_ws = function (channel, on_message) {
        var socket = new WebSocket ('ws://' + localhost + '/' + channel);
        socket.onerror = (error) => { console.error (error); };
        socket.onopen = () => { console.log ('connection open'); };
        socket.onclose = () => { console.log ('connection closed.') };
        socket.onmessage = on_message;
        return socket;
    };

    var uptime_socket = new_ws ('uptime', (msg) => {
        var obj = JSON.parse (msg.data);
        var el = document.getElementById ('message');
        console.log (obj.uptime);
        el.textContent = obj.uptime;
    });

    var commit_socket = new_ws ('commits', (msg) => {
        var obj = JSON.parse (msg.data);
        var el = document.getElementById ('commits');
        
        console.log (obj.commits);
        el.textContent = obj.commits;
    });

} ());
