/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "daemonconfig.h"
#include "installedpairs.h"
#include "servicestatus.h"

#include <KQuickConfigModule>

/**
 * The Offline Translation page of System Settings: the settings of the
 * dragomand daemon, its installed language models and its status.
 *
 * The settings live in the daemon, not in a KConfig file, so this module
 * implements load, save and defaults itself on top of DaemonConfig.
 */
class DragomandKcm : public KQuickConfigModule
{
    Q_OBJECT

    Q_PROPERTY(DaemonConfig *config READ config CONSTANT)
    Q_PROPERTY(InstalledPairs *pairs READ pairs CONSTANT)
    Q_PROPERTY(ServiceStatus *status READ status CONSTANT)
    /// Whether the Krakoman application is installed.
    Q_PROPERTY(bool krakomanAvailable READ isKrakomanAvailable CONSTANT)

public:
    DragomandKcm(QObject *parent, const KPluginMetaData &metaData);

    [[nodiscard]] DaemonConfig *config() const
    {
        return m_config;
    }
    [[nodiscard]] InstalledPairs *pairs() const
    {
        return m_pairs;
    }
    [[nodiscard]] ServiceStatus *status() const
    {
        return m_status;
    }
    [[nodiscard]] bool isKrakomanAvailable() const;

    Q_INVOKABLE void openKrakoman();

    void load() override;
    void save() override;
    void defaults() override;

private:
    void syncState();

    DaemonConfig *m_config;
    InstalledPairs *m_pairs;
    ServiceStatus *m_status;
};
