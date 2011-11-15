#!/usr/bin/env node
/*
 * <copyright>
 * This file contains proprietary software owned by Motorola Mobility, Inc.<br/>
 * No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.<br/>
 * (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 * </copyright>
 * */

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
