Node-DBus
===============

The node-dbus project is a simple light-weight [NodeJS][] based wrapper over
some [libdbus][] api's which enables the developer to:

* perform synchronous method-calls on a service provider
* perform asynchronous method-calls on a service provider
* send signals on the message bus
* listen to signals propogated over the message bus

Note that it is not intended to be a full-blown one-to-one mapping
of the libdbus api. For that, you might want to look at [node-libdbus][]
which is relatively concrete.

Node-dbus provides a convinient Javascript object **DBusMessage**
which is used to perform the afore-mentioned chores with some restrictions
as mentioned under the relevant api description.

It has currently been tested only on the 32-bit [Ubuntu Lucid Lynx][LL] and
64-bit [Fedora15][F15] GNOME releases and thus should be good for other distros too.

License: BSD
===============

Copyright (c) 2011, Motorola Mobility, Inc

All Rights Reserved.

Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:

* Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
* Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
* Neither the name of Motorola Mobility nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

Dependencies:
===============

The list of dependencies include:

* [NodeJS][] - ofcourse (&gt;= v0.8.0)

  * for [NodeJS][] releases &gt;=v0.5.1 & &lt;=v0.7.9, pick branch v0.1.1

* [libdbus][] - ofcourse

  `apt-get install libdbus-1-dev`

  or the equivalent for your distro.

* [glib2.0][] - for the convinient data-structures

  `apt-get install libglib2.0-dev`

  or the equivalent for your distro.

Installation:
===============

If the dependencies are met,

if you have [NPM][] installed,

for [NodeJS][] releases &gt;=v0.8.0,

    npm install node-dbus

for [NodeJS][] releases &gt;=v0.5.1 & &lt;=v0.7.9,

    npm install node-dbus@0.1.1

if you have source and [NPM][], then from the main folder

    npm install .

otherwise from the main folder of source

    node-gyp configure build

DBusMessage:
===============

A generic object which represents a:

* synchronous method-call message
* asynchronous method-call message
* a dbus signal

based on the `type` property that has been set for it.

For signals and method-calls, it provides functions to append arguments to a
message, clear the appended arguments and send the message based on other
properties that have been set as documented further. It additionally provides
mechanism to listen to signals (only) which are sent over the message bus.

It is an instance of nodejs' [EventEmitter][].

It can be accessed as:

    var dbus = require('[path/to]node-dbus');

and then inherit your object from `dbus.DBusMessage` as per your preference. For example:

    var msg = Object.create(dbus.DBusMessage, {...});

Properties:
---------------

**type**: &lt;Integer&gt;

Indicates the type of message that will be created while sending.
Defaults to `DBUS_MESSAGE_TYPE_INVALID`.

Valid values include:

-   `DBUS_MESSAGE_TYPE_SIGNAL` \- a signal to be sent over the message bus.
-   `DBUS_MESSAGE_TYPE_METHOD_CALL` \- a synchronous method-call to be made to a service provider.
-   `DBUS_MESSAGE_TYPE_METHOD_RETURN` \- an asynchronous method-call to be made to a service provider.

    This sounds a bit wierd since this value actually represents an asynch reply-message
    in [libdbus][] world, but i wanted to keep parity with libdbus constants.

**bus**: &lt;Integer&gt;

Indicates the type of message bus on which the message will be sent.
Defaults to `DBUS_BUS_SYSTEM`. For using the session bus, set to `DBUS_BUS_SESSION`.

**address**: &lt;String&gt;

The remote address for obtaining a shared dbus connection from the bus. See [`dbus_connection_open()`][dbuscxnopen].
If not specified, default `bus` address is used.

**destination**: &lt;String&gt;

Name of the service provider that the message should be sent to.

Typically used for method-calls and the filtering match-rule for signals to be listened.

Refer the [D-Bus spec][] for conventions.

**path**: &lt;String&gt;

For method-calls, it represents the object path the message should be sent to.

Whereas for signals, it represents the path to the object emitting the signal.

Refer the [D-Bus spec][] for conventions.

**iface**: &lt;String&gt;

For method-calls, it is the service provider's interface to invoke the method on.

For signals, it indicates the interface the signal is emitted from.

Refer the [D-Bus spec][] for conventions.

**member**: &lt;String&gt;

Name of the signal to be sent or method to be invoked.

**sender**: &lt;String&gt;

It is the unique bus name of the connection which originated the message.

In node-dbus, it is only used to construct the match-rule that is used to filter
and listen to the signals that are passed over the message bus.

