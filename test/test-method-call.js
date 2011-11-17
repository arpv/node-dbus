#!/usr/bin/env node
/*
 * <copyright>
 * This file contains proprietary software owned by Motorola Mobility, Inc.<br/>
 * No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.<br/>
 * (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 * </copyright>
 * */

var dbus = require('../../node-dbus');

var dbusMsg = Object.create(dbus.DBusMessage, {
  destination: {
    value: dbus.DBUS_SERVICE_DBUS
  },
  path: {
    value: dbus.DBUS_PATH_DBUS
  },
  iface: {
    value: dbus.DBUS_INTERFACE_DBUS
  },
  member: {
    value: 'GetNameOwner',
    writable: true
  },
  bus: {
    value: dbus.DBUS_BUS_SYSTEM
  },
  type: {
    value: dbus.DBUS_MESSAGE_TYPE_METHOD_RETURN,
    writable: true
  }
});

dbusMsg.on ("methodResponse", function (arg) {
  console.log ("[PASSED] Got method response with data ::");
  console.log (arguments);
});

dbusMsg.on ("error", function (error) {
  console.log ("[FAILED] ERROR -- ");
  console.log(error);
});

//Asynchronous call for 'GetNameOwner' which expects an argument of type string
dbusMsg.appendArgs('s', dbus.DBUS_SERVICE_DBUS);
dbusMsg.send();

//switch to synchronous call for 'ListNames' which does not expect any argument
dbusMsg.type = dbus.DBUS_MESSAGE_TYPE_METHOD_CALL;
dbusMsg.clearArgs();
dbusMsg.member = 'ListNames';
dbusMsg.send();
