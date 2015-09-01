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

#include <QtCore/QObject>
#include <QtQml/QQmlParserStatus>
#include <QtDBus/QDBusInterface>
#include <QtDBus/QDBusServiceWatcher>
#include <QtDBus/QDBusPendingCallWatcher>

class ButeoSyncFW : public QObject, public QQmlParserStatus
{
    Q_OBJECT
    Q_INTERFACES(QQmlParserStatus)

    Q_PROPERTY(bool syncing READ syncing NOTIFY syncStatusChanged)
    Q_PROPERTY(int profilesCount READ profilesCount NOTIFY profilesChanged)
    Q_PROPERTY(QStringList visibleSyncProfiles READ visibleSyncProfiles NOTIFY profilesChanged)

public:
    ButeoSyncFW(QObject *parent = 0);

    bool syncing() const;
    QStringList visibleSyncProfiles() const;
    int profilesCount() const;

    // QQmlParserStatus
    void classBegin();
    void componentComplete();

    //! \brief  Enum to indicate the change type of the Profile Operation
    enum ProfileChangeType
    {
        //! a New Profile has been added
        ProfileAdded = 0,
        //! a Existing Profile has been modified
        ProfileModified,
        //! Profile has been Removed
        ProfileRemoved,
        //! Profile log file Modified.
        ProfileLogsModified
    };
    Q_ENUMS(ProfileChangeType)

    //! \brief  Enum to indicate the change type of the Profile Operation
    enum SyncStatus
    {
        //! Sync request has been queued
        SyncQueued = 0,
        //! Sync session has been started
        SyncStarted,
        //! Sync session is progressing
        SyncProgress,
        //! Sync session has encountered an error
        SyncError,
        //! Sync session was successfully completed
        SyncDone,
        //! Sync session was aborted
        SyncAborted
    };
    Q_ENUMS(SyncStatus)

signals:
    /*! \brief Notifies about a change in profile.
     *
     * This signal is sent when the profile data is modified or when a profile
     * is added or deleted in msyncd.
     * \param aProfileId Id of the changed profile.
     * \param aChangeType
     *      0 (ADDITION): Profile was added.
     *      1 (MODIFICATION): Profile was modified.
     *      2 (DELETION): Profile was deleted.
     * \param aChangedProfile changed sync profie as XMl string.
     *
     */
    void profileChanged(QString aProfileId,int aChangeType, QString aChangedProfile);

    /*!
     * \brief Notifies about a change in synchronization status.
     *
     * \param aProfileId Id of the profile used in the sync session whose
     *  status has changed.
     * \param aStatus The new status. One of the following:
     *      0 (QUEUED): Sync request has been queued or was already in the
     *          queue when sync start was requested.
     *      1 (STARTED): Sync session has been started.
     *      2 (PROGRESS): Sync session is progressing.
     *      3 (ERROR): Sync session has encountered an error and has been stopped,
     *          or the session could not be started at all.
     *      4 (DONE): Sync session was successfully completed.
     *      5 (ABORTED): Sync session was aborted.
     *  Statuses 3-5 are final, no more status changes will be sent from the
     *  same sync session.
     * \param aMessage A message describing the status change in detail. This
     *  can for example be shown to the user or written to a log
         * \param aStatusDetails
     *  When aStatus is ERROR, this parameter contains a specific error code.
     *  When aStatus is PROGRESS, this parameter contains more details about the progress
     *  \see SyncCommonDefs::SyncProgressDetails
     */
    void syncStatus(QString aProfileId, int aStatus,
                    QString aMessage, int aStatusDetails);

    /*!
     * syncStatus notify signal
     */
    void syncStatusChanged();
    void profilesChanged();

public slots:
    /*!
     * \brief Requests to starts synchronizing using a profile Id
     *
     * A status change signal (QUEUED, STARTED or ERROR) will be sent by the
     * daemon when the request is processed. If there is a sync already in
     * progress using the same resources that are needed by the given profile,
     * adds the sync request to a sync queue. Otherwise a sync session is
     * started immediately.
     *
     * \param aProfileId Id of the profile to use in sync.
     * \return True if a profile with the Id was found. Otherwise
     *  false and no status change signals will follow from this request.
     */
    bool startSync(const QString &aProfileId);

    /*!
     * \brief Requests to starts synchronizing using a profile category
     *
     * \param category Category name of the profile.
     * \return True if a profile with the Id was found. Otherwise
     *  false and no status change signals will follow from this request.
     *
     * \see ButeoSyncFW::startSync
     */
    bool startSyncByCategory(const QString &category);

    /*!
     * \brief Stops synchronizing the profile with the given Id.
     *
     * If the sync request is still in queue and not yet started, the queue
     * entry is removed.
     *
     * \param aProfileId Id of the profile to stop syncing.
     */
    void abortSync(const QString &aProfileId) const;

    /*!
     * \brief Gets the list of profile names of currently running syncs.
     *
     * \return Profile name list.
     */
    QStringList getRunningSyncList() const;

    /*! \brief Gets enabled profiles  matching the profile category.
     *
     * \param category Category name of the profile.
     * \return The sync profile ids as string list.
     */
    QStringList syncProfilesByCategory(const QString &category) const;

    /*!
     * \brief This function should be called when sync profile has to be deleted
     *
     * \param aProfileId Id of the profile to be deleted.
     * \return status of the remove operation
     */
    bool removeProfile(const QString &profileId) const;

private slots:
    void serviceOwnerChanged(const QString &name, const QString &oldOwner, const QString &newOwner);
    void onSyncProfilesByKeyFinished(QDBusPendingCallWatcher *watcher);
    void onAllVisibleSyncProfilesFinished(QDBusPendingCallWatcher *watcher);
    void onSyncStatusChanged();
    void reloadProfiles();

private:
    QScopedPointer<QDBusInterface> m_iface;
    QScopedPointer<QDBusServiceWatcher> m_serviceWatcher;
    QScopedPointer<QDBusPendingCallWatcher> m_reloadProfilesWatcher;
    QMultiMap<QString, QPair<QString, bool> > m_profilesByCategory;
    bool m_waitSyncStart;

    void initialize();
    void deinitialize();
    QStringList profiles(const QString &category = QString(), bool onlyEnabled=false) const;
    QMultiMap<QString, QPair<QString, bool> > paserProfiles(const QStringList &profiles) const;
};
