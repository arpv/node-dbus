#!/usr/bin/env python

# Copyright (c) 2011, Motorola Mobility, Inc
#
# All Rights Reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the following conditions are met:
#
# * Redistributions of source code must retain the above copyright notice, this list of conditions and the following disclaimer.
# * Redistributions in binary form must reproduce the above copyright notice, this list of conditions and the following disclaimer in the documentation and/or other materials provided with the distribution.
# * Neither the name of Motorola Mobility nor the names of its contributors may be used to endorse or promote products derived from this software without specific prior written permission.

# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.


import Options
from os import unlink, symlink, listdir
from os.path import splitext, exists

srcdir = "."
blddir = "build"
VERSION = "0.1.2"

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

def shutdown(bld):
  if Options.commands['clean'] or Options.commands['distclean']:
    for binding in listdir('.'):
      extn = splitext(binding)[1]
      if extn == '.node':
        unlink('./'+binding)
  else:
    if exists('build/Release/ndbus.node') and not exists('ndbus.node'):
      symlink('build/Release/ndbus.node', 'ndbus.node')
