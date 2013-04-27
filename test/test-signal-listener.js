#!/usr/bin/env node
/*
<copyright>
Copyright (c) 2011, Motorola Mobility, Inc

All Rights Reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

  - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
  - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
  - Neither the name of Motorola Mobility nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
</copyright>
*/

var dbus = require('../dbus'),
http = require('http');

var dbusMonitorSignal = Object.create(dbus.DBusMessage, {
  path: {
    value: '/org/ndbus/signaltest',
    writable: true
  },
  iface: {
    value: 'org.ndbus.signaltest',
    writable: true
  },
  member: {
    value: 'TestingNDbus',
    writable: true
  },
  bus: {
    value: dbus.DBUS_BUS_SESSION,
    writable: true
  },
  type: {
    value: dbus.DBUS_MESSAGE_TYPE_SIGNAL
  }
});

dbusMonitorSignal.on ("signalReceipt", function () {
  console.log ("[PASSED] Signal received with data :: ");
  console.log (arguments);
});

dbusMonitorSignal.on ("error", function (error) {
  console.log ("[FAILED] ERROR -- ");
  console.log(error);
});

//listen on session bus
dbusMonitorSignal.addMatch();
//dbusMonitorSignal.removeMatch(); //stop listening the session bus for this match rule

//listen on the system bus as well
dbusMonitorSignal.bus = dbus.DBUS_BUS_SYSTEM;
dbusMonitorSignal.addMatch();
//dbusMonitorSignal.removeMatch(); //stop listening the system bus for this match rule

/**** Run the 'test-signal-send' test-case to receive the signal and test ****/

var httpServer = http.createServer(function(){});
httpServer.listen(8084);

console.log("I have started listening to the the dbus signal 'TestingNDbus'.\nPls run the test-case 'test-signal-send' to to receive the signal here.");
