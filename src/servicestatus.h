/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "dragomanclient.h"

#include <QObject>
#include <QStringList>

/**
 * The daemon's status, fetched on demand only: every call counts as
 * activity and would keep an idle daemon from exiting, so nothing polls.
 */
class ServiceStatus : public QObject
{
    Q_OBJECT

    Q_PROPERTY(bool loading READ isLoading NOTIFY changed)
    /// Whether an answer (or an error) has arrived at least once.
    Q_PROPERTY(bool fetched READ isFetched NOTIFY changed)
    Q_PROPERTY(QString errorText READ errorText NOTIFY changed)
    Q_PROPERTY(QString version READ version NOTIFY changed)
    /// The daemon's resident memory, formatted, or empty when unknown.
    Q_PROPERTY(QString memory READ memory NOTIFY changed)
    /// Loaded routes as "Bulgarian to English".
    Q_PROPERTY(QStringList loaded READ loaded NOTIFY changed)

public:
    explicit ServiceStatus(QObject *parent = nullptr);

    [[nodiscard]] bool isLoading() const
    {
        return m_loading;
    }
    [[nodiscard]] bool isFetched() const
    {
        return m_fetched;
    }
    [[nodiscard]] QString errorText() const
    {
        return m_errorText;
    }
    [[nodiscard]] QString version() const
    {
        return m_status.version;
    }
    [[nodiscard]] QString memory() const;
    [[nodiscard]] QStringList loaded() const;

    Q_INVOKABLE void refresh();

Q_SIGNALS:
    void changed();

private:
    Dragoman::Client m_client;
    Dragoman::DaemonStatus m_status;
    QString m_errorText;
    bool m_loading = false;
    bool m_fetched = false;
};
