#!/usr/bin/env python
# -*- coding: utf-8 -*-

class InstallerItem:

	def __init__(self,name="",version="",category="",debpath="",createdesktop=False,configpath="",apppath="",status="未安装",regfiles=[],desktopnames=[],iconnames=[]):
		self.name = name
		self.version = version
		self.category = category
		self.debpath = debpath
		self.createdesktop = createdesktop
		self.configpath = configpath
		self.apppath = apppath
		self.status = status
		self.regfiles = regfiles
		self.desktopnames = desktopnames
		self.iconnames = iconnames

		

	def set_name(self,name):
		self.name = name

	def set_version(self,version):
		self.version = version
	
	def set_category(self,category):
		self.category = category

	def set_debpath(self,debpath):
		self.debpath = debpath

	def set_createdesktop(self,createdesktop):
		self.createdesktop = createdesktop

	def set_configpath(self,configpath):
		self.configpath = configpath

	def set_apppath(self,apppath):
		self.apppath = apppath

	def set_status(self,status):
		self.status = status

	def set_regfiles(self,regfiles):
		self.regfiles = regfiles

	def set_desktopnames(self,desktopnames):
		self.desktopnames = desktopnames

	def set_iconnames(self,iconnames):
		self.iconnames = iconnames


class UserLevelInstallerItem:

	def __init__(self,name="",zh_name="",version="",category="",level='1',debpath="",createdesktop=False,configpath="",apppath="",status="未安装",regfiles=[],desktopnames=[],iconnames=[],enable=False,disable=False,speed=False):
		self.name = name
		self.zh_name = zh_name
		self.version = version
		self.category = category
		self.level = level
		self.debpath = debpath
		self.createdesktop = createdesktop
		self.configpath = configpath
		self.apppath = apppath
		self.status = status
		self.regfiles = regfiles
		self.desktopnames = desktopnames
		self.iconnames = iconnames
		self.enable = enable
		self.disable = disable
		self.speed = speed

		

	def set_name(self,name):
		self.name = name

	def set_zhname(self,zh_name):
		self.zh_name = zh_name

	def set_version(self,version):
		self.version = version
	
	def set_category(self,category):
		self.category = category

	def set_level(self,level):
		self.level = level

	def set_debpath(self,debpath):
		self.debpath = debpath

	def set_createdesktop(self,createdesktop):
		self.createdesktop = createdesktop

	def set_configpath(self,configpath):
		self.configpath = configpath

	def set_apppath(self,apppath):
		self.apppath = apppath

	def set_status(self,status):
		self.status = status

	def set_regfiles(self,regfiles):
		self.regfiles = regfiles

	def set_desktopnames(self,desktopnames):
		self.desktopnames = desktopnames

	def set_iconnames(self,iconnames):
		self.iconnames = iconnames


	def set_enable(self,enable):
		self.enable = enable

	def set_disable(self,disable):
		self.disable = disable

	def set_speed(self,speed):
		self.speed = speed
