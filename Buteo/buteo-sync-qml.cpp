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
    : QObject(parent),
      m_busy(false)
{
    connect(this, SIGNAL(syncStatus(QString,int,QString,int)), SIGNAL(syncStatusChanged()));
}

bool ButeoSyncFW::syncing() const
{
    return m_busy || !(getRunningSyncList().isEmpty());
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
    m_serviceWatcher.reset(new QDBusServiceWatcher(BUTEO_DBUS_SERVICE_NAME,
                                                   QDBusConnection::sessionBus(),
                                                   QDBusServiceWatcher::WatchForOwnerChange,
                                                   this));
    connect(m_serviceWatcher.data(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));

    initialize();
}

void ButeoSyncFW::initialize()
{
    if (!m_iface.isNull()) {
        return;
    }

    m_iface.reset(new QDBusInterface(BUTEO_DBUS_SERVICE_NAME,
                                     BUTEO_DBUS_OBJECT_PATH,
                                     BUTEO_DBUS_INTERFACE));

    if (!m_iface->isValid()) {
        m_iface.reset();
        qWarning() << "Fail to connect with syncfw";
        return;
    }

    connect(m_iface.data(),
            SIGNAL(syncStatus(QString, int, QString, int)),
            SIGNAL(syncStatus(QString, int, QString, int)), Qt::QueuedConnection);
    connect(m_iface.data(),
            SIGNAL(signalProfileChanged(QString, int, QString)),
            SIGNAL(profileChanged(QString, int, QString)), Qt::QueuedConnection);

    // notify changes on properties
    emit syncStatusChanged();
    emit profileChanged("", 0, "");
}

bool ButeoSyncFW::startSync(const QString &aProfileId) const
{
    if (m_iface) {
        QDBusReply<bool> reply = m_iface->call(QLatin1String("startSync"), aProfileId);
        return reply.value();
    }
    return false;
}

bool ButeoSyncFW::startSyncByCategory(const QString &category)
{

    QDBusPendingCall pcall = m_iface->asyncCall("syncProfilesByKey", "category", category);
    if (pcall.isError()) {
        qWarning() << "Fail to call syncProfilesByKey:" << pcall.error().message();
        return false;
    } else {
        m_busy = true;
        emit syncStatusChanged();

        QDBusPendingCallWatcher *watcher = new QDBusPendingCallWatcher(pcall, this);
        connect(watcher, SIGNAL(finished(QDBusPendingCallWatcher*)),
                this, SLOT(onSyncProfilesByKeyFinished(QDBusPendingCallWatcher*)));
    }
    return true;
}

void ButeoSyncFW::onSyncProfilesByKeyFinished(QDBusPendingCallWatcher *watcher)
{
    QStringList profiles;
    QDBusPendingReply<QStringList> reply = *watcher;
    if (reply.isError()) {
       qWarning() << "Fail to call 'syncProfilesByKey'" << reply.error().message();
    } else {
       profiles = reply.value();
    }

    watcher->deleteLater();

    QStringList ids = idsFromProfileList(profiles);
    foreach(const QString &profile, ids) {
        startSync(profile);
    }
    m_busy = false;
    emit syncStatusChanged();
}

void ButeoSyncFW::abortSync(const QString &aProfileId) const
{
    if (m_iface) {
        m_iface->asyncCall(QLatin1String("abortSync"), aProfileId);
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
         return idsFromProfileList(profiles.value());
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

void ButeoSyncFW::serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner)
{
    Q_UNUSED(oldOwner);

    if (name == BUTEO_DBUS_SERVICE_NAME) {
        if (!newOwner.isEmpty()) {
            // service appear
            initialize();
        } else if (!m_iface.isNull()) {
            // lost service
            deinitialize();
        }
    }
}

void ButeoSyncFW::deinitialize()
{
    m_iface.reset();
    // notify changes on properties
    emit syncStatus("", 0, "", 0);
    emit profileChanged("", 0, "");
}

QStringList ButeoSyncFW::idsFromProfileList(const QStringList &profiles) const
{
    // extract ids
    QStringList ids;
    foreach(const QString &profile, profiles) {
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
