/*
 * This file contains proprietary software owned by Motorola Mobility, Inc.
 * No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
 * (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 * */

#include "ndbus.h"

namespace ndbus {

extern "C" {

enum {
  TYPE_MISMATCH,
  TYPE_NOT_SUPPORTED,
  OUT_OF_MEMORY,
  SUCCESS
};

static gboolean
async_message_error (DBusMessage *msg) {
  g_return_val_if_fail (msg != NULL, FALSE);
  return (dbus_message_get_type (msg) == DBUS_MESSAGE_TYPE_ERROR);
}

void
NDbusFree (gchar *service, gchar *interface,
    gchar *object_path, gchar *member) {
  if (service)
    g_free(service);
  if (interface)
    g_free(interface);
  if (object_path)
    g_free(object_path);
  if (member)
    g_free(member);
}

} //extern "C"

gboolean
NDbusIsValidV8Value (const Handle<Value> value) {
  return ((!value.IsEmpty()) &&
      (!value->IsUndefined()) &&
      (!value->IsNull()));
}

static gboolean
NDbusIsValidV8Array (const Local<Value> array) {
  return ((array->IsArray()) &&
      (Local<Array>::Cast(array)->Length() > 0));
}

static Local<Value>
NDbusExtractMessageArgs (DBusMessageIter *reply_iter) {
  HandleScope scope;

  Local<Value> ret;
  switch (dbus_message_iter_get_arg_type(reply_iter)) {
    case DBUS_TYPE_BOOLEAN:
      {
        gboolean value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = Boolean::New(value)->ToBoolean();
        break;
      }
    case DBUS_TYPE_BYTE:
      {
        guint8 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        //up-casting!
        ret = Uint32::NewFromUnsigned(value);
        break;
      }
    case DBUS_TYPE_UINT16:
      {
        guint16 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        //up-casting!
        ret = Uint32::NewFromUnsigned(value);
        break;
      }
    case DBUS_TYPE_UINT32:
      {
        guint32 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = Uint32::NewFromUnsigned(value);
        break;
      }
    case DBUS_TYPE_UINT64:
      {
        guint64 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        //up-casting!
        ret = Number::New(value);
        break;
      }
    case DBUS_TYPE_INT16:
      {
        gint16 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        //up-casting!
        ret = Int32::New(value);
        break;
      }
    case DBUS_TYPE_INT32:
      {
        gint32 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = Int32::New(value);
        break;
      }
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_DOUBLE:
      {
        gdouble value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = Number::New(value);
        break;
      }
    case DBUS_TYPE_SIGNATURE:
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_STRING:
      {
        gchar *value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = v8::String::New(value, strlen(value));
        break;
      }
    case DBUS_TYPE_STRUCT:
    case DBUS_TYPE_ARRAY:
      {
        DBusMessageIter sub_iter;
        dbus_message_iter_recurse(reply_iter, &sub_iter);

        gboolean dictionary = (dbus_message_iter_get_arg_type(&sub_iter)
            == DBUS_TYPE_DICT_ENTRY);
        if (dictionary) {
          Local<Object> obj = Object::New();
          do {
            DBusMessageIter dict_iter;
            dbus_message_iter_recurse(&sub_iter, &dict_iter);

            Local<Value> key =
              NDbusExtractMessageArgs(&dict_iter);

            dbus_message_iter_next(&dict_iter);

            Local<Value> value =
              NDbusExtractMessageArgs(&dict_iter);

            obj->Set(key, value, None);
          } while (dbus_message_iter_next(&sub_iter));
          ret = obj;
        } else {
          Local<Array> arr = Array::New();
          gint i = 0;
          do {
            arr->Set(i++,
                NDbusExtractMessageArgs(&sub_iter));
          } while (dbus_message_iter_next(&sub_iter));
          ret = arr;
        }
        break;
      }
    case DBUS_TYPE_VARIANT:
      {
        DBusMessageIter sub_iter;
        dbus_message_iter_recurse(reply_iter, &sub_iter);
        ret = NDbusExtractMessageArgs(&sub_iter);
        break;
      }
    default:
      {
        ret = Local<Value> (*Undefined());
        //ret = v8::String::New("");
        break;
      }
  }
  return scope.Close(ret);
}

static gint
NDbusMessageAppendArgsReal (DBusMessageIter * iter,
    const gchar *signature, Local<Value> value) {
  DBusSignatureIter signiter;
  dbus_signature_iter_init(&signiter, signature);

  switch (dbus_signature_iter_get_current_type(&signiter)) {
    case DBUS_TYPE_BOOLEAN:
      {
        if (!value->IsBoolean())
          return TYPE_MISMATCH;
        gboolean val = value->BooleanValue();
        dbus_message_iter_append_basic(iter, DBUS_TYPE_BOOLEAN, &val);
        break;
      }
    case DBUS_TYPE_INT32:
      {
        if (!value->IsInt32())
          return TYPE_MISMATCH;
        gint32 val = value->Int32Value();
        dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &val);
        break;
      }
    case DBUS_TYPE_UINT32:
      {
        if (!value->IsUint32())
          return TYPE_MISMATCH;
        guint32 val = value->Uint32Value();
        dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &val);
        break;
      }
    case DBUS_TYPE_INT64:
      {
        if (!value->IsNumber())
          return TYPE_MISMATCH;
        gint64 val = value->IntegerValue();
        dbus_message_iter_append_basic(iter, DBUS_TYPE_INT64, &val);
        break;
      }
    case DBUS_TYPE_DOUBLE:
      {
        if (!value->IsNumber())
          return TYPE_MISMATCH;
        gdouble val = value->NumberValue();
        dbus_message_iter_append_basic(iter, DBUS_TYPE_DOUBLE, &val);
        break;
      }
    case DBUS_TYPE_SIGNATURE:
      {
        gchar *str_value = NDbusV8StringToCStr(value);
        if (!str_value)
          return TYPE_MISMATCH;
        dbus_message_iter_append_basic(iter, DBUS_TYPE_SIGNATURE, &str_value);
        g_free(str_value);
        break;
      }
    case DBUS_TYPE_OBJECT_PATH:
      {
        gchar *str_value = NDbusV8StringToCStr(value);
        if (!str_value)
          return TYPE_MISMATCH;
        dbus_message_iter_append_basic(iter, DBUS_TYPE_OBJECT_PATH, &str_value);
        g_free(str_value);
        break;
      }
    case DBUS_TYPE_STRING:
      {
        gchar *str_value = NDbusV8StringToCStr(value);
        if (!str_value)
          return TYPE_MISMATCH;
        dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &str_value);
        g_free(str_value);
        break;
      }
    case DBUS_TYPE_ARRAY:
      {
        DBusMessageIter subiter;
        guint i = 0;
        gint status;
        if (dbus_signature_iter_get_element_type(&signiter) ==
            DBUS_TYPE_DICT_ENTRY) {
          if (!value->IsObject())
            return TYPE_MISMATCH;

          Local<Object> obj = Local<Object>::Cast(value);
          Local<Array> obj_properties = obj->GetOwnPropertyNames();
          //dont add 'empty' objects; tell me a usecase for it
          if (!obj_properties->Length())
            break;

          DBusSignatureIter dictsigniter, dictsignsubiter;
          dbus_signature_iter_recurse(&signiter, &dictsigniter);
          gchar *dict_signature =
            dbus_signature_iter_get_signature(&dictsigniter);

          if (!dbus_message_iter_open_container
              (iter, DBUS_TYPE_ARRAY, dict_signature, &subiter)) {
            dbus_free(dict_signature);
            return OUT_OF_MEMORY;
          }

          dbus_free(dict_signature);
          dbus_signature_iter_recurse(&dictsigniter, &dictsignsubiter);

          gchar *sign_key =
            dbus_signature_iter_get_signature(&dictsignsubiter);
          dbus_signature_iter_next(&dictsignsubiter);
          gchar *sign_val =
            dbus_signature_iter_get_signature(&dictsignsubiter);

          guint len = obj_properties->Length();
          for (; i<len; i++) {
            DBusMessageIter dictiter;
            if (!dbus_message_iter_open_container
                (&subiter, DBUS_TYPE_DICT_ENTRY,
                 NULL, &dictiter)) {
              dbus_free(sign_key);
              dbus_free(sign_val);
              return OUT_OF_MEMORY;
            }

            status = NDbusMessageAppendArgsReal(&dictiter, sign_key,
                obj_properties->Get(i));

            if (status != SUCCESS) {
              dbus_free(sign_key);
              dbus_free(sign_val);
              return status;
            }

            status = NDbusMessageAppendArgsReal(&dictiter, sign_val,
                obj->Get(obj_properties->Get(i)));

            if (status != SUCCESS) {
              dbus_free(sign_key);
              dbus_free(sign_val);
              return status;
            }

            dbus_message_iter_close_container(&subiter, &dictiter);
          }

          dbus_free(sign_key);
          dbus_free(sign_val);
          dbus_message_iter_close_container(iter, &subiter);
          break;
        } else {
          if (!value->IsArray())
            return TYPE_MISMATCH;

          Local<Array> arr = Local<Array>::Cast(value);
          //dont add 'empty' arrays; tell me a usecase for it
          if (!arr->Length())
            break;

          DBusSignatureIter arraysigniter;
          dbus_signature_iter_recurse(&signiter, &arraysigniter);
          gchar *array_signature =
            dbus_signature_iter_get_signature(&arraysigniter);

          if (!dbus_message_iter_open_container
              (iter, DBUS_TYPE_ARRAY, array_signature, &subiter)) {
            dbus_free(array_signature);
            return OUT_OF_MEMORY;
          }

          while (i < arr->Length()) {
            status = NDbusMessageAppendArgsReal(&subiter,
                array_signature,(arr->Get(i++)));
            if (status != SUCCESS) {
              dbus_free(array_signature);
              return status;
            }
          }

          dbus_message_iter_close_container(iter, &subiter);
          dbus_free(array_signature);
          break;
        }
      }
    case DBUS_TYPE_VARIANT:
      {
        DBusMessageIter subiter;
        gchar vsignature[2];
        vsignature[0] = 0;

        if (value->IsString()) {
          vsignature[0] = DBUS_TYPE_STRING;
          vsignature[1] = '\0';
        } else if (value->IsInt32()) {
          vsignature[0] = DBUS_TYPE_INT32;
          vsignature[1] = '\0';
        } else if (value->IsBoolean()) {
          vsignature[0] = DBUS_TYPE_BOOLEAN;
          vsignature[1] = '\0';
        }

        if (vsignature[0]) {
          if (!dbus_message_iter_open_container
              (iter, DBUS_TYPE_VARIANT, vsignature, &subiter)) {
            g_free(vsignature);
            return OUT_OF_MEMORY;
          }

          gint status = NDbusMessageAppendArgsReal(&subiter,
              vsignature, value);
          if (status != SUCCESS) {
            g_free(vsignature);
            return status;
          }

          dbus_message_iter_close_container(iter, &subiter);
        } else
          return TYPE_MISMATCH;
        break;
      }
    default:
      return TYPE_NOT_SUPPORTED;
  }
  return SUCCESS;
}

