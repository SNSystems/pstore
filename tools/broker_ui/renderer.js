// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.

var localhost = 'localhost:8080';
// var localhost = window.location.host;
function new_ws (channel, on_message) {
    var socket = new WebSocket ('ws://' + localhost + '/' + channel);
    socket.onerror = error => { console.error(error); };
    socket.onopen = () => { console.log('connection open (' + channel + ')'); };
    socket.onclose = () => { console.log('connection closed (' + channel + ')'); };
    socket.onmessage = on_message;
    return socket;
}

var commits = 0;

function message_received (msg) {
    var obj = JSON.parse(msg.data);
    if (obj.uptime != undefined) {
        document.getElementById('uptime').textContent = obj.uptime;
    }
    if (obj.commits != undefined) {
        commits = obj.commits;
        document.getElementById('commits').textContent = obj.commits;
    }
}

var uptime_socket = new_ws ('uptime', message_received);
var commit_socket = new_ws ('commits', message_received);

var prev_commits = 0;
setInterval (() => {
    var arrived = commits - prev_commits;
    var el = document.getElementById('tps');
    el.textContent = arrived;
    prev_commits = commits;
}, 1000);
