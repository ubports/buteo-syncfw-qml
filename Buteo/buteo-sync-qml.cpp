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
#include <QtCore/QTimer>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusReply>
#include <QtXml/QDomDocument>

#define BUTEO_DBUS_SERVICE_NAME   "com.meego.msyncd"
#define BUTEO_DBUS_OBJECT_PATH    "/synchronizer"
#define BUTEO_DBUS_INTERFACE      "com.meego.msyncd"

typedef QPair<QString, bool> ProfileData;

ButeoSyncFW::ButeoSyncFW(QObject *parent)
    : QObject(parent),
      m_waitSyncStart(false)
{
    connect(this, SIGNAL(syncStatus(QString,int,QString,int)), SIGNAL(syncStatusChanged()));
    connect(this, SIGNAL(profileChanged(QString,int,QString)), SIGNAL(profilesChanged()));
}

bool ButeoSyncFW::syncing() const
{
    return (m_waitSyncStart || !getRunningSyncList().isEmpty());
}

QStringList ButeoSyncFW::visibleSyncProfiles() const
{
    return profiles();
}

int ButeoSyncFW::profilesCount() const
{
    return profiles().count();
}

void ButeoSyncFW::classBegin()
{
    m_serviceWatcher.reset(new QDBusServiceWatcher(BUTEO_DBUS_SERVICE_NAME,
                                                   QDBusConnection::sessionBus(),
                                                   QDBusServiceWatcher::WatchForOwnerChange,
                                                   this));
    connect(m_serviceWatcher.data(), SIGNAL(serviceOwnerChanged(QString,QString,QString)),
            this, SLOT(serviceOwnerChanged(QString,QString,QString)));

    initialize();
}

void ButeoSyncFW::componentComplete()
{
}

void ButeoSyncFW::initialize()
{
    if (!m_iface.isNull()) {
        return;
    }

    m_waitSyncStart = false;
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
            SIGNAL(syncStatus(QString, int, QString, int)));
    connect(m_iface.data(),
            SIGNAL(signalProfileChanged(QString, int, QString)),
            SIGNAL(profileChanged(QString, int, QString)), Qt::QueuedConnection);

    // track signals for internal state
    connect(m_iface.data(),
            SIGNAL(signalProfileChanged(QString, int, QString)),
            SLOT(reloadProfiles()));
    connect(m_iface.data(),
            SIGNAL(syncStatus(QString, int, QString, int)),
            SLOT(onSyncStatusChanged()));

    reloadProfiles();

    // notify changes on properties
    emit syncStatusChanged();
}

bool ButeoSyncFW::startSync(const QString &aProfileId)
{
    if (!m_iface) {
        return false;
    }

    QDBusPendingCall pcall = m_iface->asyncCall(QLatin1String("startSync"), aProfileId);
    if (pcall.isError()) {
        qWarning() << "Fail to start sync:" << pcall.error().message();
        return false;
    }

    if (!m_waitSyncStart) {
        m_waitSyncStart = true;
        emit syncStatusChanged();
    }

    return true;
}

bool ButeoSyncFW::startSyncByCategory(const QString &category)
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
        m_iface->asyncCall(QLatin1String("abortSync"), aProfileId);
    }
}

QStringList ButeoSyncFW::getRunningSyncList() const
{
    if (m_iface) {
        QDBusReply<QStringList> syncList = m_iface->call(QLatin1String("runningSyncs"));
        return syncList;
    }
    return QStringList();
}

QStringList ButeoSyncFW::syncProfilesByCategory(const QString &category) const
{
    return profiles(category, true);
}

bool ButeoSyncFW::removeProfile(const QString &profileId) const
{
    if (m_iface) {
        QDBusReply<bool> result = m_iface->call(QLatin1String("removeProfile"), profileId);
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
    m_waitSyncStart = false;
    m_profilesByCategory.clear();
    m_reloadProfilesWatcher.reset();
    m_iface.reset();

    // notify changes on properties
    emit profilesChanged();
    emit syncStatusChanged();
}

QStringList ButeoSyncFW::profiles(const QString &category, bool onlyEnabled) const
{
    QStringList result;
    QList<ProfileData> filter = category.isEmpty() ?
                m_profilesByCategory.values() : m_profilesByCategory.values(category);

    foreach(const ProfileData &p, filter) {
        if (!onlyEnabled || p.second) {
            result << p.first;
        }
    }
    return result;
}


QMultiMap<QString, QPair<QString, bool> >  ButeoSyncFW::paserProfiles(const QStringList &profiles) const
{
    // extract category/ids
    QMultiMap<QString, ProfileData> profilesByCategory;

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
                QString profileCategory;
                for(int i = 0; i < values.count(); i++) {
                    QDomElement v = values.at(i).toElement();
                    if ((v.attribute("name") == "enabled") &&
                        (v.attribute("value") == "false")) {
                        enabled = false;
                    }
                    if (v.attribute("name") == "category") {
                        profileCategory = v.attribute("value");
                    }
                }

                QString profileName = e.attribute("name", "");
                if (profileName.isEmpty() || profileCategory.isEmpty()) {
                    qWarning() << "Fail to retrieve profile name and category";
                } else {
                    profilesByCategory.insertMulti(profileCategory, qMakePair(profileName, enabled));
                }
          } else {
              qWarning() << "Profile not found in:" << profile;
          }
       } else {
           qWarning() << "Fail to parse profile:" << profile;
           qWarning() << "Error:" << errorMsg << errorLine << errorColumn;
       }
    }
    return profilesByCategory;
}

void ButeoSyncFW::reloadProfiles()
{
    if (m_reloadProfilesWatcher) {
        m_reloadProfilesWatcher.reset();
    }

    if (!m_iface) {
        return;
    }

    // we only care about profiles that uses online-accounts
    QDBusPendingCall pcall = m_iface->asyncCall(QLatin1String("syncProfilesByKey"),
                                                QLatin1String("use_accounts"),
                                                QLatin1String("true"));
    if (pcall.isError()) {
        qWarning() << "Fail to call syncProfilesByKey:" << pcall.error().message();
    } else {
        m_reloadProfilesWatcher.reset(new QDBusPendingCallWatcher(pcall, this));
        connect(m_reloadProfilesWatcher.data(), SIGNAL(finished(QDBusPendingCallWatcher*)),
                SLOT(onAllVisibleSyncProfilesFinished(QDBusPendingCallWatcher*)), Qt::UniqueConnection);
    }
}

void ButeoSyncFW::onAllVisibleSyncProfilesFinished(QDBusPendingCallWatcher *watcher)
{
    QStringList profiles;
    QDBusPendingReply<QStringList> reply = *watcher;
    if (reply.isError()) {
       qWarning() << "Fail to call 'syncProfilesByKey'" << reply.error().message();
    } else {
       profiles = reply.value();
    }

    m_profilesByCategory = paserProfiles(profiles);
    m_reloadProfilesWatcher.take()->deleteLater();

    // notify property change
    emit profilesChanged();
}

void ButeoSyncFW::onSyncStatusChanged()
{
    m_waitSyncStart = false;
}