static void
NDbusRetrieveSignalListners (GHashTable *signal_watchers,
    gchar *key, Local<Array> *listeners) {
  if (signal_watchers) {
    GSList *object_list = (GSList *)
      g_hash_table_lookup(signal_watchers, key);

    if (object_list) {
      GSList *tmp = object_list;
      gint index = Local<Array>::Cast(*listeners)->Length();

      while(tmp != NULL) {
        NDbusObjectInfo *info = (NDbusObjectInfo *)tmp->data;
        if (NDbusIsValidV8Value(info->object))
          Local<Array>::Cast(*listeners)->Set(index++, info->object);
        tmp = g_slist_next(tmp);
      }
    }
  }
}

//EXPOSED
gboolean
NDbusRemoveAllSignalListeners (gpointer key,
    gpointer value, gpointer user_data) {
  GSList *list = (GSList *) value;
  if (list) {
    g_slist_foreach(list, (GFunc)NDbusFreeObjectInfo, NULL);
    g_slist_free(list);
    list = NULL;
  }
  return TRUE;
}

gchar*
NDbusV8StringToCStr (const Local<Value> str) {
  if (str->IsString()) {
    v8::String::Utf8Value vStr(str);
    gint len = strlen(*vStr)+1;
    gchar *cStr = (gchar *) g_malloc(len);
    strncpy(cStr, *vStr, len);
    return cStr;
  }
  return NULL;
}