Refer the [D-Bus spec][] for conventions.

**timeout**: &lt;Integer&gt;

Used only for method calls. It is the timeout in milliseconds for which the method-call
shall wait for receiving reply to the message.

An `error` event will be triggered on the message object if the method-call times-out.

Defaults to -1 which indicates a sane default timeout to be used.

**variantPolicy**: &lt;Integer&gt;

Dictates the policy to be used when a variant type code supplied to `appendArgs()` method
is expected to contain data which is a container-type (only) such as an array or dict_entry.

Defaults to `NDBUS_VARIANT_POLICY_DEFAULT` which suggests that the variant's data-signature
will be `a{sv}` for a dict_entry or `av` for array.

If specified as `NDBUS_VARIANT_POLICY_SIMPLE`, the variant's data-signature will contain
the *basic* data-type of the *first* property's value of the JS object to be appended. For
example, if object to be appended is `{a:int b:int}`, the data-signature shall be `a{si}`
and so on for string's, bool's, array's and object's.

Methods:
--------------

**appendArgs(&lt;String&gt; signature, &lt;Any&gt; arg1, [...])**:

A DBus service provider may expect certain input arguments along with the method-call
it has exposed. Or an application may want to attach information to a signal it sends over
the message bus for interested listeners. This function facilitates the process by
attaching the information (input arguments) to the message before sending it.

Note that internally, the actual appending of arguments to the message (both signals and method-calls)
will only happen during the `send()` method. Hence it is possible to `clearArgs()` and/or
re-append arguments before `send()`.

It expects a data-type signature string as the first argument, which represents the
type of each input argument to the message payload, followed by valid input arguments
for the message in *EXACT* order of the types as mentioned in the signature.

If the signature is invalid or the order of input arguments does not match the signature,
an `error` event shall be emitted on the message object indicating the error that occurred.

For details on how the signature string should look like, please refer to tbe [D-Bus spec][].

Example:

    //If a method-call expects input arguments OR
    //a signal should be sent with arguments of type string and an integer
    msg.appendArgs('si', 'stringArg', 73);

It is important to note that dictionaries (`DBUS_TYPE_DICT_ENTRY`) are represented
as javascript objects.

    //if the signature should contain a string followed by a
    //dict entry of string and variant types
    msg.appendArgs('sa{sv}',
                   'Artist',
                   {name: 'Dave Mustaine', rating: 10, awesome: true});

NOTE:

As of now, only the following list of primitive data types from the [D-Bus spec][]
are supported for `appendArgs()` :

boolean, int32, uint32, int64, double, signature, object\_path, string,
array, dict\_entry (dictionary), and variant.

**clearArgs()**:

Clears any input arguments that were previously appended to the message.

NOTE:

- a call to `appenArgs()` with valid data will implicitly clear any previously appended args.
- internally, the actual appending of input arguments happens during `send()`

**send()**:

Sends the message which can either be a signal or a synch/asynch method-call
depending on the `type` specified, over the `bus`, taking into account the
other appropriate properties that have been set on the message.

It will append input arguments (if any) to the message before sending.

If something goes wrong, an `error` event shall be emitted on the
object indicating the error occurred.

For method-calls, if a non-erroneous reply is received, the event `methodResponse`
will be emitted on the message object and any output arguments which are expected
to be received from the method-call will be supplied along-with.

Refer to description of `methodResponse` event for details.

NOTE:

- for method-calls, `destination`, `path` and `member` MUST be set
- for signals, `path`, `iface` and `member` MUST be set

**addMatch()**:

Used for listening to messages which are traveling on the message bus.

It is a wrapper over the [libdbus][] api [`dbus_bus_add_match()`][dbbus] with some restrictions
for performance and simplicity.

Read the doc for [`dbus_bus_add_match()`][dbbus] carefully before proceeding further.

It is used for listening to signals only (at least for now; patches are welcome).
The match-rule for filtering the messages on the specified `bus` will be constructed
internally by node-dbus based on the properties `iface`, `member`, `path`, `sender`
and `destination` of the message object.

- Properties `iface` and `member` MUST be set
- whereas `path`, `sender` and `destination` are optional based on your filtering needs.
- Filtering based on arguments is not supported (at least for now; patches are welcome).

When a match (filter) for a signal is successfully added, node-dbus shall hold a reference
to the message object until it is `removeMatch()` 'ed.

If an error occurs, event `error` shall be emitted on the message object indicating the
error occurred.

