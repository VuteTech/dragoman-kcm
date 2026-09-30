/*
    SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
    SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
    SPDX-License-Identifier: GPL-3.0-or-later
*/

#include "installedpairs.h"
#include "languagenames.h"

#include <KFormat>
#include <KLocalizedString>

#include <QCollator>

#include <algorithm>

using namespace Qt::StringLiterals;

InstalledPairs::InstalledPairs(QObject *parent)
    : QAbstractListModel(parent)
{
}

int InstalledPairs::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : int(m_pairs.size());
}

QVariant InstalledPairs::data(const QModelIndex &index, int role) const
{
    if (!checkIndex(index, CheckIndexOption::IndexIsValid | CheckIndexOption::ParentIsInvalid)) {
        return {};
    }
    const Dragoman::PairInfo &pair = m_pairs.at(index.row());
    const auto operation = m_operations.constFind(key(pair.source, pair.target));
    const bool busy = operation != m_operations.cend();
    switch (role) {
    case Qt::DisplayRole:
    case TitleRole:
        return title(pair.source, pair.target);
    case SourceRole:
        return pair.source;
    case TargetRole:
        return pair.target;
    case VersionRole:
        return pair.installedVersion;
    case AvailableVersionRole:
        return pair.availableVersion;
    case UpdateAvailableRole:
        return pair.hasUpdate();
    case SizeTextRole:
        return pair.size >= 0 ? KFormat().formatByteSize(double(pair.size)) : QString();
    case OriginRole:
        return pair.origin;
    case RemovableRole:
        return pair.isRemovable();
    case QualityTextRole:
        return qualityText(pair.quality);
    case ExperimentalRole:
        return isExperimental(pair.releaseStatus);
    case BusyRole:
        return busy;
    case ProgressRole:
        return busy ? operation->progress : -1.0;
    case StageRole:
        return busy ? operation->stage : QString();
    default:
        return {};
    }
}

QHash<int, QByteArray> InstalledPairs::roleNames() const
{
    return {
        {SourceRole, "source"},
        {TargetRole, "target"},
        {TitleRole, "title"},
        {VersionRole, "version"},
        {AvailableVersionRole, "availableVersion"},
        {UpdateAvailableRole, "updateAvailable"},
        {SizeTextRole, "sizeText"},
        {OriginRole, "origin"},
        {RemovableRole, "removable"},
        {QualityTextRole, "qualityText"},
        {ExperimentalRole, "experimental"},
        {BusyRole, "busy"},
        {ProgressRole, "progress"},
        {StageRole, "stage"},
    };
}

int InstalledPairs::updateCount() const
{
    return int(std::ranges::count_if(m_pairs, &Dragoman::PairInfo::hasUpdate));
}

QString InstalledPairs::totalSizeText() const
{
    qint64 total = 0;
    for (const auto &pair : m_pairs) {
        total += std::max<qint64>(pair.size, 0);
    }
    return KFormat().formatByteSize(double(total));
}

bool InstalledPairs::isUpdating() const
{
    // Removals are operations too, but without a job.
    return std::ranges::any_of(m_operations, [](const Operation &operation) {
        return !operation.job.isNull();
    });
}

QString InstalledPairs::qualityText(double quality)
{
    // Thresholds on the COMET-22 scores of Mozilla's current models.
    if (quality < 0) {
        return {};
    }
    if (quality >= 0.88) {
        return i18nc("@info translation quality", "Very good");
    }
    if (quality >= 0.85) {
        return i18nc("@info translation quality", "Good");
    }
    if (quality >= 0.80) {
        return i18nc("@info translation quality", "Fair");
    }
    return i18nc("@info translation quality", "Basic");
}

bool InstalledPairs::isExperimental(const QString &releaseStatus)
{
    // "Release", "Release Desktop", "Release Android" are released; unknown
    // (empty) says nothing.
    return !releaseStatus.isEmpty() && !releaseStatus.startsWith("Release"_L1);
}

void InstalledPairs::refresh()
{
    if (m_loading) {
        m_refreshAgain = true; // the answer on its way may already be stale
        return;
    }
    m_loading = true;
    Q_EMIT loadingChanged();
    m_client.listPairs([this](const QList<Dragoman::PairInfo> &pairs, const QString &error) {
        m_loading = false;
        if (error.isEmpty()) {
            m_loaded = true;
            setPairs(pairs);
        }
        setErrorText(error);
        Q_EMIT loadingChanged();
        if (std::exchange(m_refreshAgain, false)) {
            refresh();
        }
    });
}

