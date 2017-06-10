#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Copyright (c) 2013-2015, Kevin Laeufer
# Copyright (c) 2013, Martin Rosekeit
# Copyright (c) 2013-2016, Niklas Hauser
# Copyright (c) 2014, 2016, Sascha Schade
# Copyright (c) 2015, Georgi Grinshpun
# Copyright (c) 2016, Fabian Greif
# Copyright (c) 2016, Daniel Krebs
#
# This file is part of the modm project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
# -----------------------------------------------------------------------------
#
# DESCRIPTION
#
# This tool checks which files are needed for a specific target
# using the xml device file and adds a custom builder for all template files
# and for other files that need to be copied to the generated folder
#
# WARNING: Do NOT name this file platform.py because this overrides a
#          different platform module used by /usr/lib/scons/SCons/Tool/tex.py


from SCons.Script import *
import os
from string import Template

from configfile import Scanner # for header and source file endings

# add device_file module from tools to path
# this is apparently not pythonic, but I see no other way to do this
# without polluting the site_tools directory or having duplicate code
sys.path.append(os.path.join(os.path.dirname(__file__), '..', '..', 'tools', 'device_files'))
from device import DeviceFile
from device_identifier import DeviceIdentifier
from driver import DriverFile
from parameters import ParameterDB


#------------------------------------------------------------------------------
#
def platform_tools_find_device_file(env):
	architecture_path = os.path.join(env['MODM_LIBRARY_PATH'], 'modm', 'architecture')
	device = env['MODM_DEVICE']
	env.Debug("Device String: %s" % device)

	id = DeviceIdentifier(device)

	# Find Device File
	xml_path = os.path.join(env['MODM_PLATFORM_PATH'], 'devices', id.platform)
	files = []
	device_file = None

	if id.platform == 'hosted':
		file = os.path.join(xml_path, id.family + '.xml')
		if os.path.exists(file):
			device_file = file

	elif id.platform == 'avr':
		for file in os.listdir(xml_path):
			if id.family in file:
				fileArray = file.replace(id.family,"").replace(".xml","").split("-")
				names = fileArray[0].split("_")
				types = fileArray[1].split("_")
				pinIdString = fileArray[2] if len(fileArray) > 2 else None
				if id.name in names:
					type  = id.type
					if type == None:
						type = 'none'
					if type in types:
						if id.family == 'xmega' and id.pin_id != pinIdString:
							continue
						device_file = os.path.join(xml_path, file)
						break

	elif id.platform == 'stm32':
		for file in os.listdir(xml_path):
			if 'stm32'+id.family in file:
				fileArray = file.replace('stm32'+id.family[0],'').replace('.xml','').split('-')
				if len(fileArray) < 3:
					continue
				names = fileArray[0].split('_')
				pins = fileArray[1].split('_')
				sizes = fileArray[2].split('_')
				if id.name in names and id.pin_id in pins and id.size_id in sizes:
					device_file = os.path.join(xml_path, file)
					break
	else:
		temp_device = device
		while temp_device != None and len(temp_device) > 0:
			device_file = os.path.join(xml_path, temp_device + '.xml')
			files.append(device_file)
			if os.path.isfile(device_file):
				break
			else:
				temp_device = temp_device[:-1]
				device_file = None

	# Check for error
	if device_file == None:
		env.Error("MODM Error: Could not find xml device file." + os.linesep)
		# for f in files:
		#	env.Error("Tried: " + f + os.linesep)
		Exit(1)

	# Now we need to parse the Xml File
	env.Debug("Found device file: " + device_file)
	env['MODM_DEVICE_FILE'] = DeviceFile(device_file, env.GetLogger())

	if id.platform == 'hosted':
		env['ARCHITECTURE'] = 'hosted/' + id.family
	else:
		# for microcontrollers architecture = core
		env['ARCHITECTURE'] = env['MODM_DEVICE_FILE'].getProperties(device)['core']
		if id.platform == 'avr':
			env['AVRDUDE_DEVICE'] = env['MODM_DEVICE_FILE'].getProperties(device)['mcu']