When a signal that is being listened to is received on the message bus,event `signalReceipt`
shall be emitted on the message object along with the signal details and arguments (if any)
that were extracted from the signal.

A match (filter) for a particular signal based on a particular match-rule will be added only once.
That is, subsequent calls to this api for the same message object will do nothing, unless you
change the value of any one of the properties mentioned above.

It is recommended to create and manage separate message objects for different signals which
are to be listened so that it is easier to track them individually when they are received.

**removeMatch()**:

Stops listening to a signal, the match filter for which was added previously with `addMatch()`.

This will also remove the reference to the message object which node-dbus held during `addMatch()`.
Refer to the description of `addMatch()` for details.

It is a wrapper over the [libdbus][] api [`dbus_bus_remove_match()`][dbbus].

- Properties `iface` and `member` MUST be set

Care should be taken to make sure that values of `iface`, `member`, `path`, `destination`
and `sender` are exactly the same as they were specified when the match (filter) was added
for the message object. Otherwise, the match (filter) wont be removed and an `error` event
will be emitted on the message object.

**closeConnection()**:

Depending on the specified `bus` of the message object, this function shall

- remove the message filter and all signal watchers over the bus
- destroy the underlying dbus connection

Each time a `send()` or `addMatch()` is called, node-dbus automagically sets up a shared
dbus connection, adds a message filter on the bus and sets up internal data structures,
*IF* it has not been done before. This function will clean up all of it.

