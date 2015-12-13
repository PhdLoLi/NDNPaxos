#! /usr/bin/env python
# coding: utf-8

import sys
import os
from waflib import Options
from waflib import Logs

APPNAME = "ndnpaxos"
VERSION = "0.1"

top = "."
out = "bin"

pargs = ['--cflags', '--libs']
CFLAGS = []

COMPILER_LANG = "compiler_cxx"


def options(opt):
    opt.load(COMPILER_LANG)
    opt.load('boost protoc', tooldir=['waf-tools'])
    
    opt.add_option('-d', '--debug', dest='debug', default=False, action='store_true')
    opt.add_option('-l', '--log', dest="log", default='', help='log level', action='store')
    opt.add_option('-m', '--mode', dest="mode", default='', help='mode type', action='store')

def configure(conf):
    conf.env['CXX'] = "clang++"
    conf.load(COMPILER_LANG)
    conf.load('boost protoc')

    _enable_pic(conf)
    _enable_debug(conf)     #debug
    _enable_log(conf)       #log level
    _enable_mode(conf)      #mode type 
    _enable_static(conf)    #static
    _enable_cxx11(conf)


    conf.check_cfg(package='yaml-cpp', uselib_store='YAML-CPP', args=pargs)

    conf.check_cfg(package='libndn-cxx', args=['--cflags', '--libs'],
                   uselib_store='NDN_CXX', mandatory=True)

    USED_BOOST_LIBS = ['system', 'filesystem', 'date_time', 'iostreams', 'thread',
                      'regex', 'program_options', 'chrono', 'random']

    conf.check_boost(lib=USED_BOOST_LIBS, mandatory=True)

    conf.env.PREFIX = "/usr"
    conf.env.LIBDIR = "/usr/lib"
    conf.env.INCLUDEDIR = "/usr/include"

def build(bld):

#    os.system('protoc -I=libndnpaxos --python_out=script libndnpaxos/ndnpaxos.proto')

    bld.stlib(source=bld.path.ant_glob(['libndnpaxos/view.cpp', 'libndnpaxos/ndnpaxos.proto',
                                        'libndnpaxos/proposer.cpp', 'libndnpaxos/acceptor.cpp', 
                                        'libndnpaxos/captain.cpp', 'libndnpaxos/commo.cpp', 
                                        'libndnpaxos/producer.cpp', 'libndnpaxos/consumer.cpp',
                                        'libndnpaxos/*.proto',
                                        'libzfec/fec.cc']), 
              target="ndnpaxos",
              includes="libndnpaxos libzfec",
              use="BOOST NDN_CXX PROTOBUF YAML-CPP",
              install_path="${PREFIX}/lib")

    for app in bld.path.ant_glob('examples/*.cpp'):
        bld(features=['cxx', 'cxxprogram'],
            source = app,
            target = '%s' % (str(app.change_ext('','.cpp'))),
            includes="libndnpaxos libzfec", 
            use="ndnpaxos",
            ) 

    for app in bld.path.ant_glob('test/*.cpp'):
        bld(features=['cxx', 'cxxprogram'],
            source = app,
            target = '%s' % (str(app.change_ext('','.cpp'))),
            includes="libndnpaxos", 
            use="ndnpaxos",
            ) 

def _enable_debug(conf):
    if Options.options.debug:
        Logs.pprint("PINK", "Debug support enabled")
        conf.env.append_value("CFLAGS", "-Wall -Wno-unused -pthread -O0 -g -rdynamic -fno-omit-frame-pointer -fno-strict-aliasing".split())
        conf.env.append_value("CXXFLAGS", "-Wall -Wno-unused -pthread -O0 -g -rdynamic -fno-omit-frame-pointer -fno-strict-aliasing".split())
        conf.env.append_value("LINKFLAGS", "-Wall -Wno-unused -O0 -g -rdynamic -fno-omit-frame-pointer".split())
    else:
        conf.env.append_value("CFLAGS", "-Wall -O2 -pthread".split())
        conf.env.append_value("CXXFLAGS", "-Wall -O2 -pthread".split())
#        conf.env.append_value("LINKFLAGS", "-Wall -O2 -pthread".split())

    if os.getenv("CLANG") == "1":
        Logs.pprint("PINK", "Use clang as compiler")
        conf.env.append_value("C", "clang++")

def _enable_log(conf):
    if Options.options.log == 'trace':
        Logs.pprint("PINK", "Log level set to trace")
        conf.env.append_value("CFLAGS", "-DLOG_LEVEL=6")
        conf.env.append_value("CXXFLAGS", "-DLOG_LEVEL=6")
    elif Options.options.log == 'debug':
        Logs.pprint("PINK", "Log level set to debug")
        conf.env.append_value("CFLAGS", "-DLOG_LEVEL=5")
        conf.env.append_value("CXXFLAGS", "-DLOG_LEVEL=5")
    elif Options.options.log == 'info':
        Logs.pprint("PINK", "Log level set to info")
        conf.env.append_value("CFLAGS", "-DLOG_LEVEL=4")
        conf.env.append_value("CXXFLAGS", "-DLOG_LEVEL=4")
    elif Options.options.log == '':
        pass
    else:
        Logs.pprint("PINK", "unsupported log level")
#    if os.getenv("DEBUG") == "1":

def _enable_mode(conf):
    if Options.options.mode == 'RS':
        Logs.pprint("PINK", "Mode type set to RS")
        conf.env.append_value("CFLAGS", "-DMODE_TYPE=1")
        conf.env.append_value("CXXFLAGS", "-DMODE_TYPE=1")
    elif Options.options.mode == 'E':
        Logs.pprint("PINK", "Mode type set to Epaxos")
        conf.env.append_value("CFLAGS", "-DMODE_TYPE=2")
        conf.env.append_value("CXXFLAGS", "-DMODE_TYPE=2")
    elif Options.options.mode == 'RSII':
        Logs.pprint("PINK", "Mode type set to RSII")
        conf.env.append_value("CFLAGS", "-DMODE_TYPE=3")
        conf.env.append_value("CXXFLAGS", "-DMODE_TYPE=3")
    elif Options.options.mode == '':
        pass
    else:
        Logs.pprint("PINK", "unsupported Mode type")

def _enable_static(conf):
    if os.getenv("STATIC") == "1":
        Logs.pprint("PINK", "statically link")
        conf.env.append_value("CFLAGS", "-static")
        conf.env.append_value("CXXFLAGS", "-static")
        conf.env.append_value("LINKFLAGS", "-static")
        pargs.append('--static')

def _enable_pic(conf):
    conf.env.append_value("CFLAGS", "-fPIC")
    conf.env.append_value("CXXFLAGS", "-fPIC")
    conf.env.append_value("LINKFLAGS", "-fPIC")

def _enable_cxx11(conf):
    Logs.pprint("PINK", "C++11 features enabled")
    if sys.platform == "darwin":
        conf.env.append_value("CXXFLAGS", "-stdlib=libc++")
        conf.env.append_value("LINKFLAGS", "-stdlib=libc++")
    conf.env.append_value("CXXFLAGS", "-std=c++0x")
