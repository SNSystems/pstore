# Broker UI

This is an application built with [ElectronJS](http://electron.js.com). It provides a simple dashboard for the [pstore broker deamon](../brokerd).

To get started:

~~~bash
$ npm install
$ npm start
~~~

Note that at the moment it’s important to start `pstore-brokerd` _before_ starting this application. If the initial attempt to contact the broker fails, there’s currently no retry mechanism.