Local<Value>
NDbusGetProperty (const Local<Object> obj,
    const gchar *name) {
  return obj->Get(v8::String::New(name));
}

gboolean
NDbusIsMatchAdded (GSList *list, Local<Object> obj) {
  if (list) {
    GSList *tmp = list;
    while (tmp != NULL) {
      NDbusObjectInfo *info = (NDbusObjectInfo *) tmp->data;
      if (info &&
          NDbusIsValidV8Value(info->object) &&
          (info->object == obj))
        return TRUE;
      tmp = g_slist_next(tmp);
    }
  }
  return FALSE;
}

Local<Value>
NDbusRetrieveMessageArgs(DBusMessage *msg) {
  DBusMessageIter msg_iter;
  if (dbus_message_iter_init(msg, &msg_iter)) {
    gint i = 0;
    Local<Array> args_array = Array::New();
    while (dbus_message_iter_get_arg_type(&msg_iter) !=
        DBUS_TYPE_INVALID) {
      args_array->Set(i++,
          NDbusExtractMessageArgs(&msg_iter));
      dbus_message_iter_next(&msg_iter);
    }
    return args_array;
  }
  return Local<Value>(*Undefined());
}

gchar*
NDbusConstructMatchString (gchar *interface,
    gchar *member, gchar *object_path,
    gchar *sender, gchar *destination) {
  gchar *match_str =
    g_strconcat("type='signal',interface='", interface,
        "',member='", member,"'", NULL);
  match_str =
    (object_path)?g_strconcat(match_str,
        ",path='", object_path, "'", NULL):match_str;
  match_str =
    (sender)?g_strconcat(match_str,
        ",sender='", sender, "'", NULL):match_str;
  match_str =
    (destination)?g_strconcat(match_str,
        ",destination='", destination, "'", NULL):match_str;
  return match_str;
}

