/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

// The buffered settings against the fake daemon on a private bus.

#include "daemonconfig.h"
#include "fakedaemon.h"

#include <QSignalSpy>

using namespace Qt::StringLiterals;

namespace
{

/// A daemon from before GetConfig: it answers GetStatus only.
class OldTranslator : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "dev.l10n_bg.dragomand.Translator1")
public Q_SLOTS:
    Q_SCRIPTABLE QVariantMap GetStatus()
    {
        return {{u"version"_s, u"0.1.0"_s}};
    }
};

} // namespace

class DaemonConfigTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        m_daemon.start();
        if (QTest::currentTestFailed() || QTest::currentTestResolved()) {
            return; // skipped without dbus-daemon, or failed
        }
        m_defaults = m_daemon.translator->config;
    }

    void cleanupTestCase()
    {
        m_daemon.stop();
    }

    void init()
    {
        m_daemon.translator->config = m_defaults;
        QVERIFY(m_daemon.setRegistered(true));
    }

    void loads()
    {
        DaemonConfig config;
        QCOMPARE(config.state(), DaemonConfig::State::Loading);
        QSignalSpy state(&config, &DaemonConfig::stateChanged);
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        QCOMPARE(config.memoryBudgetMb(), 512);
        QCOMPARE(config.keepWarm(), 2);
        QCOMPARE(config.keepWarmSeconds(), 600);
        QCOMPARE(config.keepWarmMinutes(), 10);
        QCOMPARE(config.idleExitSeconds(), 60);
        QVERIFY(config.network());
        QVERIFY(!config.allowPrerelease());
        QVERIFY(!config.needsSave());
        QVERIFY(config.representsDefaults());
        QVERIFY(config.errorText().isEmpty());
    }

    void loadsOtherValues()
    {
        m_daemon.translator->config.insert(u"memory_budget_mb"_s, QVariant::fromValue(qulonglong(256)));
        m_daemon.translator->config.insert(u"allow_prerelease"_s, true);
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        QCOMPARE(config.memoryBudgetMb(), 256);
        QVERIFY(config.allowPrerelease());
        QVERIFY(!config.needsSave());
        QVERIFY(!config.representsDefaults());
    }

    void editsNeedSaving()
    {
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        QSignalSpy needsSave(&config, &DaemonConfig::needsSaveChanged);
        QSignalSpy defaults(&config, &DaemonConfig::representsDefaultsChanged);
        QSignalSpy values(&config, &DaemonConfig::valuesChanged);

        config.setKeepWarm(3);
        QVERIFY(config.needsSave());
        QVERIFY(!config.representsDefaults());
        QCOMPARE(needsSave.count(), 1);
        QCOMPARE(defaults.count(), 1);
        QCOMPARE(values.count(), 1);

        config.setKeepWarm(3); // no change, no signal
        QCOMPARE(values.count(), 1);

        config.setKeepWarm(2); // back to what was loaded
        QVERIFY(!config.needsSave());
        QVERIFY(config.representsDefaults());
        QCOMPARE(needsSave.count(), 2);
    }

    void clampsToTheDaemonsRanges()
    {
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        config.setMemoryBudgetMb(1);
        QCOMPARE(config.memoryBudgetMb(), 64);
        config.setKeepWarm(100);
        QCOMPARE(config.keepWarm(), 16);
        config.setIdleExitSeconds(0);
        QCOMPARE(config.idleExitSeconds(), 5);
        config.setKeepWarmSeconds(1000000);
        QCOMPARE(config.keepWarmSeconds(), 86400);
    }

    void minutesKeepOddSeconds()
    {
        m_daemon.translator->config.insert(u"keep_warm_seconds"_s, QVariant::fromValue(qulonglong(90)));
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        QCOMPARE(config.keepWarmMinutes(), 2);
        config.setKeepWarmMinutes(2); // what the page shows: not an edit
        QCOMPARE(config.keepWarmSeconds(), 90);
        QVERIFY(!config.needsSave());
        config.setKeepWarmMinutes(5);
        QCOMPARE(config.keepWarmSeconds(), 300);
        QVERIFY(config.needsSave());
    }

    void savesOnlyTheChangedKeys()
    {
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        // Changed behind the page's back, without a signal: saving the
        // whole set would put the old value back.
        m_daemon.translator->config.insert(u"network"_s, false);

        Dragoman::Client observer;
        QSignalSpy changed(&observer, &Dragoman::Client::configChanged);
        QSignalSpy saved(&config, &DaemonConfig::saved);
        config.setMemoryBudgetMb(1024);
        config.setKeepWarm(4);
        config.save();
        QVERIFY(!config.needsSave()); // clean while the call runs
        QTRY_COMPARE(saved.count(), 1);
        QVERIFY(!config.isSaving());
        QVERIFY(!config.needsSave());
        QVERIFY(config.errorText().isEmpty());

        const QVariantMap stored = m_daemon.translator->config;
        QCOMPARE(stored.value(u"memory_budget_mb"_s).metaType(), QMetaType::fromType<qulonglong>());
        QCOMPARE(stored.value(u"memory_budget_mb"_s).toULongLong(), 1024ULL);
        QCOMPARE(stored.value(u"keep_warm"_s).metaType(), QMetaType::fromType<uint>());
        QCOMPARE(stored.value(u"keep_warm"_s).toUInt(), 4U);
        QCOMPARE(stored.value(u"network"_s).toBool(), false);
        QTRY_COMPARE(changed.count(), 1);
        QCOMPARE(changed.at(0).at(0).toMap().value(u"memory_budget_mb"_s).toULongLong(), 1024ULL);
    }

    void savingNothingChangesNothing()
    {
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        Dragoman::Client observer;
        QSignalSpy changed(&observer, &Dragoman::Client::configChanged);
        QSignalSpy saved(&config, &DaemonConfig::saved);
        config.save();
        QCOMPARE(saved.count(), 1);
        QTest::qWait(100);
        QCOMPARE(changed.count(), 0);
    }

    void failedSaveKeepsTheEdits()
    {
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        config.setIdleExitSeconds(120);
        QVERIFY(m_daemon.setRegistered(false));
        QSignalSpy failed(&config, &DaemonConfig::saveFailed);
        config.save();
        QTRY_COMPARE(failed.count(), 1);
        QVERIFY(config.needsSave());
        QCOMPARE(config.idleExitSeconds(), 120);
        QVERIFY(!config.errorText().isEmpty());
        QCOMPARE(m_daemon.translator->config.value(u"idle_exit_seconds"_s).toULongLong(), 60ULL);

        QVERIFY(m_daemon.setRegistered(true));
        QSignalSpy saved(&config, &DaemonConfig::saved);
        config.save();
        QTRY_COMPARE(saved.count(), 1);
        QVERIFY(config.errorText().isEmpty());
        QCOMPARE(m_daemon.translator->config.value(u"idle_exit_seconds"_s).toULongLong(), 120ULL);
    }

    void defaults()
    {
        m_daemon.translator->config.insert(u"memory_budget_mb"_s, QVariant::fromValue(qulonglong(256)));
        m_daemon.translator->config.insert(u"network"_s, false);
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        QVERIFY(!config.representsDefaults());

        config.setDefaults();
        QVERIFY(config.representsDefaults());
        QVERIFY(config.needsSave());
        QCOMPARE(config.memoryBudgetMb(), 512);
        QVERIFY(config.network());

        QSignalSpy saved(&config, &DaemonConfig::saved);
        config.save();
        QTRY_COMPARE(saved.count(), 1);
        QCOMPARE(m_daemon.translator->config.value(u"memory_budget_mb"_s).toULongLong(), 512ULL);
        QCOMPARE(m_daemon.translator->config.value(u"network"_s).toBool(), true);
        QVERIFY(!config.needsSave());
        QVERIFY(config.representsDefaults());
    }

    void externalChangeReloads()
    {
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);

        Dragoman::Client other;
        other.setConfig({{u"memory_budget_mb"_s, QVariant::fromValue(qulonglong(2048))}}, [](const QString &) { });
        QTRY_COMPARE(config.memoryBudgetMb(), 2048);
        QVERIFY(!config.needsSave());
        QVERIFY(!config.representsDefaults());
    }

    void externalChangeKeepsEdits()
    {
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        config.setKeepWarm(5);

        Dragoman::Client other;
        other.setConfig({{u"memory_budget_mb"_s, QVariant::fromValue(qulonglong(2048))}, {u"keep_warm"_s, QVariant::fromValue(uint(1))}},
                        [](const QString &) { });
        QTRY_COMPARE(config.memoryBudgetMb(), 2048); // not edited: follows the daemon
        QCOMPARE(config.keepWarm(), 5); // edited: kept
        QCOMPARE(config.baseline().keepWarm, 1U);
        QVERIFY(config.needsSave());
        QCOMPARE(config.edited().changesFrom(config.baseline()).keys(), QStringList{u"keep_warm"_s});
    }

    void missingDaemon()
    {
        QVERIFY(m_daemon.setRegistered(false));
        DaemonConfig config;
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Unavailable);
        QVERIFY(!config.errorText().isEmpty());
        QVERIFY(!config.needsSave());
        QVERIFY(config.representsDefaults());
        config.setDefaults(); // nothing to reset
        config.save(); // nothing to save
        QVERIFY(!config.needsSave());

        // Back, and loaded on the next try.
        QVERIFY(m_daemon.setRegistered(true));
        config.load();
        QTRY_COMPARE(config.state(), DaemonConfig::State::Ready);
        QVERIFY(config.errorText().isEmpty());
    }

    void olderDaemon()
    {
        QVERIFY(m_daemon.setRegistered(false));
        {
            QDBusConnection old = QDBusConnection::connectToBus(QString::fromUtf8(qgetenv("DBUS_SESSION_BUS_ADDRESS")), u"old-dragomand"_s);
            QVERIFY(old.isConnected());
            OldTranslator translator;
            QVERIFY(old.registerObject(QString::fromLatin1(Fake::objectPath), &translator, QDBusConnection::ExportScriptableSlots));
            QVERIFY(old.registerService(QString::fromLatin1(Fake::serviceName)));

            DaemonConfig config;
            config.load();
            QTRY_COMPARE(config.state(), DaemonConfig::State::Unsupported);
            QVERIFY(config.errorText().contains(u"0.2"_s));
            QVERIFY(!config.needsSave());

            old.unregisterService(QString::fromLatin1(Fake::serviceName));
            old.unregisterObject(QString::fromLatin1(Fake::objectPath));
        }
        QDBusConnection::disconnectFromBus(u"old-dragomand"_s);
    }

private:
    Fake::Daemon m_daemon;
    QVariantMap m_defaults;
};

QTEST_GUILESS_MAIN(DaemonConfigTest)

#include "daemonconfigtest.moc"
