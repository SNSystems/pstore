//*    _                                _       _    *
//*   (_) __ ___   ____ _ ___  ___ _ __(_)_ __ | |_  *
//*   | |/ _` \ \ / / _` / __|/ __| '__| | '_ \| __| *
//*   | | (_| |\ V / (_| \__ \ (__| |  | | |_) | |_  *
//*  _/ |\__,_| \_/ \__,_|___/\___|_|  |_| .__/ \__| *
//* |__/                                 |_|         *
//===- tools/brokerd/html/javascript.js -----------------------------------===//
// Copyright (c) 2017-2019 by Sony Interactive Entertainment, Inc.
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
