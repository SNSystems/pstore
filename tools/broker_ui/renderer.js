// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.

var localhost = 'localhost:8080';
// var localhost = window.location.host;
var new_ws = function (channel, on_message) {
    var socket = new WebSocket ('ws://' + localhost + '/' + channel);
    socket.onerror = error => { console.error(error); };
    socket.onopen = () => { console.log('connection open (' + channel + ')'); };
    socket.onclose = () => { console.log('connection closed (' + channel + ')'); };
    socket.onmessage = on_message;
    return socket;
};

var uptime_socket = new_ws ('uptime', msg => {
    var obj = JSON.parse(msg.data);
    var el = document.getElementById('message');
    console.log(obj.uptime);
    el.textContent = obj.uptime;
});

var commits = 0;
var commit_socket = new_ws ('commits', msg => {
    var obj = JSON.parse(msg.data);
    var el = document.getElementById('commits');
    console.log(obj.commits);
    commits = obj.commits;
    el.textContent = commits;
});

var prev_commits = 0;
setInterval (() => {
    var arrived = commits - prev_commits;
    var el = document.getElementById('tps');
    el.textContent = arrived;
    prev_commits = commits;
}, 1000);