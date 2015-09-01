/*
 * Copyright (C) 2015 Canonical, Ltd.
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

import QtQuick 2.2
import Buteo 0.1
import QtTest 1.0

Item {
    id: root

    property var buteoComponent

    TestCase {
        name: 'ButeoSyncFWTestCase'

        function init()
        {
            buteoComponent = Qt.createQmlObject('import Buteo 0.1; ButeoSync{ }', root);
            // wait profiles to load
            tryCompare(buteoComponent, 'profilesCount', 4)
        }

        function cleanup()
        {
            if (buteoComponent) {
                var activeProfiles = buteoComponent.getRunningSyncList()
                for (var i in activeProfiles) {
                    buteoComponent.abortSync(activeProfiles[i])
                }
                activeProfiles = buteoComponent.getRunningSyncList()
                compare(activeProfiles.length, 0)

                buteoComponent.destroy()
                buteoComponent = null
            }
        }

        function cleanupTestCase()
        {
            // Test service dissapear
            // We can only test it on 'cleanupTestCase' because it will cause the dbus
            // service to dissapear and any test after this call will fail
            buteoComponent = Qt.createQmlObject('import Buteo 0.1; ButeoSync{ }', root);

            var spy = Qt.createQmlObject('import QtTest 1.0; SignalSpy{ }', root);
            spy.target = buteoComponent
            spy.signalName = "profilesChanged"

            buteoComponent.abortSync('quit')

            tryCompare(spy, "count", 1)
            tryCompare(buteoComponent, 'syncing', false)
            tryCompare(buteoComponent, 'visibleSyncProfiles', [])

            buteoComponent.destroy()
            buteoComponent = null
        }

        function test_start_sync()
        {
            var spy = Qt.createQmlObject('import QtTest 1.0; SignalSpy{ }', root);
            spy.target = buteoComponent
            spy.signalName = "syncStatus"

            compare(buteoComponent.startSync('1234'), false)
            compare(buteoComponent.startSync('63807467'), true)

            tryCompare(spy, "count", 1)
            compare(spy.signalArguments[0][0], '63807467')
            compare(spy.signalArguments[0][1], ButeoSync.SyncStarted)
            compare(spy.signalArguments[0][2], '')
        }

        function test_abort_sync()
        {
            var spy = Qt.createQmlObject('import QtTest 1.0; SignalSpy{ }', root);
            spy.target = buteoComponent
            spy.signalName = "syncStatus"

            compare(buteoComponent.startSync('63807467'), true)
            tryCompare(spy, "count", 1)

            var spy2 = Qt.createQmlObject('import QtTest 1.0; SignalSpy{ }', root);
            spy2.target = buteoComponent
            spy2.signalName = "syncStatus"

            buteoComponent.abortSync('63807467')

            tryCompare(spy2, "count", 1)
            compare(spy2.signalArguments[0][0], '63807467')
            compare(spy2.signalArguments[0][1], ButeoSync.SyncAborted)
            compare(spy2.signalArguments[0][2], 'aborted by the user')
        }

        function test_syncing_property()
        {
            // check if no sync is running
            compare(buteoComponent.syncing, false)

            // start a new sync and check if the property changed to true
            buteoComponent.startSync('63807467')
            tryCompare(buteoComponent, 'syncing', true)

            // abort current sync and check if sync property changed to false
            buteoComponent.abortSync('63807467')
            tryCompare(buteoComponent, 'syncing', false)
        }

        function test_visibleSyncProfiles_property()
        {
            tryCompare(buteoComponent, 'profilesCount', 4)
            var spy = Qt.createQmlObject('import QtTest 1.0; SignalSpy{ }', root);
            spy.target = buteoComponent
            spy.signalName = "profileChanged"

            buteoComponent.removeProfile('3')
            tryCompare(spy, "count", 1)
            compare(spy.signalArguments[0][0], '3')
            compare(spy.signalArguments[0][1], ButeoSync.ProfileRemoved)
            compare(spy.signalArguments[0][2], 'deleted')
            tryCompare(buteoComponent, 'profilesCount', 3)
            spy.clear()

            buteoComponent.removeProfile('2')
            tryCompare(spy, "count", 1)
            compare(spy.signalArguments[0][0], '2')
            compare(spy.signalArguments[0][1], ButeoSync.ProfileRemoved)
            compare(spy.signalArguments[0][2], 'deleted')
            tryCompare(buteoComponent, 'profilesCount', 2)
            spy.clear()

            buteoComponent.removeProfile('1')
            tryCompare(spy, "count", 1)
            compare(spy.signalArguments[0][0], '1')
            compare(spy.signalArguments[0][1], ButeoSync.ProfileRemoved)
            compare(spy.signalArguments[0][2], 'deleted')
            tryCompare(buteoComponent, 'profilesCount', 1)
            spy.clear()

            buteoComponent.removeProfile('0')
            tryCompare(spy, "count", 1)
            compare(spy.signalArguments[0][0], '0')
            compare(spy.signalArguments[0][1], ButeoSync.ProfileRemoved)
            compare(spy.signalArguments[0][2], 'deleted')
            tryCompare(buteoComponent, 'profilesCount', 0)
            spy.clear()
        }

        function test_sync_by_profile_by_category()
        {
            var profiles = buteoComponent.syncProfilesByCategory('contacts')
            compare(profiles.length, 2)
            compare(profiles[0], '63807467')
            compare(profiles[1], 'test-profile')
        }

        function test_sync_by_category()
        {
            var spy = Qt.createQmlObject('import QtTest 1.0; SignalSpy{ }', root);
            spy.target = buteoComponent
            spy.signalName = "syncStatus"

            compare(buteoComponent.startSyncByCategory('contacts'), true)

            // wait for two signals (since we have two contacts profiles)
            tryCompare(spy, "count", 2)
            // first profile
            compare(spy.signalArguments[0][0], '63807467')
            compare(spy.signalArguments[0][1], ButeoSync.SyncStarted)
            compare(spy.signalArguments[0][2], '')

            // secound profile
            compare(spy.signalArguments[1][0], 'test-profile')
            compare(spy.signalArguments[1][1], ButeoSync.SyncStarted)
            compare(spy.signalArguments[1][2], '')

            var activeProfiles = buteoComponent.getRunningSyncList()
            compare(activeProfiles.length, 2)
            compare(activeProfiles[0], '63807467')
            compare(activeProfiles[1], 'test-profile')
        }
    }
}
