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
  Isolate* isolate = Isolate::GetCurrent();
  EscapableHandleScope scope(isolate);

  Local<Value> ret;
  switch (dbus_message_iter_get_arg_type(reply_iter)) {
    case DBUS_TYPE_BOOLEAN:
      {
        gboolean value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = Boolean::New(isolate, value)->ToBoolean();
        break;
      }
    case DBUS_TYPE_BYTE:
      {
        guint8 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        //up-casting!
        ret = Uint32::NewFromUnsigned(isolate, value);
        break;
      }
    case DBUS_TYPE_UINT16:
      {
        guint16 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        //up-casting!
        ret = Uint32::NewFromUnsigned(isolate, value);
        break;
      }
    case DBUS_TYPE_UINT32:
      {
        guint32 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = Uint32::NewFromUnsigned(isolate, value);
        break;
      }
    case DBUS_TYPE_UINT64:
      {
        guint64 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        //up-casting!
        ret = Number::New(isolate, value);
        break;
      }
    case DBUS_TYPE_INT16:
      {
        gint16 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        //up-casting!
        ret = Int32::New(isolate, value);
        break;
      }
    case DBUS_TYPE_INT32:
      {
        gint32 value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = Int32::New(isolate, value);
        break;
      }
    case DBUS_TYPE_INT64:
    case DBUS_TYPE_DOUBLE:
      {
        gdouble value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = Number::New(isolate, value);
        break;
      }
    case DBUS_TYPE_SIGNATURE:
    case DBUS_TYPE_OBJECT_PATH:
    case DBUS_TYPE_STRING:
      {
        gchar *value;
        dbus_message_iter_get_basic(reply_iter, &value);
        ret = v8::String::NewFromUtf8(isolate, value);
        break;
      }
    case DBUS_TYPE_STRUCT:
    case DBUS_TYPE_ARRAY:
      {
        DBusMessageIter sub_iter;
        dbus_message_iter_recurse(reply_iter, &sub_iter);

        gboolean dictionary = (dbus_message_iter_get_arg_type(&sub_iter)
            == DBUS_TYPE_DICT_ENTRY);
        gint currentType;
        if (dictionary) {
          Local<Object> obj = Object::New(isolate);
          while((currentType = dbus_message_iter_get_arg_type (&sub_iter)) != DBUS_TYPE_INVALID) {
            DBusMessageIter dict_iter;
            dbus_message_iter_recurse(&sub_iter, &dict_iter);

            Local<Value> key =
              NDbusExtractMessageArgs(&dict_iter);

            dbus_message_iter_next(&dict_iter);

            Local<Value> value =
              NDbusExtractMessageArgs(&dict_iter);

            obj->ForceSet(key, value, None);
            dbus_message_iter_next(&sub_iter);
          }
          ret = obj;
        } else {
          Local<Array> arr = Array::New(isolate);
          gint i = 0;
          while((currentType = dbus_message_iter_get_arg_type (&sub_iter)) != DBUS_TYPE_INVALID) {
            arr->Set(i++, NDbusExtractMessageArgs(&sub_iter));
            dbus_message_iter_next(&sub_iter);
          }
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
        ret = Undefined(isolate);
//         ret = v8::String::NewFromUtf8(isolate, "");
        break;
      }
  }
  return scope.Escape(ret);
}

/**
 * Creates a signature string for parameters of type "variant". This string will
 * become a part of the parameter.
 *
 * @param value variant value for which to generate the signature
 * @param status status of the operation, eg. SUCCESS, OF_MEMORY, etc
 * @param variantPolicy indicates how variants should be resolved
 * @param buffer private - do NOT use! Memory where the signature will be stored.
 * @param index private - do NOT use! The current position within the buffer, when signature is created.
 * @return the generated signature or NULL on error. Use g_free to release the signature after use.
 */
