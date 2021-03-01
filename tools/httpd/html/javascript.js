//===- tools/httpd/html/javascript.js -------------------------------------===//
//*    _                                _       _    *
//*   (_) __ ___   ____ _ ___  ___ _ __(_)_ __ | |_  *
//*   | |/ _` \ \ / / _` / __|/ __| '__| | '_ \| __| *
//*   | | (_| |\ V / (_| \__ \ (__| |  | | |_) | |_  *
//*  _/ |\__,_| \_/ \__,_|___/\___|_|  |_| .__/ \__| *
//* |__/                                 |_|         *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
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
            el.textContent = obj.uptime !== undefined ? obj.uptime : 'Unknown';
        }
    };

    window.onload = () => {
        var request = new XMLHttpRequest ();
        request.open('GET', 'http://' + window.location.host + '/cmd/version');
        request.responseType = 'json';
        request.onload = () => {
            var el = document.getElementById('version');
            if (el !== null) {
                el.textContent = request.response.version;
            }
        };
        request.send();
    };

} ());
