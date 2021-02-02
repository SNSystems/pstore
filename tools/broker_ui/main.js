//*                  _        *
//*  _ __ ___   __ _(_)_ __   *
//* | '_ ` _ \ / _` | | '_ \  *
//* | | | | | | (_| | | | | | *
//* |_| |_| |_|\__,_|_|_| |_| *
//*                           *
//===- tools/broker_ui/main.js --------------------------------------------===//
//
// Part of the pstore project, under the Apache License v2.0 with LLVM Exceptions.
// See https://github.com/SNSystems/pstore/blob/master/LICENSE.txt for license
// information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
const {app, BrowserWindow, nativeTheme} = require ('electron');

// Keep a global reference of the window object, if you don't, the window will
// be closed automatically when the JavaScript object is garbage collected.
let main_window;

function dark_side () {
    main_window.webContents.send('dark-mode', nativeTheme.shouldUseDarkColors);
}
nativeTheme.on('updated', dark_side);

function createWindow () {
    // Create the browser window.

    main_window = new BrowserWindow ({
        width : 800,
        height : 600,
        show : false,
        webPreferences : {
            nodeIntegration : true,
            enableRemoteModule: true
        },
    });
    main_window.once('ready-to-show', () => {
        dark_side ();
        main_window.show();
    });

    // and load the index.html of the app.
    main_window.loadFile('index.html');

    // Open the DevTools.
    //main_window.webContents.openDevTools();

    // Emitted when the window is closed.
    main_window.on('closed', function () {
        // Dereference the window object, usually you would store windows
        // in an array if your app supports multi windows, this is the time
        // when you should delete the corresponding element.
        main_window = null;
    });
}

// This method will be called when Electron has finished
// initialization and is ready to create browser windows.
// Some APIs can only be used after this event occurs.
app.on('ready', createWindow);

// Quit when all windows are closed.
app.on('window-all-closed', function () {
    // On macOS it is common for applications and their menu bar
    // to stay active until the user quits explicitly with Cmd + Q
    if (process.platform !== 'darwin') {
        app.quit()
    }
});

app.on('activate', function () {
    // On macOS it's common to re-create a window in the app when the
    // dock icon is clicked and there are no other windows open.
    if (main_window === null) {
        createWindow ()
    }
});