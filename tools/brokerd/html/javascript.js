(function () {
    'use strict';

    //var localhost = 'localhost:8080';
    const localhost = window.location.host;
    function new_ws (channel, on_message) {
        var socket = new WebSocket ('ws://' + localhost + '/' + channel);
        socket.onerror = (error) => { console.error (error); };
        socket.onopen = () => { console.log ('connection open'); };
        socket.onclose = () => { console.log ('connection closed') };
        socket.onmessage = on_message;
        return socket;
    }

    window.onload = () => {
        const el = document.getElementById ('message');
        if (el !== null) {
            new_ws ('uptime', (msg) => {
                try {
                    const obj = JSON.parse (msg.data);
                    if (obj.hasOwnProperty ('uptime')) {
                        el.textContent = obj.uptime;
                    } else {
                        el.textContent = 'Unknown';
                    }
                } catch (e) {
                    if (e instanceof SyntaxError) {
                        el.textContent = 'Bad JSON';
                    } else {
                        throw e;
                    }
                }
            });
        }
    };
} ());