static char*
NDbusCreateSignatureForVariant(Local<Value> value, gint &status, NDbusVariantPolicy variantPolicy, gchar* buffer, gint &index) {
  // allocate memory for the signature, if necessary
  gboolean cleanUp = FALSE;
  if (buffer == NULL) {
      // fill up with zeros, so that we'll be able to construct signature at the beginning
      // of the buffer and the signature will always be a NULL terminated string.
      buffer = g_try_new0(gchar, DBUS_MAXIMUM_SIGNATURE_LENGTH);
      if(buffer == NULL) {
        status = OUT_OF_MEMORY;
        return NULL;
      }
      index = 0; // just in case
      cleanUp = TRUE; // in case of error, we'll have to release memory
  }

  if (value->IsArray()) {
    // make sure the signature is not too long. We'll need at least 2 characters for array:
    // 1 char for array and 1 for the type of elements
    if (index + 2 > DBUS_MAXIMUM_SIGNATURE_LENGTH - 1) {
      status = OUT_OF_MEMORY;
      if(cleanUp) {
        g_free(buffer);
      }
      return NULL;
    }

    Local<Array> array = Local<Array>::Cast(value);
    buffer[index++] = DBUS_TYPE_ARRAY;
    if(variantPolicy == NDBUS_VARIANT_POLICY_DEFAULT) {
        buffer[index++] = DBUS_TYPE_VARIANT;
    } else if (array->Length()) {
      NDbusCreateSignatureForVariant(array->Get(0), status, variantPolicy, buffer, index);
    } else {
      // array is empty, so we cannot infer the type of elements - we'll just use a string.
      buffer[index++] = DBUS_TYPE_STRING;
    }
  } else if(value->IsObject()) {
    // make sure the signature is not too long. We'll need at least 5 characters for a map:
    // 1 char for array, 2 for brackets, 1 for key and 1 for value
    if (index + 5 > DBUS_MAXIMUM_SIGNATURE_LENGTH - 1) {
      status = OUT_OF_MEMORY;
      if(cleanUp) {
        g_free(buffer);
      }
      return NULL;
    }
    Local<Object> obj = Local<Object>::Cast(value);
    Local<Array> properties = obj->GetOwnPropertyNames();
    buffer[index++] = DBUS_TYPE_ARRAY;
    buffer[index++] = DBUS_DICT_ENTRY_BEGIN_CHAR;
    buffer[index++] = DBUS_TYPE_STRING; // keys of objects are strings in JS.
    if(variantPolicy == NDBUS_VARIANT_POLICY_DEFAULT) {
        buffer[index++] = DBUS_TYPE_VARIANT;
    } else if(properties->Length()) {
        NDbusCreateSignatureForVariant(obj->Get(properties->Get(0)), status, variantPolicy, buffer, index);
        // we do not know how many characters did the method above added, so we have
        // to check, if we can still add the closing bracket
        if (index + 1 > DBUS_MAXIMUM_SIGNATURE_LENGTH - 1) {
          status = OUT_OF_MEMORY;
          if(cleanUp) {
            g_free(buffer);
          }
          return NULL;
        }
    } else {
      buffer[index++] = DBUS_TYPE_STRING; // no properties in this object, so we'll assume values are strings
    }
    buffer[index++] = DBUS_DICT_ENTRY_END_CHAR;
  } else {
    // make sure the signature is not too long
    if (index + 1 > DBUS_MAXIMUM_SIGNATURE_LENGTH - 1) {
      status = OUT_OF_MEMORY;
      if(cleanUp) {
        g_free(buffer);
      }
      return NULL;
    }

    if (value->IsString()) {
      buffer[index++] = DBUS_TYPE_STRING;
    } else if (value->IsInt32()) {
      buffer[index++] = DBUS_TYPE_INT32;
    } else if (value->IsUint32()) {
      buffer[index++] = DBUS_TYPE_UINT32;
    } else if (value->IsNumber()) {
      buffer[index++] = DBUS_TYPE_DOUBLE;
    } else if (value->IsBoolean()) {
      buffer[index++] = DBUS_TYPE_BOOLEAN;
    } else {
        status = TYPE_NOT_SUPPORTED;
        if(cleanUp) {
          g_free(buffer);
        }
        return NULL;
    }
  }
  return buffer;
}

