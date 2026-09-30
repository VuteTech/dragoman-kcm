/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "daemonconfig.h"

#include <KLocalizedString>

#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusPendingCallWatcher>
#include <QDBusPendingReply>

#include <algorithm>
#include <cmath>

using namespace Qt::StringLiterals;

namespace
{

constexpr auto memoryBudgetKey = "memory_budget_mb"_L1;
constexpr auto keepWarmKey = "keep_warm"_L1;
constexpr auto keepWarmSecondsKey = "keep_warm_seconds"_L1;
constexpr auto idleExitSecondsKey = "idle_exit_seconds"_L1;
constexpr auto networkKey = "network"_L1;
constexpr auto allowPrereleaseKey = "allow_prerelease"_L1;

/// @p key of @p map as a number, or @p fallback when absent or not a number.
qint64 number(const QVariantMap &map, QLatin1StringView key, qint64 fallback)
{
    bool ok = false;
    const qint64 value = map.value(key).toLongLong(&ok);
    return ok ? value : fallback;
}

} // namespace

DaemonConfig::Values DaemonConfig::Values::fromMap(const QVariantMap &map)
{
    const Values defaults;
    Values values;
    values.memoryBudgetMb = number(map, memoryBudgetKey, defaults.memoryBudgetMb);
    values.keepWarm = uint(std::max<qint64>(0, number(map, keepWarmKey, defaults.keepWarm)));
    values.keepWarmSeconds = number(map, keepWarmSecondsKey, defaults.keepWarmSeconds);
    values.idleExitSeconds = number(map, idleExitSecondsKey, defaults.idleExitSeconds);
    values.network = map.value(networkKey, defaults.network).toBool();
    values.allowPrerelease = map.value(allowPrereleaseKey, defaults.allowPrerelease).toBool();
    return values;
}

QVariantMap DaemonConfig::Values::changesFrom(const Values &other) const
{
    // The daemon's own types: t for sizes and durations, u for counts.
    QVariantMap changes;
    if (memoryBudgetMb != other.memoryBudgetMb) {
        changes.insert(memoryBudgetKey, QVariant::fromValue(qulonglong(memoryBudgetMb)));
    }
    if (keepWarm != other.keepWarm) {
        changes.insert(keepWarmKey, QVariant::fromValue(keepWarm));
    }
    if (keepWarmSeconds != other.keepWarmSeconds) {
        changes.insert(keepWarmSecondsKey, QVariant::fromValue(qulonglong(keepWarmSeconds)));
    }
    if (idleExitSeconds != other.idleExitSeconds) {
        changes.insert(idleExitSecondsKey, QVariant::fromValue(qulonglong(idleExitSeconds)));
    }
    if (network != other.network) {
        changes.insert(networkKey, network);
    }
    if (allowPrerelease != other.allowPrerelease) {
        changes.insert(allowPrereleaseKey, allowPrerelease);
    }
    return changes;
}

DaemonConfig::DaemonConfig(QObject *parent)
    : QObject(parent)
{
    connect(&m_client, &Dragoman::Client::configChanged, this, [this](const QVariantMap &config) {
        if (m_state == State::Ready) {
            rebase(Values::fromMap(config), false);
        }
    });
}

bool DaemonConfig::needsSave() const
{
    return m_state == State::Ready && m_edited != m_baseline;
}

bool DaemonConfig::representsDefaults() const
{
    return m_state != State::Ready || m_edited == Values{};
}

void DaemonConfig::setMemoryBudgetMb(int value)
{
    mutate([this, value] {
        m_edited.memoryBudgetMb = std::clamp(value, s_minMemoryBudgetMb, s_maxMemoryBudgetMb);
    });
}

void DaemonConfig::setKeepWarm(int value)
{
    mutate([this, value] {
        m_edited.keepWarm = uint(std::clamp(value, 0, s_maxKeepWarm));
    });
}

void DaemonConfig::setKeepWarmSeconds(int value)
{
    mutate([this, value] {
        m_edited.keepWarmSeconds = std::clamp(value, 0, s_maxKeepWarmSeconds);
    });
}

int DaemonConfig::keepWarmMinutes() const
{
    return int(std::lround(double(m_edited.keepWarmSeconds) / 60));
}

void DaemonConfig::setKeepWarmMinutes(int value)
{
    // Only a real change: 90 seconds shows as 2 minutes and stays 90
    // until the minutes are edited.
    if (value != keepWarmMinutes()) {
        setKeepWarmSeconds(std::clamp(value, 0, s_maxKeepWarmSeconds / 60) * 60);
    }
}

void DaemonConfig::setIdleExitSeconds(int value)
{
    mutate([this, value] {
        m_edited.idleExitSeconds = std::clamp(value, s_minIdleExitSeconds, s_maxIdleExitSeconds);
    });
}

void DaemonConfig::setNetwork(bool value)
{
    mutate([this, value] {
        m_edited.network = value;
    });
}

