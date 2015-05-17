/*
 * Copyright (c) 2011, Motorola Mobility, Inc
 * All Rights Reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  - Neither the name of Motorola Mobility nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * */

#include "ndbus.h"

namespace ndbus {

static GHashTable *system_signal_watchers;
static GHashTable *session_signal_watchers;
static DBusConnection *system_bus;
static DBusConnection *session_bus;
Persistent<Object> global_target;

#define NDBUS_DEFINE_STRING_CONSTANT(target, constant)          \
                (target)->ForceSet(v8::String::NewFromUtf8(isolate, #constant, v8::String::kInternalizedString), \
                v8::String::NewFromUtf8(isolate, constant),                                                 \
                static_cast<v8::PropertyAttribute>(v8::ReadOnly|v8::DontDelete))

void NDbusRemoveMatch (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  gint message_type =
    NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_TYPE)->IntegerValue();
  if (message_type != DBUS_MESSAGE_TYPE_SIGNAL)
    NDBUS_EXCPN_TYPE;

  gint cnxn_type =
    NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_BUS)->IntegerValue();
  gboolean sess_bus = (cnxn_type == DBUS_BUS_SESSION);
  DBusConnection *bus_cnxn =
    sess_bus?session_bus:system_bus;
  GHashTable *signal_watchers =
    sess_bus?session_signal_watchers:system_signal_watchers;
  if (!bus_cnxn || !signal_watchers)
    NDBUS_EXCPN_NOMATCH;

  gchar *interface =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_INTERFACE));
  if (!interface)
    NDBUS_EXCPN_INTERFACE;

  gchar *signal_name =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_MEMBER));
  if (!signal_name) {
    g_free(interface);
    NDBUS_EXCPN_MEMBER;
  }

  gchar *object_path =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_PATH));
  gchar *destination =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_DEST));
  gchar *sender =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_SENDER));

  gchar *match_str = NDbusConstructMatchString(interface,
      signal_name, object_path, sender, destination);
  gchar *key = NDbusConstructKey(interface,
      signal_name, object_path, sender, destination);
  NDbusFree(destination, interface, object_path, signal_name);

  gboolean removed = FALSE;
  GSList *object_list =
    (GSList *) g_hash_table_lookup(signal_watchers, key);
  if (object_list) {
    GSList *tmp = object_list;
    while (tmp != NULL) {
      NDbusObjectInfo *info = (NDbusObjectInfo *) tmp->data;
      if (info &&
          (info->object == args.This())) {
        NDbusFreeObjectInfo((void*)info, NULL);
        object_list = g_slist_delete_link(object_list, tmp);
        if (g_slist_length(object_list) <= 0) {
          //there are no more listeners for this signal
          //so clean up the list
          g_slist_free(object_list);
          g_hash_table_remove(signal_watchers, key);
          object_list = NULL;
        } else {
          //the head of the list has changed and thus needs to be updated
          g_hash_table_insert(signal_watchers, g_strdup(key), object_list);
        }
        removed = TRUE;
        dbus_bus_remove_match(bus_cnxn, match_str, NULL);
        dbus_connection_flush(bus_cnxn);
        break;
      }
      tmp = g_slist_next(tmp);
    }
  }

  g_free(match_str);
  g_free(key);
  if (!removed)
    NDBUS_EXCPN_NOMATCH;
  args.GetReturnValue().Set(TRUE);
}

void NDbusAddMatch (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  gint message_type =
    NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_TYPE)->IntegerValue();
  if (message_type != DBUS_MESSAGE_TYPE_SIGNAL)
    NDBUS_EXCPN_TYPE;

  gchar *interface =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_INTERFACE));
  if (!interface)
    NDBUS_EXCPN_INTERFACE;

  gchar *signal_name =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_MEMBER));
  if (!signal_name) {
    g_free(interface);
    NDBUS_EXCPN_MEMBER;
  }

  gchar *object_path =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_PATH));
  gchar *destination =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_DEST));
  gchar *sender =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_SENDER));

  gint cnxn_type =
    NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_BUS)->IntegerValue();
  gboolean sess_bus = (cnxn_type == DBUS_BUS_SESSION);
  DBusConnection *bus_cnxn =
    sess_bus?session_bus:system_bus;
  GHashTable *signal_watchers =
    sess_bus?session_signal_watchers:system_signal_watchers;

  gchar *match_str = NDbusConstructMatchString(interface,
      signal_name, object_path, sender, destination);
  gchar *key = NDbusConstructKey(interface,
      signal_name, object_path, sender, destination);
  NDbusFree(destination, interface, object_path, signal_name);

  GSList *object_list =
    (GSList *) g_hash_table_lookup(signal_watchers, key);
  if (NDbusIsMatchAdded(object_list, args.This())) {
    g_free(match_str);
    g_free(key);
    args.GetReturnValue().Set(TRUE);
  }

  NDbusObjectInfo *listener = g_new0(NDbusObjectInfo, 1);
  listener->object.Reset(isolate, args.This());
  object_list = g_slist_prepend(object_list, (void *) listener);

  g_hash_table_insert(signal_watchers, g_strdup(key), object_list);

  dbus_bus_add_match(bus_cnxn, match_str, NULL);
  dbus_connection_flush(bus_cnxn);

  g_free(match_str);
  g_free(key);
  args.GetReturnValue().Set(TRUE);
}

