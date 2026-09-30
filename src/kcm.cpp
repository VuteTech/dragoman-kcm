/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "kcm.h"

#include <KIO/ApplicationLauncherJob>
#include <KPluginFactory>
#include <KService>

#include <QQmlEngine>

using namespace Qt::StringLiterals;

K_PLUGIN_CLASS_WITH_JSON(DragomandKcm, "kcm_dragomand.json")

namespace
{
constexpr auto krakomanDesktopName = "dev.l10n_bg.krakoman"_L1;
}

DragomandKcm::DragomandKcm(QObject *parent, const KPluginMetaData &metaData)
    : KQuickConfigModule(parent, metaData)
    , m_config(new DaemonConfig(this))
    , m_pairs(new InstalledPairs(this))
    , m_status(new ServiceStatus(this))
{
    // Typed properties and enums for the page.
    constexpr auto uri = "dev.l10n_bg.dragomand.kcm";
    qmlRegisterUncreatableType<DaemonConfig>(uri, 1, 0, "DaemonConfig", u"provided by the module"_s);
    qmlRegisterUncreatableType<InstalledPairs>(uri, 1, 0, "InstalledPairs", u"provided by the module"_s);
    qmlRegisterUncreatableType<ServiceStatus>(uri, 1, 0, "ServiceStatus", u"provided by the module"_s);

    setButtons(Apply | Default);
    // The base class resets these in load() and save(); the daemon's
    // answers arrive later, and a refused save leaves the edits unsaved.
    connect(m_config, &DaemonConfig::needsSaveChanged, this, &DragomandKcm::syncState);
    connect(m_config, &DaemonConfig::representsDefaultsChanged, this, &DragomandKcm::syncState);
    connect(m_config, &DaemonConfig::saveFailed, this, &DragomandKcm::syncState);
}

bool DragomandKcm::isKrakomanAvailable() const
{
    return KService::serviceByDesktopName(krakomanDesktopName) != nullptr;
}

void DragomandKcm::openKrakoman()
{
    if (const KService::Ptr service = KService::serviceByDesktopName(krakomanDesktopName)) {
        auto *job = new KIO::ApplicationLauncherJob(service);
        job->start();
    }
}

void DragomandKcm::load()
{
    KQuickConfigModule::load();
    m_config->load();
    m_pairs->refresh();
    syncState();
}

void DragomandKcm::save()
{
    KQuickConfigModule::save();
    m_config->save();
    syncState();
}

void DragomandKcm::defaults()
{
    KQuickConfigModule::defaults();
    m_config->setDefaults();
    syncState();
}

void DragomandKcm::syncState()
{
    setNeedsSave(m_config->needsSave());
    setRepresentsDefaults(m_config->representsDefaults());
}

#include "kcm.moc"