void DaemonConfig::setAllowPrerelease(bool value)
{
    mutate([this, value] {
        m_edited.allowPrerelease = value;
    });
}

void DaemonConfig::load()
{
    const int generation = ++m_generation;
    setState(State::Loading);
    m_client.config([this, generation](const QVariantMap &config, const QString &error) {
        if (generation != m_generation) {
            return;
        }
        if (!error.isEmpty()) {
            failedToLoad(error);
            return;
        }
        setErrorText({});
        rebase(Values::fromMap(config), true);
        setState(State::Ready);
    });
}

void DaemonConfig::failedToLoad(const QString &message)
{
    // Client::config() reports only the message. Whether the daemon is
    // missing or merely too old to have GetConfig shows in the D-Bus error
    // name, so ask once more without the library.
    QDBusMessage call = QDBusMessage::createMethodCall(u"dev.l10n_bg.dragomand.Translator1"_s,
                                                       u"/dev/l10n_bg/dragomand/Translator1"_s,
                                                       u"dev.l10n_bg.dragomand.Translator1"_s,
                                                       u"GetConfig"_s);
    auto *watcher = new QDBusPendingCallWatcher(QDBusConnection::sessionBus().asyncCall(call), this);
    const int generation = m_generation;
    connect(watcher, &QDBusPendingCallWatcher::finished, this, [this, generation, message](QDBusPendingCallWatcher *watcher) {
        watcher->deleteLater();
        if (generation != m_generation) {
            return;
        }
        const QDBusPendingReply<QVariantMap> reply = *watcher;
        if (!reply.isError()) {
            // The daemon has started in the meantime.
            setErrorText({});
            rebase(Values::fromMap(reply.value()), true);
            setState(State::Ready);
        } else if (reply.error().name() == "org.freedesktop.DBus.Error.UnknownMethod"_L1) {
            setErrorText(i18nc("@info", "This version of Dragomand cannot be configured here. Update Dragomand to 0.2 or newer."));
            setState(State::Unsupported);
        } else {
            setErrorText(i18nc("@info %1 is an error message",
                               "The Dragomand translation service is not available, so its settings cannot be shown. Is Dragomand installed?\n%1",
                               message));
            setState(State::Unavailable);
        }
    });
}

void DaemonConfig::save()
{
    if (m_state != State::Ready || m_saving) {
        return;
    }
    const QVariantMap changes = m_edited.changesFrom(m_baseline);
    if (changes.isEmpty()) {
        Q_EMIT saved();
        return;
    }
    // Taken as saved at once, so the page is clean while the call runs; a
    // refusal puts the previous values back.
    const Values previous = m_baseline;
    mutate([this] {
        m_baseline = m_edited;
    });
    m_saving = true;
    Q_EMIT savingChanged();
    m_client.setConfig(changes, [this, previous](const QString &error) {
        m_saving = false;
        Q_EMIT savingChanged();
        if (error.isEmpty()) {
            setErrorText({});
            Q_EMIT saved();
            return;
        }
        mutate([this, previous] {
            m_baseline = previous;
        });
        const QString message = i18nc("@info %1 is an error message", "The settings could not be saved: %1", error);
        setErrorText(message);
        Q_EMIT saveFailed(message);
    });
}

void DaemonConfig::setDefaults()
{
    if (m_state != State::Ready) {
        return;
    }
    mutate([this] {
        m_edited = Values{};
    });
}

void DaemonConfig::setState(State state)
{
    if (m_state == state) {
        return;
    }
    mutate([this, state] {
        m_state = state;
    });
    Q_EMIT stateChanged();
}

void DaemonConfig::setErrorText(const QString &text)
{
    if (m_errorText != text) {
        m_errorText = text;
        Q_EMIT errorTextChanged();
    }
}

void DaemonConfig::mutate(const std::function<void()> &change)
{
    const bool neededSave = needsSave();
    const bool wasDefaults = representsDefaults();
    const Values before = m_edited;
    change();
    if (m_edited != before) {
        Q_EMIT valuesChanged();
    }
    if (needsSave() != neededSave) {
        Q_EMIT needsSaveChanged();
    }
    if (representsDefaults() != wasDefaults) {
        Q_EMIT representsDefaultsChanged();
    }
}

void DaemonConfig::rebase(const Values &values, bool dropEdits)
{
    mutate([this, &values, dropEdits] {
        if (dropEdits) {
            m_edited = values;
        } else {
            const auto follow = [this, &values](auto Values::*field) {
                if (m_edited.*field == m_baseline.*field) {
                    m_edited.*field = values.*field;
                }
            };
            follow(&Values::memoryBudgetMb);
            follow(&Values::keepWarm);
            follow(&Values::keepWarmSeconds);
            follow(&Values::idleExitSeconds);
            follow(&Values::network);
            follow(&Values::allowPrerelease);
        }
        m_baseline = values;
    });
}