void NDbusSendSignal (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  NDbusVariantPolicy variantPolicy = (NDbusVariantPolicy)NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_VARIANT_POLICY)->IntegerValue();

  gint message_type =
    NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_TYPE)->IntegerValue();
  if (message_type != DBUS_MESSAGE_TYPE_SIGNAL)
    NDBUS_EXCPN_TYPE;

  gchar *object_path =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_PATH));
  if (!object_path)
    NDBUS_EXCPN_PATH;

  gchar *interface =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_INTERFACE));
  if (!interface) {
    g_free (object_path);
    NDBUS_EXCPN_INTERFACE;
  }

  gchar *signal_name =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_MEMBER));
  if (!signal_name) {
    g_free(object_path);
    g_free(interface);
    NDBUS_EXCPN_MEMBER;
  }

  gint cnxn_type =
    NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_BUS)->IntegerValue();
  DBusConnection *bus_cnxn =
    (cnxn_type == DBUS_BUS_SESSION)?session_bus:system_bus;
  DBusMessage *msg =
    dbus_message_new_signal(object_path,
        interface, signal_name);

  NDbusFree(NULL, interface, object_path, signal_name);

  if (NULL == msg)
    NDBUS_EXCPN_OOM;

  Local<Object> append_error;
  if (!NDbusMessageAppendArgs (msg, args.This(), &append_error, variantPolicy)) {
    dbus_message_unref(msg);
    isolate->ThrowException(append_error);
    return;
  }

  if (!dbus_connection_send(bus_cnxn, msg, NULL)) {
    dbus_message_unref(msg);
    NDBUS_EXCPN_OOM;
  }
  dbus_connection_flush(bus_cnxn);
  dbus_message_unref(msg);

  args.GetReturnValue().Set(TRUE);
}