gchar*
NDbusConstructKey (gchar *interface,
    gchar *member, gchar *object_path,
    gchar *sender, gchar *destination) {
  gchar *key = g_strconcat(interface, "-",
      member, NULL);
  key = (object_path)?g_strconcat(key, "-",
      object_path, NULL):key;
  key = (sender)?g_strconcat(key, "-",
      sender, NULL):key;
  key = (destination)?g_strconcat(key, "-",
      destination, NULL):key;
  return key;
}

gboolean
NDbusMessageAppendArgs (DBusMessage *msg,
    Local<Object> obj, Local<Object> *error) {

  gchar *signature = NDbusV8StringToCStr(
      NDbusGetProperty(obj, NDBUS_PROPERTY_SIGN));
  Local<Array> args = Local<Array>::Cast
    (NDbusGetProperty(obj, NDBUS_PROPERTY_ARGS));

  if (signature &&
      NDbusIsValidV8Array(args)) {

    if (!dbus_signature_validate(signature, NULL)) {
      g_free(signature);
      NDBUS_SET_EXCPN(*error, DBUS_ERROR_INVALID_SIGNATURE, NDBUS_ERROR_SIGN);
      return FALSE;
    }

    DBusMessageIter iter;
    dbus_message_iter_init_append(msg, &iter);
    DBusSignatureIter sigiter;
    guint i = 0;

    dbus_signature_iter_init(&sigiter, signature);

    while (i < args->Length()) {
      gchar *sign = dbus_signature_iter_get_signature(&sigiter);
      gint status = NDbusMessageAppendArgsReal(&iter, sign,
          (args->Get(i++)));

      if (status < SUCCESS) {
        g_free(signature);
        if (status == TYPE_MISMATCH)
          NDBUS_SET_EXCPN(*error, DBUS_ERROR_FAILED, NDBUS_ERROR_MISMATCH);
        if (status == TYPE_NOT_SUPPORTED)
          NDBUS_SET_EXCPN(*error, DBUS_ERROR_FAILED, NDBUS_ERROR_UNSUPPORTED);
        if (status == OUT_OF_MEMORY)
          NDBUS_SET_EXCPN(*error, DBUS_ERROR_NO_MEMORY, NDBUS_ERROR_OOM);
        return FALSE;
      }

      dbus_signature_iter_next(&sigiter);
    }
    g_free(signature);
  }
  return TRUE;
}

void
NDbusFreeObjectInfo (gpointer data, gpointer user_data) {
  NDbusObjectInfo *info =
    (NDbusObjectInfo *)data;
  if (info) {
    if (NDbusIsValidV8Value(info->object))
      info->object.Dispose();
    g_free(info);
  }
}

