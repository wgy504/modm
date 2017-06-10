#!/usr/bin/env python2
# -*- coding: utf-8 -*-
#
# Copyright (c) 2010-2013, Fabian Greif
# Copyright (c) 2011, 2013, Georgi Grinshpun
# Copyright (c) 2012-2013, Sascha Schade
# Copyright (c) 2012, 2014, 2016, Niklas Hauser
# Copyright (c) 2013-2014, 2016, Kevin Läufer
# Copyright (c) 2016, Daniel Krebs
#
# This file is part of the modm project.
#
# This Source Code Form is subject to the terms of the Mozilla Public
# License, v. 2.0. If a copy of the MPL was not distributed with this
# file, You can obtain one at http://mozilla.org/MPL/2.0/.
# -----------------------------------------------------------------------------

import re, os, sys
import xml.etree.ElementTree as et
import xml.parsers.expat
import ConfigParser
import SCons.Node
import SCons.Errors

def listify(node):
	return [node,] if (not isinstance(node, list) and \
					   not isinstance(node, SCons.Node.NodeList)) else node

# -----------------------------------------------------------------------------
class ParserException(Exception):
	pass

class Parser(ConfigParser.RawConfigParser):

	def read(self, filename):
		try:
			return ConfigParser.RawConfigParser.read(self, filename)
		except ConfigParser.ParsingError as e:
			raise SCons.Errors.UserError(str(e) + '\n')

	def get(self, section, option, default=None):
		try:
			return ConfigParser.RawConfigParser.get(self, section, option)
		except (ConfigParser.NoOptionError,
				ConfigParser.NoSectionError), e:
			if default != None:
				return default
			else:
				raise ParserException(e)

	def getboolean(self, section, option, default=None):
		try:
			return ConfigParser.RawConfigParser.getboolean(self, section, option)
		except (ConfigParser.NoOptionError,
				ConfigParser.NoSectionError,
				ParserException), e:
			if default != None:
				return default
			else:
				raise ParserException(e)

	def items(self, section):
		try:
			return ConfigParser.RawConfigParser.items(self, section)
		except (ConfigParser.NoOptionError,
				ConfigParser.NoSectionError), e:
			raise ParserException(e)

# -----------------------------------------------------------------------------
class FileList(list):
	"""
	Special list for file object. Checks if a file is already in the list
	when adding it.
	"""
	def append(self, item):
		if hasattr(item, '__getitem__'):
			self.extend(item)
		else:
			self.__append(item)

	def __append(self, item):
		if not isinstance(item, SCons.Node.Node):
			item = SCons.Node.FS.default_fs.File(str(item))
			# print dir(item)
		if not self.__contains__(item):
			list.append(self, item)

	def extend(self, l):
		for item in l:
			self.append(item)

	def __iadd__(self, item):
		self.append(item)
		return self

