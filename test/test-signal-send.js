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

var dbus = require('../dbus');

var dbusSignal = Object.create(dbus.DBusMessage, {
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
  variantPolicy: {
    value: dbus.NDBUS_VARIANT_POLICY_DEFAULT,
    writable: true
  },
  type: {
    value: dbus.DBUS_MESSAGE_TYPE_SIGNAL
  }
});

dbusSignal.on ('error', function (error) {
  console.log ("[FAILED] ERROR -- ");
  console.log (error);
});

dbusSignal.appendArgs('svviasa{sv}',
                      'Coming from SESSION Bus!',
                      'non-container variant',
                      {type:'default variant policy', value:0, mixedPropTypes:true},
                      73,
                      ['strArray1','strArray2'],
                      {dictPropInt: 31, dictPropStr: 'dictionary', dictPropBool: true});
//send signal on session bus
//check signal receipt in 'test-signal-listener' process
//or on your terminal with $dbus-monitor --session
dbusSignal.send();

//switch to system bus and send
//check signal receipt in 'test-signal-listener' process
//or on your terminal with $dbus-monitor --system
dbusSignal.bus = dbus.DBUS_BUS_SYSTEM;
dbusSignal.variantPolicy = dbus.NDBUS_VARIANT_POLICY_SIMPLE;
dbusSignal.appendArgs('svviasa{sv}',
                      'Coming from SYSTEM Bus!',
                      88,
                      {type:'simple variant policy', value:'1', mixedPropTypes:'not allowed'},
                      37,
                      ['strArray3','strArray4'],
                      {dictPropInt: 999999});
dbusSignal.send();

