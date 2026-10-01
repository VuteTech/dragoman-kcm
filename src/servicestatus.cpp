/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "servicestatus.h"
#include "languagenames.h"

#include <KFormat>
#include <KLocalizedString>

ServiceStatus::ServiceStatus(QObject *parent)
    : QObject(parent)
{
}

QString ServiceStatus::memory() const
{
    if (m_status.residentMb < 0) {
        return {};
    }
    return KFormat().formatByteSize(double(m_status.residentMb) * 1024 * 1024, 0);
}

QStringList ServiceStatus::loaded() const
{
    QStringList routes;
    for (const QString &route : m_status.loaded) {
        // "bg-en", "zh-Hant-en": a script subtag (Hant) joins the code before it.
        QStringList codes;
        for (const QString &part : route.split(u'-')) {
            if (!codes.isEmpty() && part.size() == 4 && part.front().isUpper()) {
                codes.last() += u'-' + part;
            } else {
                codes.append(part);
            }
        }
        routes.append(codes.size() != 2
                          ? route
                          : i18nc("@item language pair", "%1 to %2", Dragoman::languageName(codes.at(0)), Dragoman::languageNameInSentence(codes.at(1))));
    }
    return routes;
}

void ServiceStatus::refresh()
{
    if (m_loading) {
        return;
    }
    m_loading = true;
    Q_EMIT changed();
    m_client.status([this](const Dragoman::DaemonStatus &status, const QString &error) {
        m_loading = false;
        m_fetched = true;
        m_status = status;
        m_errorText = error;
        Q_EMIT changed();
    });
}
