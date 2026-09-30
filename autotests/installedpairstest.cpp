/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

// The installed language models against the fake daemon on a private bus.

#include "installedpairs.h"
#include "fakedaemon.h"

#include <QAbstractItemModelTester>
#include <QLocale>
#include <QSignalSpy>

using namespace Qt::StringLiterals;

class InstalledPairsTest : public QObject
{
    Q_OBJECT
private Q_SLOTS:
    void initTestCase()
    {
        qputenv("LANGUAGE", "C");
        QLocale::setDefault(QLocale::c());
        m_daemon.start();
        if (QTest::currentTestFailed() || QTest::currentTestResolved()) {
            return; // skipped without dbus-daemon, or failed
        }
    }

    void cleanupTestCase()
    {
        m_daemon.stop();
    }

    void init()
    {
        m_daemon.translator->installed = {u"bg-en"_s};
    }

    void qualityLabels_data()
    {
        QTest::addColumn<double>("quality");
        QTest::addColumn<QString>("label");
        QTest::newRow("unknown") << -1.0 << QString();
        QTest::newRow("very good") << 0.88 << u"Very good"_s;
        QTest::newRow("good") << 0.8719 << u"Good"_s;
        QTest::newRow("good at the edge") << 0.85 << u"Good"_s;
        QTest::newRow("fair") << 0.80 << u"Fair"_s;
        QTest::newRow("basic") << 0.7999 << u"Basic"_s;
        QTest::newRow("zero") << 0.0 << u"Basic"_s;
    }

    void qualityLabels()
    {
        QFETCH(double, quality);
        QFETCH(QString, label);
        QCOMPARE(InstalledPairs::qualityText(quality), label);
    }

    void experimental()
    {
        QVERIFY(!InstalledPairs::isExperimental(QString()));
        QVERIFY(!InstalledPairs::isExperimental(u"Release"_s));
        QVERIFY(!InstalledPairs::isExperimental(u"Release Desktop"_s));
        QVERIFY(InstalledPairs::isExperimental(u"Nightly"_s));
        QVERIFY(InstalledPairs::isExperimental(u"Beta"_s));
    }

    void listsInstalledPairsOnly()
    {
        m_daemon.translator->installed = {u"bg-en"_s, u"de-en"_s};
        InstalledPairs model;
        QAbstractItemModelTester tester(&model);
        model.refresh();
        QTRY_VERIFY(model.isLoaded());
        QCOMPARE(model.rowCount(), 2);
        QCOMPARE(model.updateCount(), 2);
        QCOMPARE(model.totalSizeText(), u"64.0 MiB"_s);

        // Sorted by title.
        const QModelIndex bgEn = model.index(0);
        QCOMPARE(bgEn.data(InstalledPairs::TitleRole).toString(), u"Bulgarian to English"_s);
        QCOMPARE(bgEn.data(InstalledPairs::VersionRole).toString(), u"3.0"_s);
        QCOMPARE(bgEn.data(InstalledPairs::AvailableVersionRole).toString(), u"3.1"_s);
        QVERIFY(bgEn.data(InstalledPairs::UpdateAvailableRole).toBool());
        QVERIFY(bgEn.data(InstalledPairs::RemovableRole).toBool());
        QCOMPARE(bgEn.data(InstalledPairs::OriginRole).toString(), u"user"_s);
        QCOMPARE(bgEn.data(InstalledPairs::SizeTextRole).toString(), u"32.0 MiB"_s);
        QCOMPARE(bgEn.data(InstalledPairs::QualityTextRole).toString(), u"Good"_s);
        QVERIFY(!bgEn.data(InstalledPairs::ExperimentalRole).toBool());
        QVERIFY(!bgEn.data(InstalledPairs::BusyRole).toBool());

        const QModelIndex deEn = model.index(1);
        QCOMPARE(deEn.data(InstalledPairs::TitleRole).toString(), u"German to English"_s);
        QVERIFY(deEn.data(InstalledPairs::QualityTextRole).toString().isEmpty()); // unknown
    }

    void removes()
    {
        InstalledPairs model;
        QAbstractItemModelTester tester(&model);
        model.refresh();
        QTRY_VERIFY(model.isLoaded());
        QCOMPARE(model.count(), 1);
        QSignalSpy notify(&model, &InstalledPairs::notify);

        model.remove(u"bg"_s, u"en"_s);
        QVERIFY(model.index(0).data(InstalledPairs::BusyRole).toBool());
        QTRY_COMPARE(model.count(), 0);
        QCOMPARE(notify.count(), 0); // success is silent
        QVERIFY(!m_daemon.translator->installed.contains(u"bg-en"_s));

        model.remove(u"bg"_s, u"en"_s); // no longer installed
        QTRY_COMPARE(notify.count(), 1);
        QVERIFY(notify.at(0).at(1).toBool());
    }

    void checksForUpdates()
    {
        InstalledPairs model;
        model.refresh();
        QTRY_VERIFY(model.isLoaded());
        QSignalSpy notify(&model, &InstalledPairs::notify);
        model.checkForUpdates();
        QVERIFY(model.isCheckingForUpdates());
        QTRY_COMPARE(notify.count(), 1);
        QVERIFY(!model.isCheckingForUpdates());
        QCOMPARE(notify.at(0).at(0).toString(), u"An update is available for one language model."_s);
        QVERIFY(!notify.at(0).at(1).toBool());
    }

    void updatesAll()
    {
        InstalledPairs model;
        QAbstractItemModelTester tester(&model);
        model.refresh();
        QTRY_VERIFY(model.isLoaded());
        QSignalSpy notify(&model, &InstalledPairs::notify);
        QSignalSpy busy(&model, &InstalledPairs::busyChanged);
        model.updateAll();
        QVERIFY(model.isUpdating());
        QVERIFY(model.index(0).data(InstalledPairs::BusyRole).toBool());
        QTRY_VERIFY(!model.isUpdating());
        QCOMPARE(busy.count(), 2);
        QCOMPARE(notify.count(), 0);
        QVERIFY(!model.index(0).data(InstalledPairs::BusyRole).toBool());
    }

    void missingDaemon()
    {
        QVERIFY(m_daemon.setRegistered(false));
        InstalledPairs model;
        model.refresh();
        QTRY_VERIFY(!model.isLoading());
        QVERIFY(!model.isLoaded());
        QVERIFY(!model.errorText().isEmpty());
        QCOMPARE(model.count(), 0);
        QVERIFY(m_daemon.setRegistered(true));
    }

private:
    Fake::Daemon m_daemon;
};

QTEST_GUILESS_MAIN(InstalledPairsTest)

#include "installedpairstest.moc"
