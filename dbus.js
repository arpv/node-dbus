/*
 * <copyright>
 * This file contains proprietary software owned by Motorola Mobility, Inc.<br/>
 * No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.<br/>
 * (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 * </copyright>
 * */

var events = require('events'),
    binding = require('./ndbus'),
    prop;

for (prop in binding.constants) {
  exports[prop] = binding.constants[prop];
}

if (!Array.isArray) {
  Object.defineProperty(Array, "isArray", {
      value: function(obj) {
          return Object.prototype.toString.call(obj) === "[object Array]";
      }
  });
}

exports.DBusMessage = Object.create(events.EventEmitter.prototype, {
  _inputArgs: {
    value: []
  },
  _signature: {
    value: null
  },
  bus: {
    value: binding.constants.DBUS_BUS_SYSTEM
  },
  type: {
    value: binding.constants.DBUS_MESSAGE_TYPE_INVALID
  },
  sender: {
    value: null
  },
  destination: {
    value: null
  },
  path: {
    value: null
  },
  iface: {
    value: null
  },
  member: {
    value: null
  },
  timeout: {
    value: -1
  },
  closeConnection: {
    value: function () {
      msgBus = this.bus;
      process.nextTick(function(){binding.deinit(msgBus);});
    }
  },
  appendArgs: {
    value: function (signature/*, arg1[, arg2, arg3...]*/) {
      if (arguments.length <= 1) {
        return false;
      }
      this.clearArgs();
      this._signature = signature;
      this._inputArgs = Array.prototype.slice.call(arguments, 1);
      return true;
    }
  },
  clearArgs: {
    value: function () {
      this._signature = null;
      this._inputArgs.splice(0, this._inputArgs.length);
    }
  },
  addMatch: {
    value: function () {
      try {
        if (this.type !== binding.constants.DBUS_MESSAGE_TYPE_SIGNAL) {
          throw {name: binding.constants.DBUS_ERROR_FAILED,
          message: 'Cannot add match rule. Message type must be a signal'};
        }
        binding.init.call(this);
        binding.addMatch.call(this);
      } catch (e) {
        this.emit('error', e);
      }
    }
  },
  removeMatch: {
    value: function () {
      try {
        if (this.type !== binding.constants.DBUS_MESSAGE_TYPE_SIGNAL) {
          throw {name: binding.constants.DBUS_ERROR_FAILED,
          message: 'Cannot remove match. Message type must be a signal'};
        }
        binding.removeMatch.call(this);
      } catch (e) {
        this.emit('error', e);
      }
    }
  },
  send: {
    value: function () {
      try {
        if (!this.type) {
          throw {name: binding.constants.DBUS_ERROR_FAILED,
          message: 'Invalid message type'};
        }
        binding.init.call(this);
        if (this.type === binding.constants.DBUS_MESSAGE_TYPE_SIGNAL) {
          binding.sendSignal.call(this);
        } else {
          binding.invokeMethod.call(this);
        }
      } catch (e) {
        this.emit('error', e);
      }
    }
  }
});

binding.onMethodResponse = function (args, error) {
  if (error) {
    this.emit('error', error);
  } else {
    args.unshift('methodResponse');
    this.emit.apply(this, args);
  }
};

binding.onSignalReceipt = function (objectList, args) {
  if (Array.isArray(objectList)) {
    var len = objectList.length,
        i = 0,
        caller;
    args.unshift('signalReceipt');
    for (; i<len; i++) {
      caller = objectList[i];
      caller.emit.apply(caller, args);
    }
  }
};