# -----------------------------------------------------------------------------
class Scanner:

	HEADER = ['.h', '.hpp']
	SOURCE = ['.cpp', '.c', '.sx', '.S', '.s']

	def __init__(self, env, unittest=None):
		""" Constructor

		Keyword arguments:
		env		 -	SCons environment
		unittest -	This variable has three states:
					None  => select all files
					False => exclude files from subfolders named 'test'
					True  => select only files in folders named 'test'
		"""
		self.env = env
		self.unittest = unittest

	def scan(self, path, ignore=None):
		""" Scan directories for source files

		Provides the following attributes to collect the results:
		sources		- list of source files
		header		- list of header files
		defines		- dictionary with defines needed by the source files
		"""
		if ignore is None:
			ignoreList = []
		else:
			ignoreList = listify(ignore)
		pathlist = listify(path)

		if 'MODM_BOARD_PATH' in self.env:
			pathlist.append(self.env['MODM_BOARD_PATH'])

		self.sources = FileList()
		self.header = FileList()
		self.defines = {}

		for basepath in pathlist:
			for path, directories, files in os.walk(basepath):
				if self._fileInList (path, ignoreList):
					directories = self._excludeDirectories(directories)
					continue

				if 'build.cfg' in files:
					parser = Parser()
					parser.read(os.path.join(path, 'build.cfg'))

					if self._directoryShouldBeExcluded(parser):
						directories = self._excludeDirectories(directories)
						continue

					try:
						for item in parser.items('defines'):
							self.defines[item[0]] = item[1]
					except ParserException:
						pass

				if 'module.xml' in files:
					try:
						xmltree = et.parse(os.path.join(path, 'module.xml')).getroot()
					except (OSError, xml.parsers.expat.ExpatError, xml.etree.ElementTree.ParseError) as e:
						env.Error("while parsing xml-file '%s': %s" % (filename, e))
					module_name = xmltree[0].get('name').lower()
					if not module_name in self.env['MODM_ACTIVE_MODULES']:
						self.env.Debug('Excluding module "%s"' % (module_name))
						directories = self._excludeDirectories(directories)
						continue

				if self.unittest is None or not (self.unittest ^ path.endswith(os.sep + 'test')):
					# only check this directory for files if all directories
					# should be check or unittest is active and directory
					# ends with test.
					p = path + '/*'
					for source in self.SOURCE:
						files = self.env.Glob(p + source)

						for file in files[:]:
							for ignoreFile in ignoreList:
								if self._samefile(str(file), ignoreFile):
									files.remove(file)

						self.sources.extend(files)

					for header in self.HEADER:
						self.header.extend(self.env.Glob(p + header))

	def __iadd__(self, item):
		self.append(item)
		return self

	def append(self, files):
		for file in listify(files):
			filename = str(file)
			extension = os.path.splitext(filename)[1]
			if extension in self.SOURCE:
				self.sources.append(file)
			elif extension in self.HEADER:
				self.header.append(file)

	def _directoryShouldBeExcluded(self, parser):
		try:
			target = parser.get('build', 'target')

			if target.startswith('!'):
				target = target[1:]
				invert = True
			else:
				invert = False

			if not re.search(target, self.env['ARCHITECTURE']):
				return True ^ invert
			else:
				return False ^ invert
		except ParserException:
			return False

	def _excludeDirectories(self, directories):
		# if the this directory should be excluded, remove all the
		# subdirectories from the list to exclude them as well
		tempDirectories = directories[:]
		for d in tempDirectories:
			directories.remove(d)
		return directories

	def _samefile(self, src, dst):
		# Macintosh, Unix
		if hasattr(os.path,'samefile'):
			try:
				return os.path.samefile(src, dst)
			except OSError:
				return False

		# All other platforms: check for same pathname.
		return (os.path.normcase(os.path.abspath(src)) ==
				os.path.normcase(os.path.abspath(dst)))

	def __str__(self):
		string = "Source files: "
		if self.sources:
			for s in self.sources:
				string += "\n  " + str(s)
		else:
			string += "None"

		string += "\nHeader files: "
		if self.header:
			string += "\nHeader files:"
			for h in self.header:
				string += "\n  " + str(h)
		else:
			string += "None"

		string += "\nDefines: "
		if self.defines:
			for d in self.defines:
				string += "\n  " + str(d)
		else:
			string += "None"
		return string

	def _fileInList(self, file, fileList):
		# returns true if file (or directory) is contained in fileList
		for f in fileList:
			if self._samefile (file, f):
				return True
		return False

# -----------------------------------------------------------------------------
def generate(env, **kw):
	def generate_configparser(env):
		return Parser()

	env.AddMethod(generate_configparser, 'ConfigParser')

	def find_files(env, path, unittest=None, ignore=None):
		scanner = Scanner(env, unittest)
		scanner.scan(path=path, ignore=ignore)
		return scanner

	env.AddMethod(find_files, 'FindFiles')

def exists(env):
	return True
