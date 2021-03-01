//===- tools/broker_ui/renderer.js ----------------------------------------===//
//*                     _                     *
//*  _ __ ___ _ __   __| | ___ _ __ ___ _ __  *
//* | '__/ _ \ '_ \ / _` |/ _ \ '__/ _ \ '__| *
//* | | |  __/ | | | (_| |  __/ | |  __/ |    *
//* |_|  \___|_| |_|\__,_|\___|_|  \___|_|    *
//*                                           *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
const {ipcRenderer} = require ('electron');
const {dialog} = require ('electron').remote;
let $ = require ('jquery');
const {series} = require('./series');

function new_ws (host, channel, on_message) {
    const socket = new WebSocket ('ws://' + host + '/' + channel);
    socket.onerror = event => {
        dialog.showMessageBox({
            type: "error",
            buttons: [ "Quit", "Retry" ],
            message: "Connection to socket failed",
            detail: 'Unable to contact the pstore-brokerd process at ' + host + '. Is it running?',
            cancelId: 0,
            defaultId: 1
        }).then (p => {
            console.log ('button clicked was ', p.response);
            if (p.response === 1) {
                return new_ws (host, channel, on_message);
            }
        }).catch (error => {
            console.error (error);
        });
    };
    socket.onopen = () => { console.log ('connection open (' + channel + ')'); };
    socket.onclose = () => { console.log ('connection closed (' + channel + ')'); };
    socket.onmessage = on_message;
    return socket;
}

let prev_commits = 0;
let commits = 0;

series ((time) => {
    const arrived = commits - prev_commits;
    $('#tps').text(arrived);
    prev_commits = commits;
    return arrived;
});

// There is a single for processing channel messages because I want to move to a model where there is a single
// WebSocket for all the channels. A JSON object is sent requesting subscription and unsubscription and each
// channel gets a member of the received message object.
function message_received (msg) {
    const obj = JSON.parse(msg.data);
    if (obj.hasOwnProperty('uptime')) {
        $('#uptime').text (moment.duration(obj.uptime, 'seconds').humanize());
    }
    if (obj.hasOwnProperty('commits')) {
        commits = obj.commits;
        $('#commits').text (obj.commits);
    }
}

const localhost = 'localhost:8080';
//const localhost = window.location.host;
new_ws (localhost,'uptime', message_received);
new_ws (localhost,'commits', message_received);

ipcRenderer.on('dark-mode', (event, message) => {
    document.documentElement.setAttribute('data-theme', message ? 'dark' : 'light');
});