#------------------------------------------------------------------------------
# env['MODM_PLATFORM_PATH'] is used for absolute paths
# architecture_path for relative build paths
def platform_tools_generate(env, architecture_path):

	env['MODM_PLATFORM_GENERATED_PATH'] = \
		os.path.join(env['MODM_BUILDPATH'], 'generated_platform')


	device = env['MODM_DEVICE']

	# Initialize Return Lists/Dicts
	sources = []
	defines = {}
	# make paths
	platform_path = os.path.join(architecture_path, 'platform')
	generated_path = env['MODM_PLATFORM_GENERATED_PATH']


	dev = env['MODM_DEVICE_FILE']

	# Parse Properties
	prop = dev.getProperties(device)
	env.Debug("Found properties: %s" % prop)
	defines = prop['defines']
	device_headers = prop['headers']

	if not env['ARCHITECTURE'].startswith('hosted'):
		# Set Size
		env['DEVICE_SIZE'] = { "flash": prop['flash'], "ram": prop['ram'], "eeprom": prop['eeprom'] }

		if not env['ARCHITECTURE'].startswith('avr'):
			# Find Linkerscript:
			linkerfile = os.path.join(env['MODM_PLATFORM_GENERATED_PATH'], 'driver', 'core', 'cortex', 'linkerscript.ld')
			# make the program depend directly on the linkerscript, so it re-links when something changed!
			Depends(os.path.join(env['MODM_BUILDPATH'], "..", env['MODM_PROJECT_NAME'] + ".elf"), linkerfile)
			# TODO: This is a hack for unittests on stm32. Their elf file is called "executable" instead of the project name.
			#       This should be properly solved using a custom env.Program wrapper!
			#       cc @ekiwi
			Depends(os.path.join(env['MODM_BUILDPATH'], "..", "executable.elf"), linkerfile)

			linkdir, linkfile = os.path.split(linkerfile)
			linkdir = linkdir.replace(env['MODM_ROOTPATH'], "${MODM_ROOTPATH}")
			env['LINKPATH'] = str(linkdir)
			env['LINKFILE'] = str(linkfile)

	# Loop through Drivers
	driver_list = []
	type_id_headers = []
	drivers = dev.getDriverList(device, env['MODM_PLATFORM_PATH'])
	for driver in drivers:
		ddic = {} # create dictionary describing the driver
		d = DriverFile.fromDict(driver, env['MODM_PARAMETER_DB'], env.GetLogger())
		ddic['name'] = d.name
		ddic['type'] = d.type
		ddic['headers'] = []
		build = d.getBuildList(env['MODM_PLATFORM_PATH'], env['MODM_DEVICE'])
		for f in build:
			src = os.path.join(platform_path, f[0])
			tar = os.path.join(generated_path, f[1])
			if len(f) == 3:
				res = env.Jinja2Template(target = tar, source = src, substitutions = f[2])
			else:
				res = env.Command(tar, src, Copy("$TARGET", "$SOURCE"))
			# check if target is header file
			if os.path.splitext(tar)[1] in Scanner.HEADER:
				if not f[1].endswith("_impl.hpp"):
					ddic['headers'].append(f[1]) # append path relative to platform dir
			# or source file
			elif os.path.splitext(tar)[1] in Scanner.SOURCE:
				sources.append(res)
			# check if target is "type_ids.hpp"
			if os.path.basename(tar) == "type_ids.hpp":
				type_id_headers.append(f[1]) # append path relative to platform dir
		driver_list.append(ddic)

	####### Generate Header Templates #########################################
	# Show SCons how to build the architecture/platform.hpp file:
	# each platform will get it's own platform.hpp in 'generated_platform_xxx/include_platform_hack'
	# Choosing this folder and appending to CPPPATH ensures the usage: #include <modm/architecture/platform.hpp>

	src = os.path.join(platform_path, 'platform.hpp.in')
	tar = env.Buildpath(os.path.join(architecture_path, 'platform.hpp'))
	sub = {'include_path': '../../../generated_platform/drivers.hpp'}
	if 'MODM_BOARD' in env:
		sub['board'] = env['MODM_BOARD']
	env.Jinja2Template(target = tar, source = src, substitutions = sub)

	#append and return additional CPPPATH
	cppIncludes = [env.Buildpath('.')]
	env.AppendUnique(CPPPATH = cppIncludes)

	# Show SCons how to build the drivers.hpp.in file:
	src = os.path.join(platform_path, 'drivers.hpp.in')
	tar = os.path.join(generated_path, 'drivers.hpp')
	sub = {'drivers': driver_list}
	env.Jinja2Template(target = tar, source = src, substitutions = sub)

	# Show SCons how to build device.hpp.in file:
	src = os.path.join(platform_path, 'device.hpp.in')
	tar = os.path.join(generated_path, 'device.hpp')
	sub = {'headers': device_headers}
	env.Jinja2Template(target = tar, source = src, substitutions = sub)

	# Show SCons how to build type_ids.hpp.in file:
	src = os.path.join(platform_path, 'type_ids.hpp.in')
	tar = os.path.join(generated_path, 'type_ids.hpp')
	sub = {'headers': type_id_headers}
	env.Jinja2Template(target = tar, source = src, substitutions = sub)

	return sources, defines, cppIncludes

