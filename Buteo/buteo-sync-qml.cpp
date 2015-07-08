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

#include "buteo-sync-qml.h"

#include <QtCore/QDebug>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtXml/QDomDocument>

#define BUTEO_DBUS_SERVICE_NAME   "com.meego.msyncd"
#define BUTEO_DBUS_OBJECT_PATH    "/synchronizer"
#define BUTEO_DBUS_INTERFACE      "com.meego.msyncd"

ButeoSyncFW::ButeoSyncFW(QObject *parent)
    : QObject(parent)
{
}

bool ButeoSyncFW::syncing() const
{
    return !(getRunningSyncList().isEmpty());
}

QStringList ButeoSyncFW::visibleSyncProfiles() const
{
    if (m_iface) {
        QDBusReply<QStringList> result = m_iface->call("allVisibleSyncProfiles");
        return result.value();
    }
    return QStringList();
}

void ButeoSyncFW::classBegin()
{
}

void ButeoSyncFW::componentComplete()
{
    m_iface.reset(new QDBusInterface(BUTEO_DBUS_SERVICE_NAME,
                                     BUTEO_DBUS_OBJECT_PATH,
                                     BUTEO_DBUS_INTERFACE));
    if (m_iface->lastError().isValid()) {
        qWarning() << "Fail to connect with sync monitor:" << m_iface->lastError();
        return;
    }

    connect(m_iface.data(),
            SIGNAL(syncStatus(QString, int, QString, int)),
            SIGNAL(syncStatus(QString, int, QString, int)), Qt::QueuedConnection);
    connect(m_iface.data(),
            SIGNAL(signalProfileChanged(QString, int, QString)),
            SIGNAL(profileChanged(QString, int, QString)), Qt::QueuedConnection);
}

bool ButeoSyncFW::startSync(const QString &aProfileId) const
{
    if (m_iface) {
        QDBusReply<bool> result = m_iface->call("startSync", aProfileId);
        return result.value();
    }
    return false;
}

bool ButeoSyncFW::startSyncByCategory(const QString &category) const
{
    foreach(const QString &profile, syncProfilesByCategory(category)) {
        if (!startSync(profile)) {
            return false;
        }
    }
    return true;
}

void ButeoSyncFW::abortSync(const QString &aProfileId) const
{
    if (m_iface) {
        m_iface->call("abortSync", aProfileId);
    }
}

QStringList ButeoSyncFW::getRunningSyncList() const
{
    if (m_iface) {
        QDBusReply<QStringList> syncList = m_iface->call("runningSyncs");
        return syncList;
    }
    return QStringList();
}

QStringList ButeoSyncFW::syncProfilesByCategory(const QString &category) const
{
    if (m_iface) {
         QDBusReply<QStringList> profiles = m_iface->call("syncProfilesByKey", "category", category);
         // extract ids
         QStringList ids;
         foreach(const QString &profile, profiles.value()) {
            QDomDocument doc;
            QString errorMsg;
            int errorLine;
            int errorColumn;
            if (doc.setContent(profile, &errorMsg, &errorLine, &errorColumn)) {
               QDomNodeList profileElements = doc.elementsByTagName("profile");
               if (!profileElements.isEmpty()) {
                     //check if is enabled
                     QDomElement e = profileElements.item(0).toElement();
                     QDomNodeList values = e.elementsByTagName("key");
                     bool enabled = true;
                     for(int i = 0; i < values.count(); i++) {
                         QDomElement v = values.at(i).toElement();
                         if ((v.attribute("name") == "enabled") &&
                             (v.attribute("value") == "false")) {
                             enabled = false;
                             continue;
                         }
                     }
                     if (!enabled) {
                         continue;
                     }
                     QString profileName = e.attribute("name", "");
                     if (!profileName.isEmpty()) {
                        ids << profileName;
                     } else {
                         qWarning() << "Profile name is empty in:" << profile;
                     }
               } else {
                   qWarning() << "Profile not found in:" << profile;
               }
            } else {
                qWarning() << "Fail to parse profile:" << profile;
                qWarning() << "Error:" << errorMsg << errorLine << errorColumn;
            }
         }
         return ids;
    }
    return QStringList();
}

bool ButeoSyncFW::removeProfile(const QString &profileId) const
{
    if (m_iface) {
        QDBusReply<bool> result = m_iface->call("removeProfile", profileId);
        return result;
    }
    return false;
}
