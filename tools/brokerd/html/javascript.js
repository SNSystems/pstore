//*    _                                _       _    *
//*   (_) __ ___   ____ _ ___  ___ _ __(_)_ __ | |_  *
//*   | |/ _` \ \ / / _` / __|/ __| '__| | '_ \| __| *
//*   | | (_| |\ V / (_| \__ \ (__| |  | | |_) | |_  *
//*  _/ |\__,_| \_/ \__,_|___/\___|_|  |_| .__/ \__| *
//* |__/                                 |_|         *
//===- tools/brokerd/html/javascript.js -----------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
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
