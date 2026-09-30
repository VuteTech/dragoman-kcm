// SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
// SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
// SPDX-License-Identifier: GPL-3.0-or-later

import QtQuick
import QtQuick.Controls as QQC2
import QtQuick.Layouts
import org.kde.kirigami as Kirigami
import org.kde.kirigamiaddons.formcard as FormCard
import org.kde.kcmutils as KCMUtils
import dev.l10n_bg.dragomand.kcm

KCMUtils.SimpleKCM {
    id: root

    readonly property DaemonConfig config: kcm.config
    readonly property InstalledPairs pairs: kcm.pairs
    readonly property ServiceStatus status: kcm.status
    readonly property bool configurable: config.state === DaemonConfig.Ready && !config.saving

    // The last message about a finished model operation.
    property string notice: ""
    property bool noticeIsError: false

    topPadding: 0
    leftPadding: 0
    rightPadding: 0
    bottomPadding: Kirigami.Units.largeSpacing

    Connections {
        target: root.pairs
        function onNotify(message: string, failure: bool): void {
            root.notice = message;
            root.noticeIsError = failure;
        }
    }

    ColumnLayout {
        spacing: 0

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.margins: Kirigami.Units.largeSpacing
            position: Kirigami.InlineMessage.Position.Header
            visible: root.config.errorText.length > 0
            type: root.config.state === DaemonConfig.Unsupported ? Kirigami.MessageType.Warning : Kirigami.MessageType.Error
            text: root.config.errorText
            actions: Kirigami.Action {
                visible: root.config.state === DaemonConfig.Unavailable
                text: i18nc("@action:button", "Try Again")
                icon.name: "view-refresh"
                onTriggered: kcm.load()
            }
        }

        // Memory and performance

        FormCard.FormHeader {
            title: i18nc("@title:group", "Memory and Performance")
        }

        FormCard.FormCard {
            enabled: root.configurable

            FormCard.FormSpinBoxDelegate {
                label: i18nc("@label:spinbox", "Memory budget for language models")
                description: i18nc("@info", "The most memory the loaded language models may take together. One language pair needs about 260 MiB. When another pair would not fit, the one unused for longest is unloaded first.")
                from: root.config.minMemoryBudgetMb
                to: root.config.maxMemoryBudgetMb
                stepSize: 64
                value: root.config.memoryBudgetMb
                textFromValue: (value, locale) => i18nc("@item:valuesuffix size in mebibytes", "%1 MiB", value)
                valueFromText: (text, locale) => parseInt(text) || root.config.memoryBudgetMb
                onValueModified: root.config.memoryBudgetMb = value
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormSpinBoxDelegate {
                label: i18nc("@label:spinbox", "Language pairs kept loaded when idle")
                description: i18nc("@info", "Recently used pairs stay in memory, so the next translation starts at once instead of after a few seconds of loading. The pair in use is always kept.")
                from: 0
                to: root.config.maxKeepWarm
                value: root.config.keepWarm
                onValueModified: root.config.keepWarm = value
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormSpinBoxDelegate {
                label: i18nc("@label:spinbox", "Keep idle language pairs loaded for")
                description: i18nc("@info", "After this long without use, a pair is unloaded and its memory is freed.")
                enabled: root.config.keepWarm > 0
                from: 0
                to: root.config.maxKeepWarmSeconds / 60
                value: root.config.keepWarmMinutes
                textFromValue: (value, locale) => i18ncp("@item:valuesuffix", "%1 minute", "%1 minutes", value)
                valueFromText: (text, locale) => parseInt(text) || 0
                onValueModified: root.config.keepWarmMinutes = value
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormSpinBoxDelegate {
                label: i18nc("@label:spinbox", "Stop the translation service when idle after")
                description: i18nc("@info", "Once nothing is loaded, the service leaves memory entirely after this long. It starts again by itself on the next request.")
                from: root.config.minIdleExitSeconds
                to: root.config.maxIdleExitSeconds
                stepSize: 5
                value: root.config.idleExitSeconds
                textFromValue: (value, locale) => i18ncp("@item:valuesuffix", "%1 second", "%1 seconds", value)
                valueFromText: (text, locale) => parseInt(text) || root.config.idleExitSeconds
                onValueModified: root.config.idleExitSeconds = value
            }
        }

        // Network

        FormCard.FormHeader {
            title: i18nc("@title:group", "Network")
        }

        FormCard.FormCard {
            enabled: root.configurable

            FormCard.FormSwitchDelegate {
                text: i18nc("@option:check", "Allow downloads and update checks")
                description: i18nc("@info", "When off, the service never goes online. Installed language models keep working; new ones can come only from distribution packages.")
                checked: root.config.network
                onToggled: root.config.network = checked
            }

            FormCard.FormDelegateSeparator {}

            FormCard.FormSwitchDelegate {
                text: i18nc("@option:check", "Use pre-release language models")
                description: i18nc("@info", "Mozilla's nightly models, published for testing before a release. They may translate better or worse than the released ones.")
                checked: root.config.allowPrerelease
                onToggled: root.config.allowPrerelease = checked
            }
        }

        // Language models

        FormCard.FormHeader {
            title: i18nc("@title:group", "Language Models")
        }

        Kirigami.InlineMessage {
            Layout.fillWidth: true
            Layout.maximumWidth: Kirigami.Units.gridUnit * 30
            Layout.alignment: Qt.AlignHCenter
            Layout.bottomMargin: Kirigami.Units.smallSpacing
            visible: root.notice.length > 0
            type: root.noticeIsError ? Kirigami.MessageType.Error : Kirigami.MessageType.Positive
            text: root.notice
            showCloseButton: true
            onVisibleChanged: {
                if (!visible) {
                    root.notice = "";
                }
            }
        }

        FormCard.FormCard {
            FormCard.FormTextDelegate {
                visible: root.pairs.count === 0
                text: root.pairs.loading ? i18nc("@info:status", "Loading…")
                    : root.pairs.errorText.length > 0 ? i18nc("@info", "The language models cannot be listed.")
                    : i18nc("@info", "No language models are installed.")
                description: root.pairs.loading ? "" : root.pairs.errorText.length > 0 ? root.pairs.errorText
                    : i18nc("@info", "A model is downloaded the first time a program asks for a translation between two languages.")
            }

            Repeater {
                model: root.pairs

                delegate: ColumnLayout {
                    id: pairItem

                    required property int index
                    required property string source
                    required property string target
                    required property string title
                    required property string version
                    required property string availableVersion
                    required property bool updateAvailable
                    required property string sizeText
                    required property string origin
                    required property bool removable
                    required property string qualityText
                    required property bool experimental
                    required property bool busy
                    required property real progress
                    required property string stage

                    function details(): string {
                        const parts = [];
                        parts.push(i18nc("@info model version", "Version %1", version));
                        if (sizeText.length > 0) {
                            parts.push(sizeText);
                        }
                        parts.push(origin === "system" ? i18nc("@info model origin", "from a system package")
                                                       : i18nc("@info model origin", "downloaded"));
                        if (qualityText.length > 0) {
                            parts.push(i18nc("@info %1 is a quality label such as Good", "quality: %1", qualityText));
                        }
                        let text = parts.join(i18nc("@info list separator", ", "));
                        if (updateAvailable) {
                            text = i18nc("@info %1 is the model details", "%1. Version %2 is available.", text, availableVersion);
                        }
                        return text;
                    }

                    Layout.fillWidth: true
                    spacing: 0

                    FormCard.FormDelegateSeparator {
                        visible: pairItem.index > 0
                    }

                    FormCard.AbstractFormDelegate {
                        Layout.fillWidth: true
                        background: null
                        hoverEnabled: false

                        contentItem: RowLayout {
                            spacing: Kirigami.Units.largeSpacing

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: Kirigami.Units.smallSpacing

                                RowLayout {
                                    Layout.fillWidth: true
                                    spacing: Kirigami.Units.smallSpacing

                                    QQC2.Label {
                                        Layout.fillWidth: !pairItem.experimental
                                        text: pairItem.title
                                        elide: Text.ElideRight
                                    }

                                    Kirigami.Chip {
                                        visible: pairItem.experimental
                                        text: i18nc("@info:status model not yet released", "Experimental")
                                        closable: false
                                        checkable: false
                                        icon.name: "flask"
                                    }

                                    Item {
                                        visible: pairItem.experimental
                                        Layout.fillWidth: true
                                    }
                                }

                                QQC2.Label {
                                    Layout.fillWidth: true
                                    text: pairItem.busy ? (pairItem.stage.length > 0 ? pairItem.stage : i18nc("@info:status", "Working…"))
                                                        : pairItem.details()
                                    color: Kirigami.Theme.disabledTextColor
                                    wrapMode: Text.Wrap
                                }

                                QQC2.ProgressBar {
                                    Layout.fillWidth: true
                                    visible: pairItem.busy
                                    from: 0
                                    to: 1
                                    value: Math.max(pairItem.progress, 0)
                                    indeterminate: pairItem.progress < 0
                                }
                            }

                            QQC2.ToolButton {
                                visible: pairItem.removable
                                enabled: !pairItem.busy
                                icon.name: "edit-delete"
                                text: i18nc("@action:button", "Remove")
                                display: QQC2.AbstractButton.IconOnly
                                onClicked: root.pairs.remove(pairItem.source, pairItem.target)

                                QQC2.ToolTip.text: i18nc("@info:tooltip", "Remove the downloaded copy of %1", pairItem.title)
                                QQC2.ToolTip.visible: hovered
                                QQC2.ToolTip.delay: Kirigami.Units.toolTipDelay
                                Accessible.name: QQC2.ToolTip.text
                            }
                        }
                    }
                }
            }

            FormCard.FormDelegateSeparator {
                visible: root.pairs.count > 0
            }

            FormCard.FormTextDelegate {
                visible: root.pairs.count > 0
                text: i18nc("@label", "Disk space used")
                description: i18ncp("@info %2 is a size such as 31 MiB", "%2 for one language model", "%2 for %1 language models", root.pairs.count, root.pairs.totalSizeText)
            }
        }

        FormCard.FormCard {
            Layout.topMargin: Kirigami.Units.largeSpacing

            FormCard.FormButtonDelegate {
                icon.name: "system-software-update"
                text: root.pairs.checkingForUpdates ? i18nc("@action:button", "Checking for Updates…") : i18nc("@action:button", "Check for Updates")
                description: root.config.state === DaemonConfig.Ready && !root.config.network
                    ? i18nc("@info", "Downloads and update checks are turned off above.")
                    : i18nc("@info", "Asks Mozilla's model service for newer versions.")
                enabled: root.pairs.loaded && !root.pairs.checkingForUpdates && (root.config.state !== DaemonConfig.Ready || root.config.network)
                onClicked: root.pairs.checkForUpdates()
            }

            FormCard.FormDelegateSeparator {
                visible: root.pairs.updateCount > 0
            }

            FormCard.FormButtonDelegate {
                visible: root.pairs.updateCount > 0
                icon.name: "update-none"
                text: root.pairs.updating ? i18nc("@action:button", "Updating…") : i18nc("@action:button", "Update All")
                description: i18ncp("@info", "An update is available for one language model.", "Updates are available for %1 language models.", root.pairs.updateCount)
                enabled: !root.pairs.updating
                onClicked: root.pairs.updateAll()
            }

            FormCard.FormDelegateSeparator {
                visible: kcm.krakomanAvailable
            }

            FormCard.FormButtonDelegate {
                visible: kcm.krakomanAvailable
                icon.name: "dev.l10n_bg.krakoman"
                text: i18nc("@action:button", "Manage in Krakoman")
                description: i18nc("@info", "Install language models for more languages, and translate text.")
                onClicked: kcm.openKrakoman()
            }
        }

        // Status

        FormCard.FormHeader {
            title: i18nc("@title:group", "Status")
        }

        FormCard.FormCard {
            FormCard.FormTextDelegate {
                visible: root.status.fetched && root.status.errorText.length > 0
                text: i18nc("@info", "The translation service does not answer.")
                description: root.status.errorText
            }

            FormCard.FormTextDelegate {
                visible: root.status.fetched && root.status.errorText.length === 0
                text: i18nc("@label", "Version")
                description: root.status.version
            }

            FormCard.FormDelegateSeparator {
                visible: root.status.fetched && root.status.errorText.length === 0 && root.status.memory.length > 0
            }

            FormCard.FormTextDelegate {
                visible: root.status.fetched && root.status.errorText.length === 0 && root.status.memory.length > 0
                text: i18nc("@label", "Memory in use")
                description: root.status.memory
            }

            FormCard.FormDelegateSeparator {
                visible: root.status.fetched && root.status.errorText.length === 0
            }

            FormCard.FormTextDelegate {
                visible: root.status.fetched && root.status.errorText.length === 0
                text: i18nc("@label", "Loaded language pairs")
                description: root.status.loaded.length > 0 ? root.status.loaded.join(i18nc("@info list separator", ", "))
                                                           : i18nc("@info no language pairs loaded", "None")
            }

            FormCard.FormDelegateSeparator {
                visible: root.status.fetched
            }

            FormCard.FormButtonDelegate {
                icon.name: "view-refresh"
                text: root.status.fetched ? i18nc("@action:button", "Refresh Status") : i18nc("@action:button", "Show Status")
                description: root.status.fetched ? "" : i18nc("@info", "The version of the translation service, its memory use and the language pairs loaded right now.")
                enabled: !root.status.loading
                onClicked: root.status.refresh()
            }
        }
    }
}
