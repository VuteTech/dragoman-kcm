/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "dragomanclient.h"

#include <QAbstractListModel>
#include <QHash>
#include <QPointer>

/**
 * The installed language pairs, with removal, the update check and
 * updating. ListLanguagePairs reads the daemon's cached records only, so
 * refreshing never touches the network; checkForUpdates() does.
 */
class InstalledPairs : public QAbstractListModel
{
    Q_OBJECT

    Q_PROPERTY(bool loading READ isLoading NOTIFY loadingChanged)
    Q_PROPERTY(bool loaded READ isLoaded NOTIFY loadingChanged)
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(int count READ count NOTIFY countsChanged)
    Q_PROPERTY(int updateCount READ updateCount NOTIFY countsChanged)
    /// The disk space of every installed pair, formatted.
    Q_PROPERTY(QString totalSizeText READ totalSizeText NOTIFY countsChanged)
    Q_PROPERTY(bool checkingForUpdates READ isCheckingForUpdates NOTIFY busyChanged)
    /// Whether any pair is being updated.
    Q_PROPERTY(bool updating READ isUpdating NOTIFY busyChanged)

public:
    enum Roles {
        SourceRole = Qt::UserRole + 1,
        TargetRole,
        TitleRole,
        VersionRole,
        AvailableVersionRole,
        UpdateAvailableRole,
        SizeTextRole,
        /// "system" or "user"
        OriginRole,
        RemovableRole,
        /// "Very good", ..., or empty when unknown.
        QualityTextRole,
        /// The model is not one of Mozilla's releases.
        ExperimentalRole,
        BusyRole,
        ProgressRole,
        StageRole,
    };
    Q_ENUM(Roles)

    explicit InstalledPairs(QObject *parent = nullptr);

    [[nodiscard]] int rowCount(const QModelIndex &parent = {}) const override;
    [[nodiscard]] QVariant data(const QModelIndex &index, int role) const override;
    [[nodiscard]] QHash<int, QByteArray> roleNames() const override;

    [[nodiscard]] bool isLoading() const
    {
        return m_loading;
    }
    [[nodiscard]] bool isLoaded() const
    {
        return m_loaded;
    }
    [[nodiscard]] QString errorText() const
    {
        return m_errorText;
    }
    [[nodiscard]] int count() const
    {
        return int(m_pairs.size());
    }
    [[nodiscard]] int updateCount() const;
    [[nodiscard]] QString totalSizeText() const;
    [[nodiscard]] bool isCheckingForUpdates() const
    {
        return m_updateCheck != nullptr;
    }
    [[nodiscard]] bool isUpdating() const;

    /// A label for a COMET-22 score, or empty for an unknown (negative) one.
    [[nodiscard]] static QString qualityText(double quality);
    /// Whether Mozilla's release status marks a model as not released.
    [[nodiscard]] static bool isExperimental(const QString &releaseStatus);

    /// Reloads the list from the daemon's cached records (no network).
    Q_INVOKABLE void refresh();
    Q_INVOKABLE void remove(const QString &source, const QString &target);
    /// Asks the provider for newer versions (network).
    Q_INVOKABLE void checkForUpdates();
    /// Installs the newer version of every pair that has one (network).
    Q_INVOKABLE void updateAll();

Q_SIGNALS:
    void loadingChanged();
    void errorTextChanged();
    void countsChanged();
    void busyChanged();
    /// A user-visible message about a finished operation.
    void notify(const QString &message, bool failure);

private:
    struct Operation {
        QPointer<Dragoman::Job> job;
        double progress = -1;
        QString stage;
    };

    void setPairs(const QList<Dragoman::PairInfo> &pairs);
    void update(const QString &source, const QString &target);
    [[nodiscard]] int rowOf(const QString &source, const QString &target) const;
    void emitRowChanged(const QString &source, const QString &target);
    void setErrorText(const QString &text);
    [[nodiscard]] QString title(const QString &source, const QString &target) const;
    [[nodiscard]] static QString key(const QString &source, const QString &target);

    Dragoman::Client m_client;
    QList<Dragoman::PairInfo> m_pairs;
    QHash<QString, Operation> m_operations;
    QPointer<Dragoman::Job> m_updateCheck;
    QString m_errorText;
    bool m_loading = false;
    bool m_loaded = false;
    bool m_refreshAgain = false;
};
