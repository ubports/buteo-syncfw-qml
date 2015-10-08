#!/usr/bin/python3

'''buteo syncfw mock template

This creates the expected methods and properties of the main
com.meego.msyncd object. You can specify D-BUS property values
'''

# This program is free software; you can redistribute it and/or modify it under
# the terms of the GNU Lesser General Public License as published by the Free
# Software Foundation; either version 3 of the License, or (at your option) any
# later version.  See http://www.gnu.org/copyleft/lgpl.html for the full text
# of the license.

__author__ = 'Renato Araujo Oliveira Filho'
__email__ = 'renatofilho@canonical.com'
__copyright__ = '(c) 2015 Canonical Ltd.'
__license__ = 'LGPL 3+'

import dbus
from gi.repository import GObject

import dbus
import dbus.service
import dbus.mainloop.glib

BUS_NAME = 'com.meego.msyncd'
MAIN_OBJ = '/synchronizer'
MAIN_IFACE = 'com.meego.msyncd'
SYSTEM_BUS = False

class ButeoSyncFw(dbus.service.Object):
    DBUS_NAME = None
    PROFILES = [
"""<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<profile type=\"sync\" name=\"test-profile\">
    <key value=\"45\" name=\"accountid\"/>
    <key value=\"buteo-contacts\" name=\"category\"/>
    <key value=\"google.Contacts-\" name=\"displayname\"/>
    <key value=\"true\" name=\"enabled\"/>
    <key value=\"true\" name=\"hidden\"/>
    <key value=\"30\" name=\"sync_since_days_past\"/>
    <key value=\"true\" name=\"use_accounts\"/>
    <profile type=\"client\" name=\"googlecontacts\">
        <key value=\"two-way\" name=\"Sync Direction\"/>
    </profile>
    <schedule time=\"05:00:00\" days=\"4,5,2,3,1,6,7\" syncconfiguredtime=\"\" interval=\"0\" enabled=\"true\">
        <rush end=\"\" externalsync=\"false\" days=\"\" interval=\"15\" begin=\"\" enabled=\"false\"/>
    </schedule>
</profile>""",
"""<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<profile type=\"sync\" name=\"testsync-ovi\">
    <key value=\"calendar\" name=\"category\"/>
    <schedule syncconfiguredtime=\"\" interval=\"0\" days=\"\" externalsync=\"true\" time=\"\" enabled=\"false\">
        <rush interval=\"0\" days=\"\" externalsync=\"false\" begin=\"\" enabled=\"false\" end=\"\"/>
    </schedule>
</profile>""",
"""<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<profile name=\"63807467\" type=\"sync\">
    <key value=\"110\" name=\"accountid\"/>
    <key value=\"buteo-contacts\" name=\"category\"/>
    <key value=\"online\" name=\"destinationtype\"/>
    <key value=\"google-contacts-renato.teste2@gmail.com\" name=\"displayname\"/>
    <key value=\"true\" name=\"enabled\"/>
    <key value=\"true\" name=\"hidden\"/>
    <key value=\"false\" name=\"scheduled\"/>
    <key value=\"true\" name=\"sync_always_up_to_date\"/>
    <key value=\"true\" name=\"sync_on_change\"/>
    <key value=\"30\" name=\"sync_since_days_past\"/>
    <key value=\"true\" name=\"use_accounts\"/>
    <profile name=\"googlecontacts\" type=\"client\">
        <key value=\"two-way\" name=\"Sync Direction\"/>
        <key value=\"gdata\" name=\"Sync Protocol\"/>
        <key value=\"HTTP\" name=\"Sync Transport\"/>
        <key value=\"prefer remote\" name=\"conflictpolicy\"/>
        <key value=\"true\" name=\"sync_on_change\"/>
    </profile>
    <schedule syncconfiguredtime=\"\" days=\"5,4,7,6,1,3,2\" interval=\"60\" enabled=\"false\" time=\"\">
        <rush begin=\"00:00:00\" end=\"00:00:00\" days=\"\" interval=\"60\" enabled=\"false\" externalsync=\"false\"/>
    </schedule>
</profile>""",
"""<?xml version=\"1.0\" encoding=\"UTF-8\"?>
<profile type=\"sync\" name=\"template-profile\">
    <key value=\"buteo-contacts\" name=\"category\"/>
    <key value=\"false\" name=\"enabled\"/>
    <schedule syncconfiguredtime=\"\" interval=\"0\" days=\"\" externalsync=\"true\" time=\"\" enabled=\"false\">
        <rush interval=\"0\" days=\"\" externalsync=\"false\" begin=\"\" enabled=\"false\" end=\"\"/>
    </schedule>
</profile>"""
    ]

    def __init__(self, object_path):
        dbus.service.Object.__init__(self, dbus.SessionBus(), object_path)
        self._mainloop = GObject.MainLoop()
        self._activeSync = []
        self._profiles = ButeoSyncFw.PROFILES

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='', out_signature='as')
    def allVisibleSyncProfiles(self):
        return self._profiles

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='ss', out_signature='as')
    def syncProfilesByKey(self, key, value):
        print ("syncProfilesByKey:", key, value)
        if key == 'category':
            if value == 'buteo-contacts':
                return [ButeoSyncFw.PROFILES[0], ButeoSyncFw.PROFILES[2], ButeoSyncFw.PROFILES[3]]
            if value == 'calendar':
                return [ButeoSyncFw.PROFILES[1]]
        return []

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='s', out_signature='')
    def abortSync(self, profileId):
        # WORKAROUND: using 'quit' as profileId will cause the service to dissapear
        # used on unit test
        if profileId == 'quit':
            self.quit()
            return

        if profileId in self._activeSync:
            self._activeSync.remove(profileId)
            self.syncStatus(profileId, 5, 'aborted by the user', 0)
        return

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='s', out_signature='b')
    def startSync(self, profileId):
        if profileId in ['63807467', 'testsync-ovi', 'test-profile']:
            self._activeSync.append(profileId)
            self.syncStatus(profileId, 1, '', 0)
            return True
        return False

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='', out_signature='as')
    def runningSyncs(self):
        return self._activeSync

    @dbus.service.signal(dbus_interface=MAIN_IFACE,
                         signature='sisi')
    def syncStatus(self, profileId, status, message, statusDetails):
        pass

    @dbus.service.signal(dbus_interface=MAIN_IFACE,
                         signature='sis')
    def signalProfileChanged(self, profileId, status, changedProfile):
        pass

    @dbus.service.method(dbus_interface=MAIN_IFACE,
                         in_signature='s', out_signature='b')
    def removeProfile(self, profileId):
        if int(profileId) < len(self._profiles):
            self._profiles.remove(self._profiles[int(profileId)])
            self.signalProfileChanged(profileId, 2, 'deleted')
            return True
        else:
            return False

    def quit(self):
        ButeoSyncFw.DBUS_NAME = None
        self._mainloop.quit()

    def _run(self):
        self._mainloop.run()

if __name__ == '__main__':
    dbus.mainloop.glib.DBusGMainLoop(set_as_default=True)

    ButeoSyncFw.DBUS_NAME = dbus.service.BusName(BUS_NAME)
    buteo = ButeoSyncFw(MAIN_OBJ)
    buteo._run()
