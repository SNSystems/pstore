//===- tools/broker_ui/preload.js -----------------------------------------===//
//*                 _                 _  *
//*  _ __  _ __ ___| | ___   __ _  __| | *
//* | '_ \| '__/ _ \ |/ _ \ / _` |/ _` | *
//* | |_) | | |  __/ | (_) | (_| | (_| | *
//* | .__/|_|  \___|_|\___/ \__,_|\__,_| *
//* |_|                                  *
//===----------------------------------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
const {contextBridge, ipcRenderer, nativeTheme} = require ('electron')

contextBridge.exposeInMainWorld('bridgeAPI', {
    ipcRenderer : () => {
        ipcRenderer.on('dark-mode', (event, message) => {
            console.log('dark-mode message received')
            document.documentElement.setAttribute('data-theme', message ? 'dark' : 'light');
        })
    }
})
