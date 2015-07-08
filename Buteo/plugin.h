/*
 * Copyright 2015 Canonical Ltd.
 *
 * This file is part of buteo-sync-qml-plugin.
 *
 * sync-monitor is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 3.
 *
 * contact-service-app is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#ifndef BUTE_SYNC_FW_QML_PLUGIN_H
#define BUTE_SYNC_FW_QML_PLUGIN_H

#include <QQmlExtensionPlugin>

class ButeoSyncQmlPlugin : public QQmlExtensionPlugin
{
    Q_OBJECT
    Q_PLUGIN_METADATA(IID "org.qt-project.Qt.QQmlExtensionInterface")

public:
    void registerTypes(const char *uri);
};

#endif //BUTE_SYNC_FW_QML_PLUGIN_H
