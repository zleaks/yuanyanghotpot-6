#!/usr/bin/python

import dbus
import os
import sys

#argv[1]: application
#argv[2]: flag
def getRegisterStatus(application):
    try:
        system_bus = dbus.SystemBus()
        registerObj = system_bus.get_object('com.nfs.register', '/')
        iface = dbus.Interface(registerObj, dbus_interface='com.nfs.register')
        registerInfo = iface.get_dbus_method("getRegisterStatus", dbus_interface=None)
        application = str(application)
        return registerInfo(application, '')
    except Exception as e:
        print("getRegisterStatus:" + str(e))
        return None

if len(sys.argv) > 1:
    registerResult = getRegisterStatus(sys.argv[1])
    if registerResult == None:
        print("Get registration information error")
    elif registerResult[0] == 1:
        print("registed")
    else:
        print("The system has not been registered yet.\nPlease complete registration first.")
        if sys.argv[1] == "yuanyanghotpot" and len(sys.argv) == 2:
            os.system("/usr/bin/reg_common.pyc")