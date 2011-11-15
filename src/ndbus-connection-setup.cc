/*
 * This file contains proprietary software owned by Motorola Mobility, Inc.
 * No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
 * (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.
 * */

#include "ndbus.h"

namespace ndbus {
extern "C" {

static void
handle_iow_freed (void *data) {
  ev_io *iow = (ev_io*)data;
  if(iow == NULL)
    return;
  iow->data = NULL;
}

static void
iow_cb (EV_P_ ev_io *w, gint events) {
  DBusWatch *watch = (DBusWatch*)w->data;
  guint dbus_condition = 0;

  if (events & EV_READ)
    dbus_condition |= DBUS_WATCH_READABLE;
  if (events & EV_WRITE)
    dbus_condition |= DBUS_WATCH_WRITABLE;
  if (events & EV_ERROR) {
    dbus_condition |= DBUS_WATCH_ERROR;
    dbus_condition |= DBUS_WATCH_HANGUP;
  }

  dbus_watch_handle (watch, dbus_condition);
}

static dbus_bool_t
add_watch (DBusWatch *watch, void *data) {
  if (!dbus_watch_get_enabled(watch)
      || dbus_watch_get_data(watch) != NULL)
    return true;

  gint fd = dbus_watch_get_unix_fd(watch);
  guint flags = dbus_watch_get_flags(watch);
  gint events = 0;

  if (flags & DBUS_WATCH_READABLE)
    events |= EV_READ;
  if (flags & DBUS_WATCH_WRITABLE)
    events |= EV_WRITE;

  ev_io *iow = g_new0(ev_io, 1);
  iow->data = (void *)watch;
  ev_io_init(iow, iow_cb, fd, events);
  ev_io_start(EV_DEFAULT_UC_ iow);
  ev_unref(EV_DEFAULT_UC);

  dbus_watch_set_data(watch, (void *)iow, handle_iow_freed);
  return true;
}

static void
remove_watch (DBusWatch *watch, void *data) {
  ev_io* iow = (ev_io*)dbus_watch_get_data(watch);

  if (iow == NULL)
    return;

  ev_io_stop(EV_DEFAULT_UC_ iow);
  g_free(iow);
  iow = NULL;
  dbus_watch_set_data(watch, NULL, NULL);
}

static void
watch_toggled (DBusWatch *watch, void *data) {
  if (dbus_watch_get_enabled (watch))
    add_watch (watch, data);
  else
    remove_watch (watch, data);
}

static void
timeout_cb (EV_P_ ev_timer *w, gint events) {
  DBusTimeout *timeout = (DBusTimeout*)w->data;
  dbus_timeout_handle(timeout);
}

static void
handle_timeout_freed (void *data) {
  ev_timer *timer = (ev_timer*)data;
  if(timer == NULL)
    return;
  timer->data =  NULL;
}

static dbus_bool_t
add_timeout (DBusTimeout *timeout, void *data) {
  if (!dbus_timeout_get_enabled (timeout)
      || dbus_timeout_get_data(timeout) != NULL)
    return true;

  ev_timer* timer = g_new0(ev_timer, 1);
  gfloat timeinSeconds = dbus_timeout_get_interval (timeout)/1000.00;
  timer->data = timeout;
  ev_timer_init (timer, timeout_cb, timeinSeconds, 0.);
  ev_timer_start (EV_DEFAULT_UC_ timer);

  dbus_timeout_set_data (timeout, (void *)timer, handle_timeout_freed);
  return true;
}

static void
remove_timeout (DBusTimeout *timeout, void *data) {
  ev_timer *timer =
    (ev_timer*)dbus_timeout_get_data (timeout);

  if (timer == NULL)
    return;

  ev_timer_stop(EV_DEFAULT_UC_ timer);
  g_free(timer);
  timer = NULL;
  dbus_timeout_set_data (timeout, NULL, NULL);
}

static void
timeout_toggled (DBusTimeout *timeout, void *data) {
  if (dbus_timeout_get_enabled (timeout))
    add_timeout (timeout, data);
  else
    remove_timeout (timeout, data);
}

static void
wakeup_ev (void *data) {
  ev_async* asyncw = (ev_async *)data;
  ev_async_send(EV_DEFAULT_UC_ asyncw);
}

static void
asyncw_cb (EV_P_ ev_async *w, gint events) {
  DBusConnection *bus_cnxn = (DBusConnection *)w->data;
  dbus_connection_read_write(bus_cnxn, 0);
  while (dbus_connection_dispatch(bus_cnxn) ==
      DBUS_DISPATCH_DATA_REMAINS);
}

gboolean
NDbusConnectionSetupWithEvLoop (DBusConnection *bus_cnxn) {
  if (!dbus_connection_set_watch_functions (bus_cnxn,
      add_watch,
      remove_watch,
      watch_toggled,
      NULL, NULL))
    return false;

  if (!dbus_connection_set_timeout_functions(bus_cnxn,
      add_timeout,
      remove_timeout,
      timeout_toggled,
      NULL, NULL))
    return false;

  ev_async *asyncw = g_new0(ev_async, 1);
  asyncw->data = (void *)bus_cnxn;
  ev_async_init(asyncw, asyncw_cb);
  ev_async_start(EV_DEFAULT_UC_ asyncw);
  ev_unref(EV_DEFAULT_UC);
  dbus_connection_set_wakeup_main_function(bus_cnxn,
      wakeup_ev,
      (void *)asyncw, g_free);

  return true;
}

} //extern "C"
} //namespace ndbus
