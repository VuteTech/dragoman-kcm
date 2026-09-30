/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

// The on-demand status against the fake daemon on a private bus.

#include "servicestatus.h"
#include "fakedaemon.h"

#include <QLocale>

using namespace Qt::StringLiterals;

class ServiceStatusTest : public QObject
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

    void fetchesOnDemand()
    {
        ServiceStatus status;
        QTest::qWait(50);
        QVERIFY(!status.isFetched()); // nothing without asking
        status.refresh();
        QVERIFY(status.isLoading());
        QTRY_VERIFY(status.isFetched());
        QVERIFY(!status.isLoading());
        QVERIFY(status.errorText().isEmpty());
        QCOMPARE(status.version(), u"0.1.0"_s);
        QCOMPARE(status.loaded().size(), 2);
        QCOMPARE(status.loaded().at(0), u"Bulgarian to English"_s);
        QVERIFY(status.memory().isEmpty()); // the fake does not report it
    }

    void missingDaemon()
    {
        QVERIFY(m_daemon.setRegistered(false));
        ServiceStatus status;
        status.refresh();
        QTRY_VERIFY(status.isFetched());
        QVERIFY(!status.errorText().isEmpty());
        QVERIFY(m_daemon.setRegistered(true));
    }

private:
    Fake::Daemon m_daemon;
};

QTEST_GUILESS_MAIN(ServiceStatusTest)

#include "servicestatustest.moc"