############## Template Tests #################################################
# -----------------------------------------------------------------------------
def test_item(dic, item_key, item_value, starts_with=False):
	if item_key not in dic:
		return False
	if starts_with and dic[item_key].startswith(item_value):
		return True
	if not starts_with and dic[item_key] == item_value:
		return True
	return False

############## Template Filters ###############################################
# -----------------------------------------------------------------------------
def filter_get_ports(gpios):
	"""
	This filter accepts a list of gpios as e.g. used by the stm32af driver
	and tries to extract information about port which is returned as a list
	of dictionaries with the following structure:
	{'name': "C", 'startPin': 0, 'width': 16}
	"""
	# collect information on available gpios
	port_ids = {}
	for gpio in gpios:
		if not gpio['port'] in port_ids:
			port_ids[gpio['port']] = [0] * 16
		port_ids[gpio['port']][int(gpio['id'])] = 1
	# create port list
	ports = []
	for name, ids in port_ids.iteritems():
		# if the port consists of at least one gpio pin
		if 1 in ids:
			port = {}
			port['name'] = name
			# find start pin as well as width
			ii = ids.index(1)
			port['startPin'] = ii
			while ii < 16 and ids[ii] == 1:
				ii = ii + 1
			port['width'] = ii - port['startPin']
			ports.append(port)
	return ports

# -----------------------------------------------------------------------------
def filter_get_adcs(gpios):
	"""
	This filter accepts a list of gpios as e.g. used by the stm32af driver
	and tries to extract information about what ADC modules are references
	which is returned as a list of instances
	['0', '1']
	"""
	# collect information on available gpios
	adcs = []
	for gpio in gpios:
		if 'afs' in gpio:
			for af in gpio['afs']:
				if 'type' in af and af['type'] == "analog" and af['peripheral'] not in adcs:
					adcs.append(af['peripheral'])
	
	adc_list = []
	for adc in adcs:
		# If the ADC has no id (e.g. for STM32F0) use '0' as id. 
		if len(adc) < 4:
			adc_list.append('0')
		else:
			adc_list.append(adc[3])
	return adc_list

# -----------------------------------------------------------------------------
def filter_letter_to_num(letter):
	"""
	This filter turns one letter into a number.
	A is 0, B is 1, etc. This is not case sensitive.
	"""
	letter = letter[0].lower()
	return ord(letter) - ord('a')

