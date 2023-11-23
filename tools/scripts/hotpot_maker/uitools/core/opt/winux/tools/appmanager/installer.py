#!/usr/bin/env python
# -*- coding: utf-8 -*-

import os,sys,stat,re,math
import subprocess
import thread
import logging
import ConfigParser
from ast import literal_eval





from installeritem import *

APP_MANAGER_DIR = "/opt/winux/tools/appmanager"

os.chdir(APP_MANAGER_DIR)
sys.path.insert(0,APP_MANAGER_DIR)

logger = logging.getLogger(__name__)
logger.setLevel(level = logging.INFO)
# logfilepath = os.path.join(os.getenv("HOME"),"hotpot.log")
#handler = logging.FileHandler(logfilepath)
handler = logging.FileHandler("hotpot.log")
handler.setLevel(logging.INFO)
formatter = logging.Formatter('[%(asctime)s] - %(funcName)s - %(levelname)s - %(lineno)d - %(message)s')
handler.setFormatter(formatter)

console = logging.StreamHandler()
console.setLevel(logging.INFO)

logger.addHandler(handler)
logger.addHandler(console)

class Installer():



	def __init__(self):


		self.special_install_dir = "/opt/winux/hotpot/appstore"
		self.WinuxGEAR_DIR = "/opt/winux/hotpot/appstore"
		self.Start_DIR = "/usr/share/applications"
		self.commonusers = self.getcommonuser()
		self.uninstall = ""
		print self.commonusers

		print "----------------> sys.argv=",sys.argv
		print "sys.argv=%s"%sys.argv
		if len(sys.argv) <3:
			self.help()
			sys.exit(0)
		if len(sys.argv) >2:
			self.appname = sys.argv[1]
			self.appversion = sys.argv[2]

			if len(sys.argv) >3 and sys.argv[3]=="uninstall":
				self.uninstall = sys.argv[3]

	def help(self):
		logger.info("鸳鸯火锅应用软件兼容平台") 


	def getcommonuser(self):
		rfo = open('/etc/passwd','r')
		lines = rfo.readlines()
		rfo.close()
		commonusers = []
		for line in lines:
			words = line.split(":")
			
			uid = words[2]
			shellcmd = words[6]
			
			if int(uid)>=1000 and uid!='65534' and "nologin" not in shellcmd:
				print '---------------->word=',words
				username = words[0]
				commonusers.append(username)

		print '-----------> commonusers=',commonusers
		return commonusers
		

	


	def getappconfiginfo(self):
		config = ConfigParser.ConfigParser()
		appdirname = "%s-%s"%(self.appname,self.appversion)
		appcontainerpath = os.path.join(self.special_install_dir,appdirname)
		appconfigfilename = "winuxapp_%s.config"%self.appname
		appconfigfilepath = os.path.join(appcontainerpath,appconfigfilename)
		
		app = UserLevelInstallerItem()
		app.name = self.appname
		app.version = self.appversion

		config.read(appconfigfilepath)
		app.desktopnames = literal_eval(config.get("config","desktopnames"))
		app.iconnames = literal_eval(config.get("config","iconnames"))
		app.associationnames = literal_eval(config.get("config","associationnames"))

		print "---------------> app.desktopnames=",app.desktopnames
		print "---------------> app.iconnames=",app.iconnames
		print "---------------> app.associationnames=",app.associationnames

		return app






	def special_install_app(self):
		app = self.getappconfiginfo()
		appdirname = "%s-%s"%(self.appname,self.appversion)
		appcontainerpath = os.path.join(self.special_install_dir,appdirname)
		for i in range(0,len(app.desktopnames)):
			#把desktop文件复制到普通用户桌面

			srcpath = os.path.join(appcontainerpath,app.desktopnames[i])
			for commonuser in self.commonusers:
				despath = os.path.join("/home",commonuser,"桌面")
				os.system("cp '%s' '%s'"%(srcpath,despath))
				desktopfilepath = os.path.join(despath,app.desktopnames[i])
				os.system("chown %s:%s %s"%(commonuser,commonuser,desktopfilepath))


		#copy association 
		# if app.associationnames!=['']:
		# 	for i in range(0,len(app.associationnames)):
		# 		fsrcpath = os.path.join(appcontainerpath,app.associationnames[i])

		# 		for commonuser in self.commonusers:
		# 			fdespath = os.path.join("/home",commonuser,".local/share/applications")
					
		# 			# if os.path.exists(fdespath) == False:
		# 				# os.makedirs(fdespath)

		# 			os.system("cp '%s' '%s'"%(fsrcpath,fdespath))
		# 			associationfilepath = os.path.join(fdespath,app.associationnames[i])
		# 			os.system("chown %s:%s %s"%(commonuser,commonuser,associationfilepath))


		# 			#update mimeinfo.cache
		# 			os.system("update-desktop-database '%s'"%fdespath)

		#copy association to /usr/share/applications
		if app.associationnames!=['']:
			for i in range(0,len(app.associationnames)):
				 fsrcpath = os.path.join(appcontainerpath,app.associationnames[i])
				 fdespath = self.Start_DIR

				 os.system("sudo cp '%s' '%s'"%(fsrcpath,fdespath))
				 associationfilepath = os.path.join(fdespath,app.associationnames[i])
				 

				 #update mimeinfo.cache
				 os.system("sudo update-desktop-database '%s'"%fdespath)

		#create Recent Dir
		for commonuser in self.commonusers:
			userpath =  os.path.join(appcontainerpath,"drive_c/users",commonuser)
			userrecentpath = os.path.join(appcontainerpath,"drive_c/users",commonuser,"Recent")
			if os.path.exists(userrecentpath) == False:
				os.makedirs(userrecentpath)

				os.system("chown %s:%s %s"%(commonuser,commonuser,userpath))
				os.system("chown %s:%s %s"%(commonuser,commonuser,userrecentpath))
				os.system("chmod -R 755 '%s'"%userpath)

			# create xdg dir
			xdg_dirs = ["桌面","我的图片","我的视频","我的文档","我的音乐"]
			linux_xdg_dirs = ["桌面","图片","视频","文档","音乐"]
			for i in range(0,len(xdg_dirs)):
				userxdgpath = os.path.join(userpath,xdg_dirs[i])
				if os.path.exists(userxdgpath) == False:
					os.system("ln -s /home/%s/%s %s"%(commonuser,linux_xdg_dirs[i],userxdgpath))
					os.system("chown %s:%s %s"%(commonuser,commonuser,userxdgpath))
					os.system("chmod -R 777 '%s'"%userxdgpath)

			#copy other directory
			srcdirpath = os.path.join(appcontainerpath,"drive_c/users/@iscaser")
			desdirpath = userpath
			allfiles = os.listdir(srcdirpath)
			for f in allfiles:
				fpath = os.path.join(srcdirpath,f)
				if os.path.islink(fpath)== False:
					os.system("cp -r '%s' '%s'"%(fpath,desdirpath))
			os.system("chown -R %s:%s %s"%(commonuser,commonuser,desdirpath))
			os.system("chmod -R 755 '%s'"%desdirpath)

			

		#把桌面快捷方式添加到开始菜单
		for i in range(0,len(app.desktopnames)):
			srcpath = os.path.join(appcontainerpath,app.desktopnames[i])
			despath = self.Start_DIR
			os.system("cp '%s' '%s'"%(srcpath,despath))
			#为desktop文件添加Category字段
			desktopnamepath = os.path.join(despath,app.desktopnames[i])
			fo = open(desktopnamepath,'a+')
			fo.writelines("Categories=Windows;\n")
			fo.close()


						


		#打标签
		# os.system("evmctl dir_ima_hash '%s'"%appcontainerpath)

	def special_uninstall_app(self):

		app = self.getappconfiginfo()
		appdirname = "%s-%s"%(self.appname,self.appversion)
		appcontainerpath = os.path.join(self.special_install_dir,appdirname)

		if os.path.exists(appcontainerpath):
			os.system("rm -rf '%s'"%appcontainerpath)

		#delete desktop
		for i in range(0,len(app.desktopnames)):
			for commonuser in self.commonusers:
				appdesktop = os.path.join("/home",commonuser,"桌面",app.desktopnames[i])
				if os.path.exists(appdesktop):
					os.system("rm -rf '%s'"%appdesktop)

		#delete fileassociation produce dynamicly in ~/.local/share/applications
		if app.associationnames!=['']:
			for i in range(0,len(app.associationnames)):

				for commonuser in self.commonusers:
					fdespath = os.path.join("/home",commonuser,".local/share/applications")
					associationfilepath = os.path.join(fdespath,app.associationnames[i])
					if os.path.exists(associationfilepath):
						os.system("rm -rf '%s'"%associationfilepath)

		#delete fileassociation in /usr/share/applications
		if app.associationnames!=['']:
			for i in range(0,len(app.associationnames)):
				associationfilepath = os.path.join(self.Start_DIR,app.associationnames[i])
				if os.path.exists(associationfilepath):
					os.system("rm -rf '%s'"%associationfilepath)
		


		#delete start menu
		for i in range(0,len(app.desktopnames)):
			srcpath = os.path.join(self.Start_DIR,app.desktopnames[i])
			if os.path.exists(srcpath):
				os.system("sudo rm '%s'"%srcpath)





if __name__ == "__main__":
	main = Installer()
	if main.uninstall == "":

		main.special_install_app()
	else:

		main.special_uninstall_app()
	

