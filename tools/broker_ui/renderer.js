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
import {series} from './series.js'
import * as ds from './node_modules/d3/dist/d3.min.js'
const bridgeAPI = window.bridgeAPI;

function newWS (port, channel, on_message) {
    const socket = new WebSocket ('ws://localhost:' + port + '/' + channel);
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
                return newWS (host, channel, on_message);
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

let prevCommits = 0;
let commits = 0;

series (time => {
    const arrived = commits - prevCommits;
    d3.select('#tps').text(arrived);
    prevCommits = commits;
    return arrived;
});

// There is a single for processing channel messages because I want to move to a model where there is a single
// WebSocket for all the channels. A JSON object is sent requesting subscription and unsubscription and each
// channel gets a member of the received message object.
function messageReceived (msg) {
    const obj = JSON.parse(msg.data);
    if (obj.hasOwnProperty('uptime')) {
        d3.select('#uptime').text(humanizeDuration(obj.uptime * 1000));
    }
    if (obj.hasOwnProperty('commits')) {
        commits = obj.commits;
        d3.select('#commits').text(commits);
    }
}

const port = 8080
//const localhost = window.location.host;
newWS (port, 'uptime', messageReceived);
newWS (port, 'commits', messageReceived);

bridgeAPI.ipcRenderer();
