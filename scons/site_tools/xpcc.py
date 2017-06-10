#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Copyright (c) 2010-2013, Fabian Greif
# Copyright (c) 2012-2014, 2016, Sascha Schade
# Copyright (c) 2012, 2014-2015, Georgi Grinshpun
# Copyright (c) 2012-2016, Niklas Hauser
# Copyright (c) 2013-2016, Kevin Laeufer
# Copyright (c) 2015-2016, Daniel Krebs
#
# This file is part of the modm project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
# -----------------------------------------------------------------------------

import os
import sys
import re
import string
import platform
import configfile as configparser
import textwrap
import getpass, subprocess
import glob

sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'tools', 'device_files'))
from device_identifier import DeviceIdentifier

from SCons.Script import *
import SCons.Tool		# to get the SCons default tool search path

# -----------------------------------------------------------------------------
def exclude_from_scanner(path, filename='build.cfg'):
	filename = os.path.join(path, filename)
	dir = os.path.dirname(filename)

	if not os.path.exists(dir):
		os.makedirs(dir)

	file = open(filename, 'w')
	file.write("""[build]\ntarget = None""")
	file.close()

# -----------------------------------------------------------------------------
def relocate_to_buildpath(env, path, strip_extension=False):
	""" Relocate path from source directory to build directory
	"""
	path = str(path)
	if strip_extension:
		path = os.path.splitext(path)[0]

	# needed when modm path has a symlink
	base = os.path.realpath(env['MODM_BASEPATH'])

	path = os.path.relpath(path, base)
	if path.startswith('..'):
		# if the file is not in a subpath of the current directory
		# build it in the root directory of the build path
		while path.startswith('..'):
			path = path[3:]
		#path = os.path.basename(path)

	return os.path.abspath(os.path.join(env['MODM_BUILDPATH'], path))

def check_architecture(env, architecture):
	if re.match(architecture, env['ARCHITECTURE']):
		return True
	else:
		return False

def show_defines_action(target, source, env):
	projectConfig = env['MODM_CONFIG']['defines'].keys()
	keys = env['MODM_LIBRARY_DEFINES'].keys()
	keys.sort()

	print ""
	print "MODM:"
	for key in keys:
		default = env['MODM_LIBRARY_DEFAULT_DEFINES'][key]
		if key in projectConfig:
			value = env['MODM_CONFIG']['defines'][key]
			projectConfig.remove(key)
			if value != default:
				print "  %s => %s (default: %s)" % (key.upper(), value, default)
				continue

		print "  %s => %s" % (key.upper(), default)

	if projectConfig:
		print "\nOther:"
		for key in projectConfig:
			print "  %s => %s" % (key.upper(), env['MODM_CONFIG']['defines'][key])
	print ""

def show_defines(env, alias="__show"):
	action = Action(show_defines_action, cmdstr="Show Defines:")
	return env.AlwaysBuild(env.Alias(alias, [], action))

def modm_library(env, buildpath=None):
	env.AppendUnique(CPPPATH = env['MODM_LIBRARY_PATH'])
	env.AppendUnique(CPPPATH = os.path.join(env['MODM_ROOTPATH'], 'ext'))

	backup = env['MODM_BUILDPATH'], env['MODM_BASEPATH']

	# set a new buildpath for the library
	if buildpath is None:
		buildpath = os.path.join(env['MODM_BUILDPATH'], 'libmodm')
	env['MODM_BUILDPATH'] = os.path.abspath(buildpath)
	env['MODM_BASEPATH'] = env['MODM_ROOTPATH']

	# exclude the buildpath from the FileScanner
	exclude_from_scanner(env['MODM_BUILDPATH'])

	# build the library
	library, defines = env.SConscript(
			os.path.join(env['MODM_ROOTPATH'], 'src', 'SConscript'),
			exports = 'env')

	# generate 'modm_config.hpp'
	env['MODM_LIBRARY_DEFINES'] = defines.copy()
	env['MODM_LIBRARY_DEFAULT_DEFINES'] = defines.copy()
	for key in defines.iterkeys():
		if key in env['MODM_CONFIG']['defines']:
			env['MODM_LIBRARY_DEFINES'][key] = env['MODM_CONFIG']['defines'][key]

	define_list = ["#define %s %s" % (key.upper(), value) \
				for key, value in env['MODM_LIBRARY_DEFINES'].iteritems()]
	define_list.sort()

	substitutions = {
		'defines': '\n'.join(define_list),
		'name': env['MODM_PROJECT_NAME']
	}

	file = env.Template(
			target = os.path.join(env['MODM_BUILDPATH'], 'modm_config.hpp'),
			source = os.path.join(env['MODM_ROOTPATH'],
								  'templates/modm_config.hpp.in'),
			substitutions = substitutions)

	env.PrependUnique(LIBS = ['modm'])
	env.PrependUnique(LIBPATH = [os.path.join(env['MODM_BUILDPATH'], 'src')])

	# restore original environment
	env['MODM_BUILDPATH'], env['MODM_BASEPATH'] = backup

	return library