void InstalledPairs::setPairs(const QList<Dragoman::PairInfo> &pairs)
{
    QList<Dragoman::PairInfo> installed;
    std::ranges::copy_if(pairs, std::back_inserter(installed), &Dragoman::PairInfo::isInstalled);
    QCollator collator;
    std::ranges::stable_sort(installed, [this, &collator](const Dragoman::PairInfo &a, const Dragoman::PairInfo &b) {
        return collator.compare(title(a.source, a.target), title(b.source, b.target)) < 0;
    });

    const auto sameKey = [](const Dragoman::PairInfo &a, const Dragoman::PairInfo &b) {
        return a.source == b.source && a.target == b.target;
    };
    if (std::ranges::equal(installed, m_pairs, sameKey)) {
        // The same rows: update in place so the view keeps its state.
        for (qsizetype row = 0; row < installed.size(); ++row) {
            if (installed.at(row) != m_pairs.at(row)) {
                m_pairs[row] = installed.at(row);
                Q_EMIT dataChanged(index(int(row)), index(int(row)));
            }
        }
    } else {
        beginResetModel();
        m_pairs = installed;
        endResetModel();
    }
    Q_EMIT countsChanged();
}

void InstalledPairs::remove(const QString &source, const QString &target)
{
    const QString pairKey = key(source, target);
    if (m_operations.contains(pairKey)) {
        return;
    }
    m_operations.insert(pairKey, {});
    emitRowChanged(source, target);
    m_client.removePair(source, target, [this, pairKey, source, target](const QString &error) {
        m_operations.remove(pairKey);
        emitRowChanged(source, target);
        if (!error.isEmpty()) {
            Q_EMIT notify(i18nc("@info %1 is a language pair, %2 an error message", "Removing %1 failed: %2", title(source, target), error), true);
        }
        refresh();
    });
}

void InstalledPairs::checkForUpdates()
{
    if (m_updateCheck) {
        return;
    }
    m_updateCheck = m_client.checkForUpdates();
    Q_EMIT busyChanged();
    connect(m_updateCheck, &Dragoman::Job::finished, this, [this](const Dragoman::Reply &reply) {
        m_updateCheck = nullptr;
        Q_EMIT busyChanged();
        if (reply.ok()) {
            const auto updates = Dragoman::toStringList(reply.results.value(u"updates"_s)).size();
            Q_EMIT notify(updates == 0
                              ? i18nc("@info", "Every installed language model is up to date.")
                              : i18ncp("@info", "An update is available for one language model.", "Updates are available for %1 language models.", updates),
                          false);
        } else if (!reply.cancelled()) {
            Q_EMIT notify(i18nc("@info %1 is an error message", "Checking for updates failed: %1", reply.error), true);
        }
        refresh();
    });
}

void InstalledPairs::updateAll()
{
    for (const auto &pair : std::as_const(m_pairs)) {
        if (pair.hasUpdate()) {
            update(pair.source, pair.target);
        }
    }
}

void InstalledPairs::update(const QString &source, const QString &target)
{
    const QString pairKey = key(source, target);
    if (m_operations.contains(pairKey)) {
        return;
    }
    Dragoman::Job *job = m_client.installPair(source, target);
    m_operations.insert(pairKey, {job, -1, {}});
    emitRowChanged(source, target);
    Q_EMIT busyChanged();
    connect(job, &Dragoman::Job::progress, this, [this, pairKey, source, target](double fraction, const QString &stage) {
        if (auto it = m_operations.find(pairKey); it != m_operations.end()) {
            it->progress = fraction;
            it->stage = stage;
            emitRowChanged(source, target);
        }
    });
    connect(job, &Dragoman::Job::finished, this, [this, pairKey, source, target](const Dragoman::Reply &reply) {
        m_operations.remove(pairKey);
        emitRowChanged(source, target);
        Q_EMIT busyChanged();
        if (!reply.ok() && !reply.cancelled()) {
            Q_EMIT notify(i18nc("@info %1 is a language pair, %2 an error message", "Updating %1 failed: %2", title(source, target), reply.error), true);
        }
        refresh();
    });
}

int InstalledPairs::rowOf(const QString &source, const QString &target) const
{
    const auto it = std::ranges::find_if(m_pairs, [&](const Dragoman::PairInfo &pair) {
        return pair.source == source && pair.target == target;
    });
    return it == m_pairs.cend() ? -1 : int(std::distance(m_pairs.cbegin(), it));
}

void InstalledPairs::emitRowChanged(const QString &source, const QString &target)
{
    if (const int row = rowOf(source, target); row >= 0) {
        Q_EMIT dataChanged(index(row), index(row), {BusyRole, ProgressRole, StageRole});
    }
}

void InstalledPairs::setErrorText(const QString &text)
{
    if (m_errorText != text) {
        m_errorText = text;
        Q_EMIT errorTextChanged();
    }
}

QString InstalledPairs::title(const QString &source, const QString &target) const
{
    return i18nc("@item language pair", "%1 to %2", Dragoman::languageName(source), Dragoman::languageName(target));
}

QString InstalledPairs::key(const QString &source, const QString &target)
{
    return source + u'-' + target;
}
