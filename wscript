#!/usr/bin/env python
# This file contains proprietary software owned by Motorola Mobility, Inc.
# No rights, expressed or implied, whatsoever to this software are provided by Motorola Mobility, Inc. hereunder.
# (c) Copyright 2011 Motorola Mobility, Inc. All Rights Reserved.

import Options
from os import unlink, getcwd, listdir
from os.path import splitext

srcdir = "."
blddir = "build"
VERSION = "0.0.1"

def set_options(opt):
  opt.tool_options("compiler_cxx")
  opt.add_option('--debug', action='store_true', default=False,
      help='Compile in debug mode with logs enabled')

def configure(conf):
  conf.check_tool("compiler_cxx")
  conf.check_tool("node_addon")
  conf.check_cfg(package='glib-2.0', uselib_store='GLIB', args='--cflags --libs', mandatory=True)
  conf.check_cfg(package='dbus-1', uselib_store='DBUS', args='--cflags --libs', mandatory=True)

def build(bld):
  ndbus = bld.new_task_gen("cxx", "shlib", "node_addon")
  ndbus.cxxflags = ["-D_FILE_OFFSET_BITS=64", "-D_LARGEFILE_SOURCE"]
  if Options.options.debug:
    ndbus.cxxflags += ["-g", "-O0", "-DENABLE_LOGS"]
  ndbus.target = "ndbus"
  ndbus.uselib = ['GLIB','DBUS']
  ndbus.source = """
                 src/ndbus.cc
                 src/ndbus-utils.cc
                 src/ndbus-connection-setup.cc
                 """
  ndbus.install_path  = getcwd()

def shutdown(bld):
  if Options.commands['clean'] or Options.commands['distclean']:
    for binding in listdir('.'):
      extn = splitext(binding)[1]
      if extn == '.node':
        unlink('./'+binding)