def modm_communication_header(env, xmlfile, path='.', dtdPath=None, namespace='robot'):
	files  = env.SystemCppPackets(xmlfile, path=path, dtdPath=dtdPath, namespace=namespace)
	files += env.SystemCppIdentifier(xmlfile, path=path, dtdPath=dtdPath, namespace=namespace)
	files += env.SystemCppCommunication(xmlfile, path=path, dtdPath=dtdPath, namespace=namespace)
	files += env.SystemCppXpccTaskCaller(xmlfile, path=path, dtdPath=dtdPath, namespace=namespace)
	if 'communication' in env['MODM_CONFIG']:
		files += env.SystemCppPostman(
				target='postman',
				source=xmlfile,
				container=env['MODM_CONFIG']['communication']['container'],
				path=path,
				dtdPath=dtdPath,
				namespace=namespace)

	source = []
	for file in files:
		if file.name.endswith('.cpp'):
			source.append(file)

	return source

def generate_defines(env, filename='defines.hpp'):
	defines = env['MODM_CONFIG']['defines']
	substitutions = {
		'defines': '\n'.join(["#define %s %s" % (key.upper(), value) \
				for key, value in defines.iteritems()])
	}
	file = env.Template(
			target = filename,
			source = os.path.join(env['MODM_ROOTPATH'],
								  'templates/defines.hpp.in'),
			substitutions = substitutions)
	return file

def c_string_literal(env, string):
	"""
		Escapes string and adds quotes.
	"""
	# Warning: Order Matters! Replace '\\' first!
	e = [("\\", "\\\\"), ("\'", "\\\'"), ("\"", "\\\""), ("\t", "\\t"), ("\n", "\\n"), ("\f", ""), ("\r", "")]
	for r in e:
		string = string.replace(r[0], r[1])
	return "\"" + string + "\""

def detect_system(env):
	"""
		Provide an unified method to determine running platform.
	"""
	# will return lowcase names so it can be used building paths

	system_name = platform.system()
	standards = {'Darwin': 'darwin', 'Linux': 'linux',
						'Windows': 'windows' }

	if system_name in standards:
		return standards[platform.system()]

	if system_name.startswith('MSYS') or system_name.startswith('CYGWIN') or system_name.startswith('MINGW'):
		return 'windows'

	# if this fails, don't bother continuing, it can only escaladate
	env.Error("Could not detect the running platform, " + system_name + " unsupported. " \
		"Currently supported by modm : Linux, Windows (MSYS, Cygwin), Darwin")
	env.Exit(1)

def define_header(env, defines, header, comment, template="define_template.hpp.in"):
	"""
		Shows Scons how to build a header file containing defines.
		The headerfile will be created in the MODM_BUILDPATH
		and the MODM_BUILDPATH will be added to the include
		search path.
		:param defines: dictionary containing key value pairs
		:param header: name of the header, #include<${header}>
		:param comment: say because of what this file was created
	"""
	include_guard = header.upper().replace('.', '_')
	comment = textwrap.wrap(comment, 78-len("// "))
	comment = "\n".join(["// " + c for c in comment])
	#c = ""
	#while len(comment) > 0:
	#	c += "// " + comment[:70] + '\n'
	#	comment = comment[70:]
	define_list = ["#define %s %s" % (key.upper(), value) for key, value in defines.iteritems()]
	file = env.Template(
		target = os.path.join(env['MODM_BUILDPATH'], header),
		source = os.path.join(env['MODM_ROOTPATH'], 'templates', template),
		substitutions = {'defines':       '\n'.join(define_list),
		                 'include_guard': include_guard,
		                 'comment':       comment})
	env.AppendUnique(CPPPATH = env['MODM_BUILDPATH'])