void NDbusInvokeMethod (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  NDbusVariantPolicy variantPolicy = (NDbusVariantPolicy)NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_VARIANT_POLICY)->IntegerValue();

  gint message_type =
    NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_TYPE)->IntegerValue();
  if (message_type != DBUS_MESSAGE_TYPE_METHOD_CALL
      && message_type != DBUS_MESSAGE_TYPE_METHOD_RETURN)
    NDBUS_EXCPN_TYPE;

  gchar *service =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_DEST));
  if(!service)
    NDBUS_EXCPN_DEST;

  gchar *object_path =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_PATH));
  if (!object_path) {
    g_free(service);
    NDBUS_EXCPN_PATH;
  }

  gchar *method_name =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_MEMBER));
  if (!method_name) {
    g_free(service);
    g_free(object_path);
    NDBUS_EXCPN_MEMBER;
  }

  gchar *interface =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_INTERFACE));

  gint cnxn_type =
    NDbusGetProperty(args.This(),
        NDBUS_PROPERTY_BUS)->IntegerValue();
  DBusConnection *bus_cnxn =
    (cnxn_type == DBUS_BUS_SESSION)?session_bus:system_bus;

  gint timeout = NDbusGetProperty(args.This(),
      NDBUS_PROPERTY_TIMEOUT)->IntegerValue();

  DBusMessage *msg =
    dbus_message_new_method_call(service, object_path,
        interface, method_name);

  NDbusFree(service, interface, object_path, method_name);

  if (NULL == msg)
    NDBUS_EXCPN_OOM;

  Local<Object> append_error;
  if (!NDbusMessageAppendArgs (msg, args.This(), &append_error, variantPolicy)) {
    dbus_message_unref(msg);
    isolate->ThrowException(append_error);
    return;
  }

  if (message_type == DBUS_MESSAGE_TYPE_METHOD_RETURN) {
    DBusPendingCall *pending;
    if (dbus_connection_send_with_reply(bus_cnxn, msg, &pending, timeout)) {
      if (pending) {
        NDbusObjectInfo *info = g_new0(NDbusObjectInfo, 1);

         info->object.Reset(isolate, args.This());
        dbus_pending_call_set_notify(pending, NDbusHandleMethodReply, (void *)info, NULL);
      } else {
        dbus_message_unref(msg);
        NDBUS_EXCPN_DISCONNECTED;
      }
    } else {
      dbus_message_unref(msg);
      NDBUS_EXCPN_OOM;
    }
    dbus_connection_flush(bus_cnxn);
  } else {
    DBusError error;
    dbus_error_init(&error);
    DBusMessage *reply =
      dbus_connection_send_with_reply_and_block(bus_cnxn, msg, timeout, &error);

    Handle<Object> local_global_target = Local<Object>::New(isolate, global_target);
    Local<Function> func = Local<Function>::Cast(local_global_target->Get(NDBUS_CB_METHODREPLY));
    const gint argc = 2;
    Local<Value> argv[2];

    if (dbus_error_is_set(&error)) {
       argv[0] = Undefined(isolate);
      NDBUS_SET_EXCPN(argv[1], error.name, error.message);
      dbus_error_free(&error);
    } else {
      Local<Value> msg_args = NDbusRetrieveMessageArgs(reply);
      argv[0] = msg_args;
      argv[1] = Undefined(isolate);
      dbus_message_unref(reply);
    }

    if (NDbusIsValidV8Value(func) &&
        func->IsFunction())
      func->Call(args.This(), argc, argv);
    else
      g_critical("\nSomeone has messed with the internal  \
          method response handler of dbus.js. 'methodResponse' wont be triggered.");
  }

  dbus_message_unref(msg);
  args.GetReturnValue().Set(TRUE);
}

void NDbusInit (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  gint cnxn_type = NDbusGetProperty(args.This(),
      NDBUS_PROPERTY_BUS)->IntegerValue();
  gchar *address =
    NDbusV8StringToCStr(NDbusGetProperty(args.This(),
          NDBUS_PROPERTY_ADDRESS));
  gboolean sess_bus = (cnxn_type == DBUS_BUS_SESSION);

  DBusConnection *bus_cnxn = sess_bus?session_bus:system_bus;
  if (bus_cnxn) {
    args.GetReturnValue().SetUndefined();
    return /* Undefined() */;
  }

  DBusError error;
  dbus_error_init(&error);
  if (address == NULL) {
    bus_cnxn = dbus_bus_get(DBusBusType(cnxn_type), &error);
  } else {
    bus_cnxn = dbus_connection_open(address, &error);
    if (!dbus_error_is_set(&error)) {
      dbus_bus_register(bus_cnxn, &error);
    }
    g_free(address);
  }

  if (dbus_error_is_set(&error)) {
    Local<Value> exptn;
    NDBUS_SET_EXCPN(exptn, error.name, error.message);
    dbus_error_free(&error);
    isolate->ThrowException(exptn);
    return;
  }

  GHashTable *signal_watchers =
    g_hash_table_new_full(g_str_hash,
        g_str_equal, (GDestroyNotify) g_free, NULL);
  if (sess_bus) {
    session_bus = bus_cnxn;
    session_signal_watchers = signal_watchers;
  } else {
    system_bus = bus_cnxn;
    system_signal_watchers = signal_watchers;
  }

  dbus_connection_set_exit_on_disconnect(bus_cnxn, FALSE);

  if (!NDbusConnectionSetupWithEvLoop(bus_cnxn))
    NDBUS_EXCPN_OOM;

  dbus_connection_add_filter(bus_cnxn, NDbusMessageFilter, (void *)signal_watchers, NULL);
  args.GetReturnValue().SetUndefined();
}