# -----------------------------------------------------------------------------
###################### Generate Platform Tools ################################
def generate(env, **kw):
	env['MODM_PLATFORM_PATH'] = \
		os.path.join(env['MODM_LIBRARY_PATH'], 'modm', 'architecture', 'platform')

	# Create Parameter DB and parse User parameters
	env['MODM_PARAMETER_DB'] = ParameterDB(env['MODM_USER_PARAMETERS'], env.GetLogger()).toDictionary()

	# Add Method to Parse XML Files, and create Template / Copy Dependencies
	env.AddMethod(platform_tools_generate, 'GeneratePlatform')
	env.AddMethod(platform_tools_find_device_file, 'FindDeviceFile')

	# Add Filter for Gpio Drivers to Template Engine
	env.AddTemplateJinja2Filter('getPorts',    filter_get_ports)
	env.AddTemplateJinja2Filter('letterToNum', filter_letter_to_num)
	env.AddTemplateJinja2Filter('getAdcs', filter_get_adcs)

	########## Add Template Tests #############################################
	# Generaic Tests (they accept a string)
	def test_platform(target, platform):
		return test_item(target, 'platform', platform)
	env.AddTemplateJinja2Test('platform', test_platform)
	def test_family(target, family):
		return test_item(target, 'family', family)
	env.AddTemplateJinja2Test('family', test_family)
	def test_name(target, name):
		return test_item(target, 'name', name)
	env.AddTemplateJinja2Test('name', test_name)
	def test_type(target, type):
		return test_item(target, 'type', type)
	env.AddTemplateJinja2Test('type', test_type)
	def test_size_id(target, size_id):
		return test_item(target, 'size-id', size_id)
	env.AddTemplateJinja2Test('size_id', test_size_id)
	def test_pin_id(target, pin_id):
		return test_item(target, 'pin-id', pin_id)
	env.AddTemplateJinja2Test('pin_id', test_pin_id)
	def test_core(target, core, starts_with=False):
		return test_item(target, 'core', core, starts_with)
	env.AddTemplateJinja2Test('core', test_core)

	# Core Tests
	def test_cortex(target):
		return test_core(target, 'cortex', True)
	env.AddTemplateJinja2Test('cortex', test_cortex)
	def test_cortex_m0(target):
		return test_core(target, 'cortex-m0')
	env.AddTemplateJinja2Test('cortex_m0', test_cortex_m0)
	def test_cortex_m3(target):
		return test_core(target, 'cortex-m3')
	env.AddTemplateJinja2Test('cortex_m3', test_cortex_m3)
	def test_cortex_m4(target):
		return test_core(target, 'cortex-m4', True)
	env.AddTemplateJinja2Test('cortex_m4', test_cortex_m4)
	def test_cortex_m7(target):
		return test_core(target, 'cortex-m7', True)
	env.AddTemplateJinja2Test('cortex_m7', test_cortex_m7)
	def test_cortex_m4f(target):
		return test_core(target, 'cortex-m4f')
	env.AddTemplateJinja2Test('cortex_m4f', test_cortex_m4f)
	def test_cortex_m7f(target):
		return test_core(target, 'cortex-m7f')
	env.AddTemplateJinja2Test('cortex_m7f', test_cortex_m7f)
	def test_cortex_m7fd(target):
		return test_core(target, 'cortex-m7fd')
	env.AddTemplateJinja2Test('cortex_m7fd', test_cortex_m7fd)

	# Platform Tests
	def test_is_stm32(target):
		return test_platform(target, 'stm32')
	env.AddTemplateJinja2Test('stm32', test_is_stm32)
	def test_is_lpc(target):
		return test_platform(target, 'lpc')
	env.AddTemplateJinja2Test('lpc', test_is_lpc)
	def test_is_avr(target):
		return test_platform(target, 'avr')
	env.AddTemplateJinja2Test('avr', test_is_avr)

	# STM32 Family Test
	def test_is_stm32f0(target):
		return test_platform(target, 'stm32') and test_family(target, 'f0')
	env.AddTemplateJinja2Test('stm32f0', test_is_stm32f0)
	def test_is_stm32l0(target):
		return test_platform(target, 'stm32') and test_family(target, 'l0')
	env.AddTemplateJinja2Test('stm32l0', test_is_stm32l0)
	def test_is_stm32f1(target):
		return test_platform(target, 'stm32') and test_family(target, 'f1')
	env.AddTemplateJinja2Test('stm32f1', test_is_stm32f1)
	def test_is_stm32l1(target):
		return test_platform(target, 'stm32') and test_family(target, 'l1')
	env.AddTemplateJinja2Test('stm32l1', test_is_stm32l1)
	def test_is_stm32f2(target):
		return test_platform(target, 'stm32') and test_family(target, 'f2')
	env.AddTemplateJinja2Test('stm32f2', test_is_stm32f2)
	def test_is_stm32f3(target):
		return test_platform(target, 'stm32') and test_family(target, 'f3')
	env.AddTemplateJinja2Test('stm32f3', test_is_stm32f3)
	def test_is_stm32f4(target):
		return test_platform(target, 'stm32') and test_family(target, 'f4')
	env.AddTemplateJinja2Test('stm32f4', test_is_stm32f4)
	def test_is_stm32l4(target):
		return test_platform(target, 'stm32') and test_family(target, 'l4')
	env.AddTemplateJinja2Test('stm32l4', test_is_stm32l4)
	def test_is_stm32f7(target):
		return test_platform(target, 'stm32') and test_family(target, 'f7')
	env.AddTemplateJinja2Test('stm32f7', test_is_stm32f7)

# -----------------------------------------------------------------------------
def exists(env):
	return True