def build_info_header(env):
	print('Building: Info Header modm_build_info.hpp')
	defines = {}
	defines['MODM_BUILD_PROJECT_NAME'] = env.CStringLiteral(env['MODM_PROJECT_NAME'])
	defines['MODM_BUILD_MACHINE'] = env.CStringLiteral(platform.node())
	defines['MODM_BUILD_USER'] = env.CStringLiteral(getpass.getuser())
	# Generate OS String
	if env.System() == 'linux':
		os = " ".join(platform.linux_distribution())
	elif env.System() == 'darwin':
		os = "Mac {0} ({2})".format(*platform.mac_ver())
	else:
		os = platform.system()
	defines['MODM_BUILD_OS'] = env.CStringLiteral(os)
	# This contains the version of the compiler that is used to build the project
	try:
		c = subprocess.check_output([env['CXX'], '--version']).split('\n', 1)[0]
	except Exception, e:
		env.Error("[CXX] compiler " + env['CXX'] + " is not in path or could not be executed")
		Exit(1)

	m = re.match("(?P<name>[a-z\-\+]+)[a-zA-Z\(\) ]* (?P<version>\d+\.\d+\.\d+)", c)
	if m: comp = "{0} {1}".format(m.group('name'), m.group('version'))
	else: comp = c
	defines['MODM_BUILD_COMPILER'] = env.CStringLiteral(comp)
	c = "Its content is created by a call to env.BuildInfoHeader() in your SConstruct file."
	env.DefineHeader(defines=defines, header="modm_build_info.hpp", comment=c)

#def get_compiler_version_string