void NDbusClose (const FunctionCallbackInfo<Value>& args) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  gint cnxn_type = args[0]->IntegerValue();
  gboolean sess_bus = (cnxn_type == DBUS_BUS_SESSION);

  DBusConnection *bus_cnxn = sess_bus?session_bus:system_bus;
  if (bus_cnxn) {
    GHashTable *signal_watchers =
      sess_bus?session_signal_watchers:system_signal_watchers;
    dbus_connection_remove_filter(bus_cnxn, NDbusMessageFilter, (void *)signal_watchers);
    dbus_connection_unref(bus_cnxn);

    if (signal_watchers) {
      g_hash_table_foreach_remove(signal_watchers,
          (GHRFunc)NDbusRemoveAllSignalListeners, NULL);
      g_hash_table_unref(signal_watchers);
    }

    if (sess_bus) {
        session_signal_watchers = NULL;
        session_bus = NULL;
    } else {
        system_signal_watchers = NULL;
        system_bus = NULL;
    }
  }
  args.GetReturnValue().SetUndefined();
}

extern "C" {
void init (Handle<Object> target) {
  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);
  Handle<Object> constants = Object::New(isolate);
  target->Set(v8::String::NewFromUtf8(isolate, "constants", v8::String::kInternalizedString), constants);

  NODE_DEFINE_CONSTANT(constants, DBUS_BUS_SESSION);
  NODE_DEFINE_CONSTANT(constants, DBUS_BUS_SYSTEM);
  NODE_DEFINE_CONSTANT(constants, DBUS_MESSAGE_TYPE_INVALID);
  NODE_DEFINE_CONSTANT(constants, DBUS_MESSAGE_TYPE_METHOD_CALL);
  NODE_DEFINE_CONSTANT(constants, DBUS_MESSAGE_TYPE_METHOD_RETURN);
  NODE_DEFINE_CONSTANT(constants, DBUS_MESSAGE_TYPE_ERROR);
  NODE_DEFINE_CONSTANT(constants, DBUS_MESSAGE_TYPE_SIGNAL);

  NODE_DEFINE_CONSTANT(constants, NDBUS_VARIANT_POLICY_DEFAULT);
  NODE_DEFINE_CONSTANT(constants, NDBUS_VARIANT_POLICY_SIMPLE);

  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_SERVICE_DBUS);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_PATH_DBUS);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_PATH_LOCAL);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_INTERFACE_DBUS);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_INTERFACE_LOCAL);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_INTERFACE_INTROSPECTABLE);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_INTERFACE_PROPERTIES);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_INTERFACE_PEER);

  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_FAILED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_NO_MEMORY);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SERVICE_UNKNOWN);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_NAME_HAS_NO_OWNER);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_NO_REPLY);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_IO_ERROR);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_BAD_ADDRESS);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_NOT_SUPPORTED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_LIMITS_EXCEEDED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_ACCESS_DENIED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_AUTH_FAILED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_NO_SERVER);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_TIMEOUT);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_NO_NETWORK);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_ADDRESS_IN_USE);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_DISCONNECTED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_INVALID_ARGS);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_FILE_NOT_FOUND);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_FILE_EXISTS);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_UNKNOWN_METHOD);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_TIMED_OUT);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_MATCH_RULE_NOT_FOUND);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_MATCH_RULE_INVALID);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_EXEC_FAILED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_FORK_FAILED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_CHILD_EXITED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_CHILD_SIGNALED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_FAILED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_SETUP_FAILED);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_CONFIG_INVALID);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_SERVICE_INVALID);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_SERVICE_NOT_FOUND);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_PERMISSIONS_INVALID);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_FILE_INVALID);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SPAWN_NO_MEMORY);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_UNIX_PROCESS_ID_UNKNOWN);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_INVALID_SIGNATURE);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_INVALID_FILE_CONTENT);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_SELINUX_SECURITY_CONTEXT_UNKNOWN);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_ADT_AUDIT_DATA_UNKNOWN);
  NDBUS_DEFINE_STRING_CONSTANT(constants, DBUS_ERROR_OBJECT_PATH_IN_USE);

  NODE_SET_METHOD(target, "init", NDbusInit);
  NODE_SET_METHOD(target, "deinit", NDbusClose);
  NODE_SET_METHOD(target, "invokeMethod", NDbusInvokeMethod);
  NODE_SET_METHOD(target, "sendSignal", NDbusSendSignal);
  NODE_SET_METHOD(target, "addMatch", NDbusAddMatch);
  NODE_SET_METHOD(target, "removeMatch", NDbusRemoveMatch);

  global_target.Reset(isolate, target);
}
NODE_MODULE(ndbus, init);
}
}//namespace ndbus

