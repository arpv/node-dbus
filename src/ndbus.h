/*
 * Copyright (c) 2011, Motorola Mobility, Inc
 * All Rights Reserved.
 * Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
 *  - Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
 *  - Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
 *  - Neither the name of Motorola Mobility nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * */

#ifndef __NDBUS_H__
#define __NDBUS_H__

#include <node.h>
using namespace v8;
using namespace node;

namespace ndbus {

#define NDBUS_PROPERTY_BUS            "bus"
#define NDBUS_PROPERTY_TYPE           "type"
#define NDBUS_PROPERTY_ADDRESS        "address"
#define NDBUS_PROPERTY_DEST           "destination"
#define NDBUS_PROPERTY_SENDER         "sender"
#define NDBUS_PROPERTY_PATH           "path"
#define NDBUS_PROPERTY_INTERFACE      "iface"
#define NDBUS_PROPERTY_MEMBER         "member"
#define NDBUS_PROPERTY_ARGS           "_inputArgs"
#define NDBUS_PROPERTY_SIGN           "_signature"
#define NDBUS_PROPERTY_TIMEOUT        "timeout"
#define NDBUS_PROPERTY_VARIANT_POLICY "variantPolicy"

#define NDBUS_SET_EXCPN(excpn, name, message) \
  {                                           \
    excpn = Object::New(isolate);             \
    Local<Object>::Cast(excpn)->              \
      Set(v8::String::NewFromUtf8(isolate, "name"),   \
          v8::String::NewFromUtf8(isolate, name));    \
    Local<Object>::Cast(excpn)->              \
      Set(v8::String::NewFromUtf8(isolate, "message"),\
          v8::String::NewFromUtf8(isolate, message));\
  }

#define NDBUS_THROW_EXCPN(name, message)  \
  {                                       \
    Isolate* isolate = Isolate::GetCurrent(); \
    Local<Object> excpn;                  \
    NDBUS_SET_EXCPN(excpn, name, message) \
    isolate->ThrowException(excpn);       \
    return;                               \
  }

#define NDBUS_ERROR_REPLY             "Invalid reply from daemon"
#define NDBUS_ERROR_NOREPLY           "No reply from daemon"
#define NDBUS_ERROR_MISMATCH          "signature-arguments type mismatch"
#define NDBUS_ERROR_UNSUPPORTED       "Argument type not supported"
#define NDBUS_ERROR_OOM               "Out of memory!"
#define NDBUS_ERROR_SIGN              "Invalid argument signature"

#define NDBUS_EXCPN_TYPE              NDBUS_THROW_EXCPN(DBUS_ERROR_FAILED, "Invalid message type")
#define NDBUS_EXCPN_DEST              NDBUS_THROW_EXCPN(DBUS_ERROR_FAILED, "Invalid destination")
#define NDBUS_EXCPN_PATH              NDBUS_THROW_EXCPN(DBUS_ERROR_FAILED, "Invalid object path")
#define NDBUS_EXCPN_INTERFACE         NDBUS_THROW_EXCPN(DBUS_ERROR_FAILED, "Invalid interface name")
#define NDBUS_EXCPN_MEMBER            NDBUS_THROW_EXCPN(DBUS_ERROR_FAILED, "Invalid member name")
#define NDBUS_EXCPN_OOM               NDBUS_THROW_EXCPN(DBUS_ERROR_NO_MEMORY, NDBUS_ERROR_OOM)
#define NDBUS_EXCPN_DISCONNECTED      NDBUS_THROW_EXCPN(DBUS_ERROR_DISCONNECTED, "Connection got disconnected")
#define NDBUS_EXCPN_NOMATCH           NDBUS_THROW_EXCPN(DBUS_ERROR_MATCH_RULE_NOT_FOUND, "The match was already removed or never added.")

#define NDBUS_CB_METHODREPLY          v8::String::NewFromUtf8(isolate, "onMethodResponse")
#define NDBUS_CB_SIGNALRECEIPT        v8::String::NewFromUtf8(isolate, "onSignalReceipt")

/**
 * How should variant in signatures of signals to send be handles.
 */
typedef enum {
    /**
     * If object or array is passed where variant is expected, then object and array
     * values are variants themselves.
     */
    NDBUS_VARIANT_POLICY_DEFAULT,
    /**
     * If object or array is passed where variant is expected, then the first property
     * of an object or the first element of an array will be inspected to determine
     * the type of the value. This is in contrast with the default policy, which
     * would use variant for values.
     *
     * This type of handling variants is required by some applications, for example
     * SetProperty method in Connman.
     *
     * Warning! The simple policy is less flexible than the default policy in that
     * all array or object values must be the same type.
     */
    NDBUS_VARIANT_POLICY_SIMPLE
} NDbusVariantPolicy;

extern "C" {

#include <stdlib.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <string.h>

#ifdef ENABLE_LOGS
#define LOG(s)                        g_print(("D-%s:%u:"s"\n"), __FILE__, __LINE__)
#define LOGV(s,...)                   g_print(("D-%s:%u:"s"\n"),  __FILE__, __LINE__, __VA_ARGS__)
#else
#define NOOP                          /*do nothing*/
#define LOG(s)                        NOOP
#define LOGV(s,...)                   NOOP
#endif

gboolean NDbusConnectionSetupWithEvLoop   (DBusConnection *bus_cnxn);
DBusHandlerResult NDbusMessageFilter      (DBusConnection *cnxn,
                                           DBusMessage * message,
                                           void *user_data);
void NDbusFree                            (gchar *service,
                                           gchar *interface,
                                           gchar *object_path,
                                           gchar *member);
} //extern "C"

extern Persistent<Object> global_target;

typedef struct {
  Persistent<Object> object;
} NDbusObjectInfo;

gboolean NDbusIsValidV8Value              (const Handle<Value> value);
gchar* NDbusV8StringToCStr                (const Local<Value> str);
Local<Value> NDbusGetProperty             (const Local<Object> obj,
                                           const gchar *name);
gboolean NDbusIsMatchAdded                (GSList *list,
                                           Local<Object> obj);
gboolean NDbusMessageAppendArgs           (DBusMessage *msg,
                                           Local<Object> obj,
                                           Local<Object> *error,
                                           NDbusVariantPolicy variantPolicy);
Local<Value> NDbusRetrieveMessageArgs     (DBusMessage *msg);
void NDbusHandleMethodReply               (DBusPendingCall *pending,
                                           void *user_data);
gchar* NDbusConstructKey                  (gchar *interface,
                                           gchar *member,
                                           gchar *object_path,
                                           gchar *sender,
                                           gchar *destination);
gchar* NDbusConstructMatchString          (gchar *interface,
                                           gchar *member,
                                           gchar *object_path,
                                           gchar *sender,
                                           gchar *destination);
gboolean NDbusRemoveAllSignalListeners    (gpointer key,
                                           gpointer value,
                                           gpointer user_data);
void NDbusFreeObjectInfo                  (gpointer data,
                                           gpointer user_data);
} //namespace ndbus

#endif  /* __NDBUS_H__ */
