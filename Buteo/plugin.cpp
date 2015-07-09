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

#include "plugin.h"
#include "buteo-sync-qml.h"

#include <QtQml/qqml.h>


void ButeoSyncQmlPlugin::registerTypes(const char *uri)
{
    // @uri Buteo.ButeoSync
    qmlRegisterType<ButeoSyncFW>(uri, 0, 1, "ButeoSync");
}
