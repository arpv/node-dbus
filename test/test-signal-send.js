#!/usr/bin/env node
/*
 * <copyright>
 * This file contains proprietary software owned by Motorola Mobility, Inc.<br/>
 * No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.<br/>
 * (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 * </copyright>
 * */

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
  type: {
    value: dbus.DBUS_MESSAGE_TYPE_SIGNAL
  }
});

dbusSignal.on ('error', function (error) {
  console.log ("[FAILED] ERROR -- ");
  console.log (error);
});

dbusSignal.appendArgs('sviasa{sv}',
                      'Coming from SESSION Bus!', true, 73,
                      ['strArray1','strArray2'],
                      {dictPropInt: 31, dictPropStr: 'dictionary', dictPropBool: true});
//send signal on session bus
//check signal receipt in 'test-signal-listener' process
//or on your terminal with $dbus-monitor --session
dbusSignal.send();

//switch to system bus and send
//check signal receipt in 'test-signal-listener' process
//or on your terminal with $dbus-monitor --session
dbusSignal.bus = dbus.DBUS_BUS_SYSTEM;
dbusSignal.appendArgs('sviasa{sv}',
                      'Coming from SYSTEM Bus!', 'variant', 37,
                      ['strArray3','strArray4'],
                      {dictPropInt: 999999});
dbusSignal.send();

