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

var events = require('events'),
    binding = require('./build/Release/ndbus'),
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
    value: [],
    writable: true
  },
  _signature: {
    value: null,
    writable: true
  },
  bus: {
    value: binding.constants.DBUS_BUS_SYSTEM
  },
  type: {
    value: binding.constants.DBUS_MESSAGE_TYPE_INVALID
  },
  address: {
    value: null
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
  variantPolicy: {
    value: binding.constants.NDBUS_VARIANT_POLICY_DEFAULT
  },
  closeConnection: {
    value: function () {
      var msgBus = this.bus;
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

binding.onSignalReceipt = function (objectList, signal, args) {
  if (Array.isArray(objectList)) {
    var len = objectList.length,
        i = 0,
        caller;
    args.unshift('signalReceipt', signal);
    for (; i<len; i++) {
      caller = objectList[i];
      caller.emit.apply(caller, args);
    }
  }
};
