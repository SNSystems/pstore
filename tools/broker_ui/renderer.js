//*                     _                     *
//*  _ __ ___ _ __   __| | ___ _ __ ___ _ __  *
//* | '__/ _ \ '_ \ / _` |/ _ \ '__/ _ \ '__| *
//* | | |  __/ | | | (_| |  __/ | |  __/ |    *
//* |_|  \___|_| |_|\__,_|\___|_|  \___|_|    *
//*                                           *
//===- tools/broker_ui/renderer.js ----------------------------------------===//
// Copyright (c) 2017-2021 by Sony Interactive Entertainment, Inc.
// All rights reserved.
//
// Developed by:
//   Toolchain Team
//   SN Systems, Ltd.
//   www.snsystems.com
//
// Permission is hereby granted, free of charge, to any person obtaining a
// copy of this software and associated documentation files (the
// "Software"), to deal with the Software without restriction, including
// without limitation the rights to use, copy, modify, merge, publish,
// distribute, sublicense, and/or sell copies of the Software, and to
// permit persons to whom the Software is furnished to do so, subject to
// the following conditions:
//
// - Redistributions of source code must retain the above copyright notice,
//   this list of conditions and the following disclaimers.
//
// - Redistributions in binary form must reproduce the above copyright
//   notice, this list of conditions and the following disclaimers in the
//   documentation and/or other materials provided with the distribution.
//
// - Neither the names of SN Systems Ltd., Sony Interactive Entertainment,
//   Inc. nor the names of its contributors may be used to endorse or
//   promote products derived from this Software without specific prior
//   written permission.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
// OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR
// ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT,
// TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE
// SOFTWARE OR THE USE OR OTHER DEALINGS WITH THE SOFTWARE.
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