static gint
NDbusMessageAppendArgsReal (DBusMessageIter * iter,
    const gchar *signature, Local<Value> value, NDbusVariantPolicy variantPolicy) {
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
                obj_properties->Get(i), variantPolicy);

            if (status != SUCCESS) {
              dbus_free(sign_key);
              dbus_free(sign_val);
              return status;
            }

            status = NDbusMessageAppendArgsReal(&dictiter, sign_val,
                obj->Get(obj_properties->Get(i)), variantPolicy);

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
                array_signature, arr->Get(i++), variantPolicy);
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
        gint status = SUCCESS;
        gint index = 0;
        // this method should not require the last 2 params (NULL and index)
        gchar* vsignature = NDbusCreateSignatureForVariant(value, status, variantPolicy, NULL, index);
        if(status != SUCCESS) {
          return status;
        }

        if (!dbus_message_iter_open_container(iter, DBUS_TYPE_VARIANT, vsignature, &subiter)) {
          g_free(vsignature);
          return OUT_OF_MEMORY;
        }
        status = NDbusMessageAppendArgsReal(&subiter, vsignature, value, variantPolicy);
        if (status != SUCCESS) {
          g_free(vsignature);
          return status;
        }
        g_free(vsignature);
        dbus_message_iter_close_container(iter, &subiter);
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
        v8::Local<v8::Object> object = v8::Local<v8::Object>::New(Isolate::GetCurrent(), info->object);

        if (NDbusIsValidV8Value(object))
          Local<Array>::Cast(*listeners)->Set(index++, object);
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
  return obj->Get(v8::String::NewFromUtf8(Isolate::GetCurrent(), name));
}

gboolean
NDbusIsMatchAdded (GSList *list, Local<Object> obj) {
  if (list) {
    GSList *tmp = list;
    while (tmp != NULL) {
      NDbusObjectInfo *info = (NDbusObjectInfo *) tmp->data;
      v8::Local<v8::Object> object = v8::Local<v8::Object>::New(Isolate::GetCurrent(), info->object);
      if (info &&
          NDbusIsValidV8Value(object) &&
          (object == obj))
        return TRUE;
      tmp = g_slist_next(tmp);
    }
  }
  return FALSE;
}

Local<Value>
NDbusRetrieveMessageArgs(DBusMessage *msg) {
  DBusMessageIter msg_iter;
  Local<Array> args_array = Array::New(Isolate::GetCurrent());
  if (dbus_message_iter_init(msg, &msg_iter)) {
    gint i = 0;
    while (dbus_message_iter_get_arg_type(&msg_iter) !=
        DBUS_TYPE_INVALID) {
      args_array->Set(i++,
          NDbusExtractMessageArgs(&msg_iter));
      dbus_message_iter_next(&msg_iter);
    }
  }
  return args_array;
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
    Local<Object> obj, Local<Object> *error, NDbusVariantPolicy variantPolicy) {
  Isolate* isolate = Isolate::GetCurrent();

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
      if(sign == NULL) {
        g_free(signature);
        NDBUS_SET_EXCPN(*error, DBUS_ERROR_NO_MEMORY, NDBUS_ERROR_OOM);
        return FALSE;
      }
      gint status = NDbusMessageAppendArgsReal(&iter, sign, args->Get(i++), variantPolicy);
      dbus_free(sign);
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
    v8::Local<v8::Object> object = v8::Local<v8::Object>::New(Isolate::GetCurrent(), info->object);
    if (NDbusIsValidV8Value(object))
      info->object.Reset();
    g_free(info);
  }
}

