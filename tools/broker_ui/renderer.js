//*                     _                     *
//*  _ __ ___ _ __   __| | ___ _ __ ___ _ __  *
//* | '__/ _ \ '_ \ / _` |/ _ \ '__/ _ \ '__| *
//* | | |  __/ | | | (_| |  __/ | |  __/ |    *
//* |_|  \___|_| |_|\__,_|\___|_|  \___|_|    *
//*                                           *
//===- tools/broker_ui/renderer.js ----------------------------------------===//
// Copyright (c) 2017-2020 by Sony Interactive Entertainment, Inc.
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
let d3 = require ('d3');
const {ipcRenderer} = require ('electron');
let $ = require ('jquery');

// This file is required by the index.html file and will
// be executed in the renderer process for that window.
// All of the Node.js APIs are available in this process.


const localhost = 'localhost:8080';
//var localhost = window.location.host;
function new_ws (channel, on_message) {
    const socket = new WebSocket ('ws://' + localhost + '/' + channel);
    socket.onerror = error => { console.error (error); };
    socket.onopen = () => { console.log ('connection open (' + channel + ')'); };
    socket.onclose = () => { console.log ('connection closed (' + channel + ')'); };
    socket.onmessage = on_message;
    return socket;
}

let prev_commits = 0;
let commits = 0;

function message_received (msg) {
    const obj = JSON.parse(msg.data);
    if (obj.hasOwnProperty('uptime')) {
        document.getElementById ('uptime').textContent = obj.uptime;
    }
    if (obj.hasOwnProperty('commits')) {
        commits = obj.commits;
        document.getElementById ('commits').textContent = obj.commits;
    }
}

new_ws ('uptime', message_received);
new_ws ('commits', message_received);

function series (pull) {
    const n = 20; // The number of data points shown.
    const timeFormat = '%H:%M:%S';
    const margin = {
        top: 20,
        right: 20,
        bottom: 20,
        left: 40,
    };
    const duration = 1000;
    const curve = d3.curveBasis; // d3.curveLinear
    const offset = 2; // set to 2 if curve===curveBasis, 1 if curveLinear

    const xDomain = (t) => {
        return [t - ((n - offset) * duration), t - (offset * duration)];
    };

    const data = [];

    const svg = d3.select ('svg');
    const width = +svg.attr ('width') - margin.left - margin.right;
    const height = +svg.attr ('height') - margin.top - margin.bottom;

    const x = d3.scaleLinear ()
        .domain (xDomain (Date.now ()))
        .range ([0, width]);

    const y = d3.scaleLinear ()
        .domain ([0, 1])
        .range ([height, 0]);

    const line = d3.line ()
        .x ((d) => { return x (d.time); })
        .y ((d) => { return y (d.value); })
        .curve (curve);

    const xAxisCall = d3.axisBottom (x).tickFormat (d3.timeFormat (timeFormat));
    const yAxisCall = d3.axisLeft (y);

    const g = svg.append ('g')
        .attr ('transform', 'translate(' + margin.left + ',' + margin.top + ')');
    g.append ('defs')
        .append ('clipPath')
        .attr ('id', 'clip')
        .append ('rect')
        .attr ('width', width)
        .attr ('height', height);

    const xAxis = g.append ('g')
        .attr ('class', 'axis axis-x')
        .attr ('transform', 'translate(0,' + y (0) + ')')
        .call (xAxisCall);

    const yAxis = g.append ('g')
        .attr ('class', 'axis axis-y')
        .call (yAxisCall);

    const path = g.append ('g')
        .attr ('clip-path', 'url(#clip)')
        .append ('path');
    path.datum (data)
        .attr ('class', 'line')
        .transition ()
        .duration (duration)
        .ease (d3.easeLinear)
        .on ('start', tick);

    function tick () {
        // Push a new data point onto the back.
        const time = Date.now ();
        data.push ({time: time, value: pull (time)});
        if (data.length > n) {
            data.shift ();
        }

        x.domain (xDomain (time));
        y.domain ([0, Math.max (1.0, d3.max (data, (d) => {
            return d.value;
        }))]);

        // Redraw the line.
        path.attr ('d', line).attr ('transform', null);

        const t = d3.transition ()
            .duration (duration)
            .ease (d3.easeLinear);

        xAxis.transition (t).call (xAxisCall);
        yAxis.transition (t).call (yAxisCall);

        d3.active (this)
            .attr ('transform', 'translate(' + (x (0) - x (duration)) + ',0)')
            .transition (t)
            .on ('start', tick);
    }
}

series ((time) => {
    const arrived = commits - prev_commits;
    $('#tps').text(arrived);
    prev_commits = commits;
    return arrived;
});

ipcRenderer.on('dark-mode', (event, message) => {
    document.documentElement.setAttribute('data-theme', message ? 'dark' : 'light');
});