# -----------------------------------------------------------------------------
def generate(env, **kw):
	# We make sure that the installed version is 2.6 or higher because
	# features from this version like os.path.relpath() are used.
	EnsurePythonVersion(2, 6)
	EnsureSConsVersion(1, 0)

	env.AddMethod(detect_system, 'System')

	# Import Logger Tool
	env.Tool('logger_tools')
	log_level = ARGUMENTS.get('LOG_LEVEL', None)
	if log_level == None:
		log_level = ARGUMENTS.get('ll', None)
	if log_level == None:
		log_level = 'warn'
	env.SetLogLevel(log_level)

	if 'TERM' in os.environ:
		# copy terminal settings from os.environ for color output
		env['ENV']['TERM'] = os.environ['TERM']

	# detect the rootpath to the modm folder
	rootpath = env.get('rootpath')

	if rootpath is None:
		# Check if a global environment variable exists that point to
		# the root of the modm-folder
		rootpath = ARGUMENTS.get('MODM_HOME', None)
		if rootpath is not None:
			env.Info("Use path from 'MODM_HOME': '%s'" % rootpath)

	if rootpath is None:
		# try to detect the rootpath
		sitepath = os.path.join('scons', 'site_tools')

		# DefaultToolpath is used when the modm toolpath is added via --site-dir=$/modm/scons
		searchpath = env.get('toolpath', []) + SCons.Tool.DefaultToolpath

		for path in searchpath:
			path = os.path.normpath(path)
			if path.endswith(sitepath):
				p = path[:-len(sitepath) - 1]
				# To avoid false detection of directories with a similar
				# name check if at least a 'src' directory exists
				if os.path.exists(os.path.join(p, 'src')):
					rootpath = p
					env.Info("Extracted path from tool path: '%s'" % rootpath)
					break

		if rootpath is None:
			env.Error("Could not detect the path to the modm-library. Use " \
				  "'Environment(rootpath=...)' to set the path to the root folder of modm.")
			env.Exit(1)
	rootpath = os.path.abspath(rootpath)

	# load the configuration file
	configfile = os.path.abspath(ARGUMENTS.get('config', env.get('configfile', 'project.cfg')))
	try:
		parser = configparser.Parser()
		parser.read(configfile)

		board = parser.get('build', 'board', 'undeclared')
		if board != 'undeclared':
			boardpath = os.path.join(rootpath, 'src', 'modm', 'architecture', 'platform', 'board', board)
			if not os.path.exists(boardpath):
				available = map(os.path.basename, glob.glob(os.path.dirname(boardpath) + '/*'))
				available.remove('build.cfg')
				env.Error("Board '{}' not found! Available boards are:\n\t- {}".format(board, "\n\t- ".join(available)))
				env.Exit(1)
			parser.read([os.path.join(boardpath, 'board.cfg'), configfile])
			env['MODM_BOARD'] = board
			env['MODM_BOARD_PATH'] = boardpath

		device = parser.get('build', 'device')

		hosted_device = env.System()

		if device == 'hosted':
			device = hosted_device
		elif device.startswith('hosted/'):
			device = device.replace('hosted/', '')

		clock  = parser.get('build', 'clock', 'NaN')

		architecture_deprecated = parser.get('build', 'architecture', 'deprecated')
		if architecture_deprecated != "deprecated":
			env.Warn("Specifying architecture is deprecated and replaced by only 'build.device'.")

		project_name = parser.get('general', 'name', 'deprecated')
		if project_name == 'deprecated':
			project_name = parser.get('build', 'name', os.path.basename(os.path.dirname(configfile)))
		else:
			env.Warn("Specifying 'general.name' is deprecated and replaced by 'build.name'!")
		env['MODM_PROJECT_NAME'] = project_name

		buildpath = env.get('buildpath')
		# put together build path
		if buildpath is None:
			buildpath = parser.get('build', 'buildpath', os.path.join(os.curdir, 'build/'))

		# optimization level can be declared in project.cfg
		optimization = parser.get('build', 'optimization', 's')
		# but can be overwritten by command line
		optimization = ARGUMENTS.get('optimization', optimization)

		env['MODM_ADDITIONAL_CCFLAGS'] = parser.get('build', 'ccflags', '').split()

		# load parameters if available
		if parser.has_section('parameters'):
			env['MODM_USER_PARAMETERS'] = parser.items('parameters')
		else:
			env['MODM_USER_PARAMETERS'] = []

		# load modules if available
		env['MODM_ACTIVE_MODULES'] = []
		if parser.has_section('modules'):
			modules = parser.items('modules')
			for m in modules:
				if m[1].lower() == 'include':
					env['MODM_ACTIVE_MODULES'].append(m[0].lower())

	except configparser.ParserException, msg:
		env.Error("Error parsing file configuration file '%s':\n%s" % (configfile, str(msg)))
		Exit(1)

	configuration = { 'defines': {}, 'environment': {} }
	for section in parser.sections():
		s = {}
		for option in parser.options(section):
			s[option] = parser.get(section, option)
		configuration[section] = s


	env['MODM_ROOTPATH'] = rootpath			# modm rootpath
	env['MODM_BASEPATH'] = os.curdir		# path of the current project
	env['MODM_LIBRARY_PATH'] = os.path.join(rootpath, 'src')
	env['MODM_CONFIG'] = configuration
	env['MODM_CONFIG_FILE'] = os.path.abspath(configfile)
	env['MODM_SYSTEM_BUILDER'] = os.path.join(rootpath, 'tools', 'system_design', 'builder')
	env['MODM_DEVICE'] = device 			# needed by the platform tools

	# compiler optimization
	optimization_levels = ['0', '1', '2', '3', 's']
	if optimization in optimization_levels:
		env['OPTIMIZATION'] = "-O{}".format(optimization)
	else:
		env.Error("scons: Unknown compiler optimization level '{}'!".format(optimization))
		env.Error("Available optimization levels are [{}]".format(', '.join(optimization_levels)))
		Exit(1)

	# tools which are used independently of the architecture
	env.Tool('template') # needs to be added before platform_tools
	env.Tool('unittest')
	env.Tool('configfile')
	env.Tool('helper')
	env.Tool('system_design')
	# Will validate the env['MODM_DEVICE'] and set env['ARCHITECTURE']
	env.Tool('platform_tools')

	env.Tool('git')
	env.Tool('qtcreator')

	env.FindDeviceFile()

	buildpath = string.Template(buildpath).safe_substitute({
				'arch': env['ARCHITECTURE'],
				'device': device,
				'name': env['MODM_PROJECT_NAME'],
				'modmpath': rootpath
			})
	buildpath = os.path.abspath(buildpath)
	# exclude the buildpath from the FileScanner
	exclude_from_scanner(buildpath)
	env['MODM_BUILDPATH'] = buildpath

	env['LIBS'] = ['']
	env['LIBPATH'] = []
	env['CPPPATH'] = []

	# architecture specific settings and tools
	if env['ARCHITECTURE'] in ['avr8', 'avr8l']:
		env['AVR_DEVICE'] = device
		env['AVR_CLOCK'] = clock
		env['LIBS'] = ['']
		env['LIBPATH'] = []

		env.Tool('avr')

		if parser.has_section('avrdude'):
			env.Tool('avrdude')
			env['AVRDUDE_PROGRAMMER'] = parser.get('avrdude', 'programmer')
			env['AVRDUDE_PORT'] = parser.get('avrdude', 'port')
			env['AVRDUDE_OPTIONS'] = ""
			env['AVRDUDE_BAUDRATE'] = []
			env['AVRDUDE_OPTIONS'] = []
			for key, value in configuration['avrdude'].items():
				if key == 'baudrate':
					env['AVRDUDE_BAUDRATE'] = value
				if key == 'options':
					env['AVRDUDE_OPTIONS'] = value
			env['AVR_FUSEBITS'] = []

			if 'fusebits' in configuration:
				if device.startswith('atmega') or device.startswith('attiny') or device.startswith('at90can'):
					fuses = ['lfuse', 'hfuse', 'efuse']
					for key, value in configuration['fusebits'].items():
						if key not in fuses:
							env.Error("Unknown fusebit '%s'! Allowed values are '%s'!" % \
									(key, "', '".join(fuses)))
							Exit(1)
						env.Append(AVR_FUSEBITS = {key: value} )
				elif device.startswith('atxmega'):
					fuses = ['fuse0', 'fuse1', 'fuse2', 'fuse4', 'fuse5']
					for key, value in configuration['fusebits'].items():
						if key not in fuses:
							env.Error("Unknown fusebit '%s'! Allowed values are '%s'!" % \
									(key, "', '".join(fuses)))
							Exit(1)
						env.Append(AVR_FUSEBITS = {key: value} )
				else:
					env.Warn("Ignoring 'fusebit' section in project configuration. Unknown device %s" % device)

		# path to the headers of a very small and incomplete libstdc++ implementation
		env.Append(CPPPATH = [os.path.join(rootpath, 'src', 'stdc++')])

	elif env['ARCHITECTURE'].startswith('hosted'):
		env['HOSTED_DEVICE'] = device
		env.Tool('hosted')
	elif env['ARCHITECTURE'] in ['cortex-m0', 'cortex-m0+', 'cortex-m3', 'cortex-m4', 'cortex-m4f', 'cortex-m7', 'cortex-m7f', 'cortex-m7fd']:
		if env['ARCHITECTURE'] in ['cortex-m7f', 'cortex-m7fd']:
			env['ARM_ARCH'] = 'cortex-m7'
		elif env['ARCHITECTURE'] == 'cortex-m4f':
			env['ARM_ARCH'] = 'cortex-m4'
		elif env['ARCHITECTURE'] == 'cortex-m0+':
			env['ARM_ARCH'] = 'cortex-m0p'
		else:
			env['ARM_ARCH'] = env['ARCHITECTURE']
		env['ARM_DEVICE'] = device
		env['ARM_CLOCK'] = clock
		if clock != "NaN" and not 'freertos' in env['MODM_ACTIVE_MODULES']:
			env.Warn("Specifying clock for ARM devices is deprecated.")

		env.Tool('arm')
		env.Tool('dfu_stm32_programmer')

		# load openocd tool if required
		program_type = None
		if parser.has_section('program'):
			program_type = parser.get('program', 'tool')
		else:
			for ptype in ['openocd', 'black_magic_probe', 'stlink', 'lpclink']:
				if parser.has_section(ptype):
					program_type = ptype
					break
		try:
			if program_type == 'openocd':
				env.Tool('openocd')
				env.Tool('openocd_remote')
				env['OPENOCD_CONFIGFILE'] = parser.get('openocd', 'configfile', 'openocd.cfg')
				env['OPENOCD_COMMANDS'] = parser.get('openocd', 'commands', 'default')
				env['OPENOCD_REMOTE_HOST'] = parser.get('openocd', 'remote_host', 'localhost')
				env['OPENOCD_REMOTE_USER'] = parser.get('openocd', 'remote_user', 'pi')
			if program_type == 'black_magic_probe':
				env.Tool('black_magic_probe')
				env['BLACK_MAGIC_PROBE_PORT'] = parser.get('black_magic_probe', 'port')
			elif program_type == 'stlink':
				env.Tool('stlink')
			elif program_type == 'lpclink':
				env['LPCLINK_PATH'] = parser.get('lpclink', 'basepath', 'default')
				env.Tool('lpclink')
		except configparser.ParserException as e:
			env.Error("Error in Configuration: %s" % e)
			Exit(1)

		if parser.has_section('debug'):
			if parser.get('debug', 'tool') == 'gdb':
				env['GDB_PORT'] = parser.get('debug', 'gdbport')
		else:
			env['GDB_PORT'] = parser.get('gdb', 'port', '3333')
		try:
			env.Tool('gdb')
		except configparser.ParserException as e:
			env.Error("Error in Configuration: %s" % e)

	else:
		env.Error("modm Error: Unknown architecture '%s'!" % env['ARCHITECTURE'])
		Exit(1)

	# append all values from environment section to the real environment
	#
	# adding * to a key name will add a space before and append to existing values
	# adding / to a key name will replace any existing value
	# raw key name means it will append to existing env value without any adjustment
	for key, value in configuration['environment'].iteritems():
		replace = False
		if '/' in key:
			replace = True
		if '*' in key or '/' in key:
			if '*' in key:
				value = " " + value
			key = key[:-1]
		if key.upper() == "CPPPATH":
			value = value.split(':')

		if replace:
			env.Replace(**{ key.upper(): value } )
		else:
			env.Append(**{ key.upper(): value } )

	# append defines from user config
	user_conf = {}
	for key,value in configuration['defines'].items():
		user_conf[key.upper()] = value
	env.Append(CPPDEFINES = user_conf)

	# These emitters are used to build everything not in place but in a
	# separate build-directory.
	def defaultEmitter(target, source, env):
		targets = []
		for file in target:
			# relocate the output to the buildpath
			filename = env.Buildpath(file.path)
			targets.append(filename)
		return targets, source

	env['BUILDERS']['Object'].add_emitter('.cpp', defaultEmitter)
	env['BUILDERS']['Object'].add_emitter('.c', defaultEmitter)
	env['BUILDERS']['Object'].add_emitter('.sx', defaultEmitter)
	env['BUILDERS']['Object'].add_emitter('.S', defaultEmitter)

	env['LIBEMITTER'] = defaultEmitter
	env['PROGEMITTER'] = defaultEmitter

	env.AddMethod(relocate_to_buildpath, 'Buildpath')
	env.AddMethod(check_architecture, 'CheckArchitecture')
	env.AddMethod(show_defines, 'ShowDefines')
	env.AddMethod(modm_library, 'ModmLibrary')
	env.AddMethod(modm_communication_header, 'XpccCommunication')
	env.AddMethod(generate_defines, 'Defines')
	env.AddMethod(c_string_literal, 'CStringLiteral')
	env.AddMethod(define_header, 'DefineHeader')
	env.AddMethod(build_info_header, 'BuildInfoHeader')

def exists(env):
	return True