DBusHandlerResult
NDbusMessageFilter (DBusConnection *cnxn,
    DBusMessage * message, void *user_data) {
  Isolate* isolate = Isolate::GetCurrent();

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
      HandleScope scope(isolate);
      gchar *key = g_strconcat(interface,
          "-", member, NULL);
      Local<Array> object_list = Array::New(isolate);

      NDbusRetrieveSignalListners(signal_watchers, key, &object_list);

      gchar *tmpKey = NULL;
      
      if (object_path) {
        tmpKey = key;
        key = g_strconcat(tmpKey, "-", object_path, NULL);
        g_free(tmpKey);
        NDbusRetrieveSignalListners(signal_watchers, key, &object_list);
      }
      if (sender) {
        tmpKey = key;
        key = g_strconcat(tmpKey, "-", sender, NULL);
        g_free(tmpKey);
        NDbusRetrieveSignalListners(signal_watchers, key, &object_list);
      }
      if (destination) {
        tmpKey = key;
        key = g_strconcat(tmpKey, "-", destination, NULL);
        g_free(tmpKey);
        NDbusRetrieveSignalListners(signal_watchers, key, &object_list);
      }
      g_free(key);

      if (NDbusIsValidV8Array(object_list)) {
        Local<Object> signal = Object::New(isolate);
        signal->Set(String::NewFromUtf8(isolate, NDBUS_PROPERTY_INTERFACE), String::NewFromUtf8(isolate, interface));
        signal->Set(String::NewFromUtf8(isolate, NDBUS_PROPERTY_MEMBER), String::NewFromUtf8(isolate, member));
        if (object_path) {
            signal->Set(String::NewFromUtf8(isolate, NDBUS_PROPERTY_PATH), String::NewFromUtf8(isolate, object_path));
        }
        else {
            signal->Set(String::NewFromUtf8(isolate, NDBUS_PROPERTY_PATH), Null(isolate));
        }
        if (sender) {
            signal->Set(String::NewFromUtf8(isolate, NDBUS_PROPERTY_SENDER), String::NewFromUtf8(isolate, sender));
        }
        else {
            signal->Set(String::NewFromUtf8(isolate, NDBUS_PROPERTY_SENDER), Null(isolate));
        }
        if (destination) {
            signal->ForceSet(String::NewFromUtf8(isolate, NDBUS_PROPERTY_DEST), String::NewFromUtf8(isolate, destination));
        }
        else {
            signal->ForceSet(String::NewFromUtf8(isolate, NDBUS_PROPERTY_DEST), Null(isolate));
        }

        Local<Value> args = NDbusRetrieveMessageArgs(message);
        const gint argc = 3;
        Local<Value> argv[argc];
        argv[0] = object_list;
        argv[1] = signal;
        argv[2] = args;

        Handle<Object> local_global_target = Local<Object>::New(isolate, global_target);
        Local<Function> func = Local<Function>::Cast(local_global_target->Get(NDBUS_CB_SIGNALRECEIPT));

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

  Isolate* isolate = Isolate::GetCurrent();
  HandleScope scope(isolate);

  DBusMessage *reply;
  NDbusObjectInfo *info = (NDbusObjectInfo *)user_data;

  v8::Local<v8::Object> object = v8::Local<v8::Object>::New(Isolate::GetCurrent(), info->object);
  if (NDbusIsValidV8Value(object)) {
    Handle<Object> local_global_target = Local<Object>::New(isolate, global_target);
    Local<Function> func = Local<Function>::Cast(local_global_target->Get(NDBUS_CB_METHODREPLY));
    const gint argc = 2;
    Local<Value> argv[2];

    reply = dbus_pending_call_steal_reply(pending);

    if(!reply) {
      argv[0] = Undefined(isolate);
      NDBUS_SET_EXCPN(argv[1], DBUS_ERROR_NO_REPLY, NDBUS_ERROR_NOREPLY);
    } else if (async_message_error(reply)){
      DBusError err;
      dbus_error_init(&err);
      dbus_set_error_from_message(&err, reply);
      argv[0] = Undefined(isolate);
      NDBUS_SET_EXCPN(argv[1], err.name, err.message);
      dbus_error_free(&err);
    } else if (dbus_message_get_type(reply) ==
        DBUS_MESSAGE_TYPE_METHOD_RETURN) {
      Local<Value> args = NDbusRetrieveMessageArgs(reply);
      argv[0] = args;
      argv[1] = Undefined(isolate);
    } else {
      argv[0] = Undefined(isolate);
      NDBUS_SET_EXCPN(argv[1], DBUS_ERROR_FAILED, NDBUS_ERROR_REPLY);
    }

    if (NDbusIsValidV8Value(func) &&
        func->IsFunction())
      func->Call(object, argc, argv);
    else
      g_critical("\nSomeone has messed with the internal  \
          method response handler of dbus.js. 'methodResponse' wont be triggered.");

    info->object.Reset();
    g_free(info);
    if (reply)
      dbus_message_unref(reply);
  }

  dbus_pending_call_unref(pending);
}

} //namespace ndbus