DBusHandlerResult
NDbusMessageFilter (DBusConnection *cnxn,
    DBusMessage * message, void *user_data) {
  if (dbus_message_get_type (message)
      != DBUS_MESSAGE_TYPE_SIGNAL)
    return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;

#if 0
  g_print("\nSignal Received: %s, Interface: %s, Sender: %s, Path: %s\n",
      dbus_message_get_member(message),
      dbus_message_get_interface(message),
      dbus_message_get_sender(message),
      dbus_message_get_path(message));
#endif

  GHashTable *signal_watchers = (GHashTable *)user_data;

  if (dbus_message_is_signal(message,
        DBUS_INTERFACE_LOCAL, "Disconnected")) {
    if (signal_watchers)
      g_hash_table_foreach_remove(signal_watchers,
          (GHRFunc)NDbusRemoveAllSignalListeners, NULL);
  } else {
    const gchar *member = dbus_message_get_member(message);
    const gchar *interface = dbus_message_get_interface(message);
    const gchar *sender = dbus_message_get_sender(message);
    const gchar *object_path = dbus_message_get_path(message);
    const gchar *destination  = dbus_message_get_destination(message);

    if (interface && member) {
      HandleScope scope;
      gchar *key = g_strconcat(interface,
          "-", member, NULL);
      Local<Array> object_list = Array::New();

      NDbusRetrieveSignalListners(signal_watchers, key, &object_list);

      if (object_path) {
        key = g_strconcat(key, "-", object_path, NULL);
        NDbusRetrieveSignalListners(signal_watchers, key, &object_list);
      }
      if (sender) {
        key = g_strconcat(key, "-", sender, NULL);
        NDbusRetrieveSignalListners(signal_watchers, key, &object_list);
      }
      if (destination) {
        key = g_strconcat(key, "-", destination, NULL);
        NDbusRetrieveSignalListners(signal_watchers, key, &object_list);
      }
      g_free(key);

      if (NDbusIsValidV8Array(object_list)) {
        Local<Value> args = NDbusRetrieveMessageArgs(message);
        const gint argc = 2;
        Local<Value> argv[2];
        argv[0] = object_list;
        argv[1] = args;
        Local<Function> func = Local<Function>::
          Cast(global_target->Get(NDBUS_CB_SIGNALRECEIPT));

        if (NDbusIsValidV8Value(func) &&
            func->IsFunction())
          func->Call(func, argc, argv);
        else
          g_critical("\nSomeone has messed with the internal  \
              signal receipt handler of dbus.js. 'signalReceipt' wont be triggered.");
      }
    }
  }
  return DBUS_HANDLER_RESULT_NOT_YET_HANDLED;
}

void
NDbusHandleMethodReply (DBusPendingCall *pending,
    void *user_data) {
  g_return_if_fail(pending != NULL);

  if(user_data == NULL) {
    dbus_pending_call_unref(pending);
    return;
  }

  HandleScope scope;

  DBusMessage *reply;
  NDbusObjectInfo *info = (NDbusObjectInfo *)user_data;

  if (NDbusIsValidV8Value(info->object)) {
    Handle<Object> object = Handle<Object>::Cast(info->object);
    Local<Function> func =
      Local<Function>::Cast(global_target->
          Get(NDBUS_CB_METHODREPLY));
    const gint argc = 2;
    Local<Value> argv[2];

    reply = dbus_pending_call_steal_reply(pending);

    if(!reply) {
      argv[0] = Local<Value> (*Undefined());
      NDBUS_SET_EXCPN(argv[1], DBUS_ERROR_NO_REPLY, NDBUS_ERROR_NOREPLY);
    } else if (async_message_error(reply)){
      DBusError err;
      dbus_error_init(&err);
      dbus_set_error_from_message(&err, reply);
      argv[0] = Local<Value> (*Undefined());
      NDBUS_SET_EXCPN(argv[1], err.name, err.message);
      dbus_error_free(&err);
    } else if (dbus_message_get_type(reply) ==
        DBUS_MESSAGE_TYPE_METHOD_RETURN) {
      Local<Value> args = NDbusRetrieveMessageArgs(reply);
      argv[0] = args;
      argv[1] = Local<Value> (*Undefined());
    } else {
      argv[0] = Local<Value> (*Undefined());
      NDBUS_SET_EXCPN(argv[1], DBUS_ERROR_FAILED, NDBUS_ERROR_REPLY);
    }

    if (NDbusIsValidV8Value(func) &&
        func->IsFunction())
      func->Call(object, argc, argv);
    else
      g_critical("\nSomeone has messed with the internal  \
          method response handler of dbus.js. 'methodResponse' wont be triggered.");

    info->object.Dispose();
    g_free(info);
    if (reply)
      dbus_message_unref(reply);
  }

  dbus_pending_call_unref(pending);
}

} //namespace ndbus