This must be used wisely, keeping in mind the fact that underneath, the actual cleanup
shall happen on the next iteration of the event loop (see nodejs' [process.nextTick][pnt]).

Thus if your code does:

    msg.closeConnection();
    //Following shall not throw an error,
    //but eventually the signal would not be listened to,
    //as the connection will close
    msg.addMatch();

whereas,

    msg.closeConnection();
    //Following shall work correctly,
    //but eventually the connection will close
    msg.send();

After a connection has been closed, a call to `send()` or `addMatch()` on a subsequent
iteration of the event loop, shall automatically set it up again.

NOTE:

Node-dbus sets up one connection each for a session and the system bus depending on the `bus`
of the message object. This connection is shared between all message objects that are created.
Thus a `closeConnection()` on any one object shall suffice, where if `bus` is `DBUS_BUS_SESSION`,
it will close the session bus and `DBUS_BUS_SYSTEM` will close the system bus.

Events:
---------------

**methodResponse**:

Emitted on the message object when a reply is received from either an asynch or sync method-call.

If the reply contains valid output arguments from the method call, then these arguments will
be supplied to the listener. Thus, the signature of the listener depends on the order in which
the output arguments are expected from the method-call's reply. Or if you are unsure,
then you just access them via the standard `arguments` javascript object.

**signalReceipt**:

Emitted on the message object when a signal is received on the message bus, which was
filtered via the `addMatch()` call.

The first argument is always an object with signal parameters.
If the signal contains valid data arguments, then those will be supplied to the listener.
Thus, the signature of the listener depends on the order in which the data arguments are
expected from the signal. Or if you are unsure, then you just access them via the
standard `arguments` javascript object.

**NOTE**:

As of now, for both `methodResponse` and `signalReceipt`, only the following list of
primitive data types from the [D-Bus spec][] will be extracted as arguments and
supplied to listener:

boolean, byte (uint8), uint16, uint32, uint64, int16, int32, int64, double,
signature, object\_path, string, struct, array, dict\_entry (dictionary)
and variant (which wraps one of the previous types)

*Some of the uncommon types like byte have NOT yet been tested and hence good luck!*

**error**:

The error event is emitted when something goes wrong during any of the operations
on the message object.

It may have been trigerred due to something as trivial as an invalid property
that was set on the object or an error response received from the daemon.

An error object shall be received in the listener which maps closely to the
DBusError format of the [libdbus][] world, where-in the object shall contain

- `name` &lt;String&gt;, which represents the error name as defined under
  the dbus protocol contants in libdbus. For example: `DBUS_ERROR_FAILED`
- `message` &lt;String&gt;, which describes the error in detail.

CONSTANTS:
---------------

The following list of constants are available for use and are directly exported from [libdbus][].
They can be accessed as properties on the exported object from dbus.js

    var dbus = require('[path/to]dbus');

For property `bus` of the message object,

- `dbus.DBUS_BUS_SESSION` = 0
  - Indicates use of the session bus.
- `dbus.DBUS_BUS_SYSTEM` = 1
  - Indicates use of the system bus. It is the default value.

For property `type` of the message object,

- `dbus.DBUS_MESSAGE_TYPE_INVALID` = 0
  - Represents an invalid message. It is the default value.
- `dbus.DBUS_MESSAGE_TYPE_METHOD_CALL` = 1
  - Indicates a synchronous method-call is intended.
- `dbus.DBUS_MESSAGE_TYPE_METHOD_RETURN` = 2
  - Indicates an asynchronous method-call is intended.
- `dbus.DBUS_MESSAGE_TYPE_ERROR` = 3
  - Currently un-used. Dont use it.
- `dbus.DBUS_MESSAGE_TYPE_SIGNAL` = 4
  - Indicates that a signal is intended to be sent or listened.

For property `variantPolicy` of the message object,

- `dbus.NDBUS_VARIANT_POLICY_DEFAULT` = 0
  - Refer to `variantPolicy` property description.
- `dbus.NDBUS_VARIANT_POLICY_SIMPLE` = 1
  - Refer to `variantPolicy` property description.

Additionally,

- `dbus.DBUS_SERVICE_DBUS` = 'org.freedesktop.DBus'
- `dbus.DBUS_PATH_DBUS` = '/org/freedesktop/DBus'
- `dbus.DBUS_PATH_LOCAL` = '/org/freedesktop/DBus/Local'
- `dbus.DBUS_INTERFACE_DBUS` = 'org.freedesktop.DBus'
- `dbus.DBUS_INTERFACE_LOCAL` = 'org.freedesktop.DBus.Local'
- `dbus.DBUS_INTERFACE_INTROSPECTABLE` = 'org.freedesktop.DBus.Introspectable'
- `dbus.DBUS_INTERFACE_PROPERTIES` = 'org.freedesktop.DBus.Properties'
- `dbus.DBUS_INTERFACE_PEER` = 'org.freedesktop.DBus.Peer'

and error `name` 's,

- `dbus.DBUS_ERROR_FAILED` = 'org.freedesktop.DBus.Error.Failed'
- `dbus.DBUS_ERROR_NO_MEMORY` = 'org.freedesktop.DBus.Error.NoMemory'
- `dbus.DBUS_ERROR_SERVICE_UNKNOWN` =  'org.freedesktop.DBus.Error.ServiceUnknown'
- `dbus.DBUS_ERROR_NAME_HAS_NO_OWNER` = 'org.freedesktop.DBus.Error.NameHasNoOwner'
- `dbus.DBUS_ERROR_NO_REPLY` = 'org.freedesktop.DBus.Error.NoReply'
- `dbus.DBUS_ERROR_IO_ERROR` = 'org.freedesktop.DBus.Error.IOError'
- `dbus.DBUS_ERROR_BAD_ADDRESS` = 'org.freedesktop.DBus.Error.BadAddress'
- `dbus.DBUS_ERROR_NOT_SUPPORTED` = 'org.freedesktop.DBus.Error.NotSupported'
- `dbus.DBUS_ERROR_LIMITS_EXCEEDED` = 'org.freedesktop.DBus.Error.LimitsExceeded'
- `dbus.DBUS_ERROR_ACCESS_DENIED` = 'org.freedesktop.DBus.Error.AccessDenied'
- `dbus.DBUS_ERROR_AUTH_FAILED` = 'org.freedesktop.DBus.Error.AuthFailed'
- `dbus.DBUS_ERROR_NO_SERVER` = 'org.freedesktop.DBus.Error.NoServer'
- `dbus.DBUS_ERROR_TIMEOUT` = 'org.freedesktop.DBus.Error.Timeout'
- `dbus.DBUS_ERROR_NO_NETWORK` = 'org.freedesktop.DBus.Error.NoNetwork'
- `dbus.DBUS_ERROR_ADDRESS_IN_USE` = 'org.freedesktop.DBus.Error.AddressInUse'
- `dbus.DBUS_ERROR_DISCONNECTED` = 'org.freedesktop.DBus.Error.Disconnected'
- `dbus.DBUS_ERROR_INVALID_ARGS` = 'org.freedesktop.DBus.Error.InvalidArgs'
- `dbus.DBUS_ERROR_FILE_NOT_FOUND` = 'org.freedesktop.DBus.Error.FileNotFound'
- `dbus.DBUS_ERROR_FILE_EXISTS` = 'org.freedesktop.DBus.Error.FileExists'
- `dbus.DBUS_ERROR_UNKNOWN_METHOD` = 'org.freedesktop.DBus.Error.UnknownMethod'
- `dbus.DBUS_ERROR_TIMED_OUT` = 'org.freedesktop.DBus.Error.TimedOut'
- `dbus.DBUS_ERROR_MATCH_RULE_NOT_FOUND` = 'org.freedesktop.DBus.Error.MatchRuleNotFound'
- `dbus.DBUS_ERROR_MATCH_RULE_INVALID` = 'org.freedesktop.DBus.Error.MatchRuleInvalid'
- `dbus.DBUS_ERROR_SPAWN_EXEC_FAILED` = 'org.freedesktop.DBus.Error.Spawn.ExecFailed'
- `dbus.DBUS_ERROR_SPAWN_FORK_FAILED` = 'org.freedesktop.DBus.Error.Spawn.ForkFailed'
- `dbus.DBUS_ERROR_SPAWN_CHILD_EXITED` = 'org.freedesktop.DBus.Error.Spawn.ChildExited'
- `dbus.DBUS_ERROR_SPAWN_CHILD_SIGNALED` = 'org.freedesktop.DBus.Error.Spawn.ChildSignaled'
- `dbus.DBUS_ERROR_SPAWN_FAILED` = 'org.freedesktop.DBus.Error.Spawn.Failed'
- `dbus.DBUS_ERROR_SPAWN_SETUP_FAILED` = 'org.freedesktop.DBus.Error.Spawn.FailedToSetup'
- `dbus.DBUS_ERROR_SPAWN_CONFIG_INVALID` = 'org.freedesktop.DBus.Error.Spawn.ConfigInvalid'
- `dbus.DBUS_ERROR_SPAWN_SERVICE_INVALID` = 'org.freedesktop.DBus.Error.Spawn.ServiceNotValid'
- `dbus.DBUS_ERROR_SPAWN_SERVICE_NOT_FOUND` = 'org.freedesktop.DBus.Error.Spawn.ServiceNotFound'
- `dbus.DBUS_ERROR_SPAWN_PERMISSIONS_INVALID` = 'org.freedesktop.DBus.Error.Spawn.PermissionsInvalid'
- `dbus.DBUS_ERROR_SPAWN_FILE_INVALID` = 'org.freedesktop.DBus.Error.Spawn.FileInvalid'
- `dbus.DBUS_ERROR_SPAWN_NO_MEMORY` = 'org.freedesktop.DBus.Error.Spawn.NoMemory'
- `dbus.DBUS_ERROR_UNIX_PROCESS_ID_UNKNOWN` = 'org.freedesktop.DBus.Error.UnixProcessIdUnknown'
- `dbus.DBUS_ERROR_INVALID_SIGNATURE` = 'org.freedesktop.DBus.Error.InvalidSignature'
- `dbus.DBUS_ERROR_INVALID_FILE_CONTENT` = 'org.freedesktop.DBus.Error.InvalidFileContent'
- `dbus.DBUS_ERROR_SELINUX_SECURITY_CONTEXT_UNKNOWN` = 'org.freedesktop.DBus.Error.SELinuxSecurityContextUnknown'
- `dbus.DBUS_ERROR_ADT_AUDIT_DATA_UNKNOWN` = 'org.freedesktop.DBus.Error.AdtAuditDataUnknown'
- `dbus.DBUS_ERROR_OBJECT_PATH_IN_USE` = 'org.freedesktop.DBus.Error.ObjectPathInUse'

[NodeJS]: http://nodejs.org/
[libdbus]: http://dbus.freedesktop.org/doc/api/html/index.html
[node-libdbus]: https://github.com/agnat/node_libdbus
[LL]: http://releases.ubuntu.com/lucid/
[F15]: http://fedoraproject.org/en/get-fedora-options
[glib2.0]: http://developer.gnome.org/glib/
[NPM]: http://npmjs.org/
[EventEmitter]: http://nodejs.org/docs/v0.4.7/api/events.html
[D-Bus spec]: http://dbus.freedesktop.org/doc/dbus-specification.html
[dbbus]: http://dbus.freedesktop.org/doc/api/html/group__DBusBus.html
[pnt]: http://nodejs.org/docs/latest/api/process.html#process.nextTick
[dbuscxnopen]: http://dbus.freedesktop.org/doc/api/html/group__DBusConnection.html#gacd32f819820266598c6b6847dfddaf9c
