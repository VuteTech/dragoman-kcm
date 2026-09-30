/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#pragma once

#include "dragomanclient.h"

#include <QObject>
#include <QVariantMap>

#include <functional>

/**
 * The daemon's settings (GetConfig and SetConfig), with the edits of the
 * page buffered until they are saved.
 *
 * The values the daemon reported are the baseline: needsSave() tells
 * whether an edit differs from it, and save() sends only the keys that do.
 * When another program changes the settings (ConfigChanged), every value
 * the page has not edited follows the daemon, and edited ones are kept.
 */
class DaemonConfig : public QObject
{
    Q_OBJECT

    Q_PROPERTY(State state READ state NOTIFY stateChanged)
    /// Why the settings cannot be shown or saved, or empty.
    Q_PROPERTY(QString errorText READ errorText NOTIFY errorTextChanged)
    Q_PROPERTY(bool saving READ isSaving NOTIFY savingChanged)
    Q_PROPERTY(bool needsSave READ needsSave NOTIFY needsSaveChanged)
    Q_PROPERTY(bool representsDefaults READ representsDefaults NOTIFY representsDefaultsChanged)

    Q_PROPERTY(int memoryBudgetMb READ memoryBudgetMb WRITE setMemoryBudgetMb NOTIFY valuesChanged)
    Q_PROPERTY(int keepWarm READ keepWarm WRITE setKeepWarm NOTIFY valuesChanged)
    Q_PROPERTY(int keepWarmSeconds READ keepWarmSeconds WRITE setKeepWarmSeconds NOTIFY valuesChanged)
    /// keepWarmSeconds in whole minutes, rounded, for the page.
    Q_PROPERTY(int keepWarmMinutes READ keepWarmMinutes WRITE setKeepWarmMinutes NOTIFY valuesChanged)
    Q_PROPERTY(int idleExitSeconds READ idleExitSeconds WRITE setIdleExitSeconds NOTIFY valuesChanged)
    Q_PROPERTY(bool network READ network WRITE setNetwork NOTIFY valuesChanged)
    Q_PROPERTY(bool allowPrerelease READ allowPrerelease WRITE setAllowPrerelease NOTIFY valuesChanged)

    // The ranges the daemon accepts.
    Q_PROPERTY(int minMemoryBudgetMb READ minMemoryBudgetMb CONSTANT)
    Q_PROPERTY(int maxMemoryBudgetMb READ maxMemoryBudgetMb CONSTANT)
    Q_PROPERTY(int maxKeepWarm READ maxKeepWarm CONSTANT)
    Q_PROPERTY(int maxKeepWarmSeconds READ maxKeepWarmSeconds CONSTANT)
    Q_PROPERTY(int minIdleExitSeconds READ minIdleExitSeconds CONSTANT)
    Q_PROPERTY(int maxIdleExitSeconds READ maxIdleExitSeconds CONSTANT)

public:
    enum class State {
        Loading,
        Ready,
        /// The daemon is not running and could not be started.
        Unavailable,
        /// The daemon is too old to have GetConfig.
        Unsupported,
    };
    Q_ENUM(State)

    /// One complete set of settings; the defaults are the daemon's.
    struct Values {
        qint64 memoryBudgetMb = 512;
        uint keepWarm = 2;
        qint64 keepWarmSeconds = 600;
        qint64 idleExitSeconds = 60;
        bool network = true;
        bool allowPrerelease = false;

        [[nodiscard]] static Values fromMap(const QVariantMap &map);
        /// The keys whose value differs from @p other, typed as SetConfig takes them.
        [[nodiscard]] QVariantMap changesFrom(const Values &other) const;

        friend bool operator==(const Values &, const Values &) = default;
    };

    static constexpr int s_minMemoryBudgetMb = 64;
    static constexpr int s_maxMemoryBudgetMb = 1048576;
    static constexpr int s_maxKeepWarm = 16;
    static constexpr int s_maxKeepWarmSeconds = 86400;
    static constexpr int s_minIdleExitSeconds = 5;
    static constexpr int s_maxIdleExitSeconds = 86400;

    explicit DaemonConfig(QObject *parent = nullptr);

    [[nodiscard]] static int minMemoryBudgetMb()
    {
        return s_minMemoryBudgetMb;
    }
    [[nodiscard]] static int maxMemoryBudgetMb()
    {
        return s_maxMemoryBudgetMb;
    }
    [[nodiscard]] static int maxKeepWarm()
    {
        return s_maxKeepWarm;
    }
    [[nodiscard]] static int maxKeepWarmSeconds()
    {
        return s_maxKeepWarmSeconds;
    }
    [[nodiscard]] static int minIdleExitSeconds()
    {
        return s_minIdleExitSeconds;
    }
    [[nodiscard]] static int maxIdleExitSeconds()
    {
        return s_maxIdleExitSeconds;
    }

    [[nodiscard]] State state() const
    {
        return m_state;
    }
    [[nodiscard]] QString errorText() const
    {
        return m_errorText;
    }
    [[nodiscard]] bool isSaving() const
    {
        return m_saving;
    }
    [[nodiscard]] bool needsSave() const;
    [[nodiscard]] bool representsDefaults() const;

    [[nodiscard]] int memoryBudgetMb() const
    {
        return int(m_edited.memoryBudgetMb);
    }
    void setMemoryBudgetMb(int value);
    [[nodiscard]] int keepWarm() const
    {
        return int(m_edited.keepWarm);
    }
    void setKeepWarm(int value);
    [[nodiscard]] int keepWarmSeconds() const
    {
        return int(m_edited.keepWarmSeconds);
    }
    void setKeepWarmSeconds(int value);
    [[nodiscard]] int keepWarmMinutes() const;
    void setKeepWarmMinutes(int value);
    [[nodiscard]] int idleExitSeconds() const
    {
        return int(m_edited.idleExitSeconds);
    }
    void setIdleExitSeconds(int value);
    [[nodiscard]] bool network() const
    {
        return m_edited.network;
    }
    void setNetwork(bool value);
    [[nodiscard]] bool allowPrerelease() const
    {
        return m_edited.allowPrerelease;
    }
    void setAllowPrerelease(bool value);

    /// The values the daemon last reported (or last accepted).
    [[nodiscard]] Values baseline() const
    {
        return m_baseline;
    }
    [[nodiscard]] Values edited() const
    {
        return m_edited;
    }

    /// Asks the daemon for its settings and drops every edit.
    Q_INVOKABLE void load();
    /// Sends the edited keys to the daemon; saved() or saveFailed() follows.
    Q_INVOKABLE void save();
    /// Sets every value to the daemon's default (not saved yet).
    Q_INVOKABLE void setDefaults();

Q_SIGNALS:
    void stateChanged();
    void errorTextChanged();
    void savingChanged();
    void needsSaveChanged();
    void representsDefaultsChanged();
    void valuesChanged();
    void saved();
    void saveFailed(const QString &message);

private:
    void setState(State state);
    void setErrorText(const QString &text);
    /// Runs @p change and emits whatever it changed.
    void mutate(const std::function<void()> &change);
    /// Replaces the baseline; edited values follow it where they were not edited.
    void rebase(const Values &values, bool dropEdits);
    void failedToLoad(const QString &message);

    Dragoman::Client m_client;
    Values m_baseline;
    Values m_edited;
    State m_state = State::Loading;
    QString m_errorText;
    bool m_saving = false;
    /// Bumped by every load(), so a stale answer is ignored.
    int m_generation = 0;
};
