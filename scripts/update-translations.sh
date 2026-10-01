#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
# SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Regenerates po/kcm_dragomand.pot through src/Messages.sh, with the xgettext
# keywords of KDE's scripty, and merges it into every po/<lang>/kcm_dragomand.po.
set -euo pipefail

root="$(cd "$(dirname "$0")/.." && pwd)"
podir="$root/po"
export podir

# The i18n() family of KI18n, in C++ and QML alike.
# The commas belong to xgettext's keyword syntax.
# shellcheck disable=SC2054
keywords=(
    -ki18n:1 -ki18nc:1c,2 -ki18np:1,2 -ki18ncp:1c,2,3
    -kki18n:1 -kki18nc:1c,2 -kki18np:1,2 -kki18ncp:1c,2,3
    -kI18N_NOOP:1 -kI18NC_NOOP:1c,2
)
XGETTEXT="xgettext --from-code=UTF-8 --c++ --kde --add-comments=i18n --no-location --package-name=kcm_dragomand --msgid-bugs-address=https://github.com/VuteTech/dragoman-kcm/issues ${keywords[*]}"
export XGETTEXT

# xgettext reads QML as JavaScript; --c++ above would misparse it.
XGETTEXT_QML="xgettext --from-code=UTF-8 --language=JavaScript --no-location ${keywords[*]}"

cd "$root/src"
bash ./Messages.sh
# Messages.sh extracts the C++ sources; the QML files need the JavaScript
# parser, and the two halves are merged.
# shellcheck disable=SC2086 # the command is split into words on purpose
find . -name '*.qml' | sort | xargs $XGETTEXT_QML -o "$podir/qml.pot"
msgcat --use-first "$podir/kcm_dragomand.pot" "$podir/qml.pot" -o "$podir/kcm_dragomand.pot"
rm "$podir/qml.pot"

for po in "$podir"/*/kcm_dragomand.po; do
    [ -e "$po" ] || continue
    msgmerge --quiet --update --backup=none --no-location "$po" "$podir/kcm_dragomand.pot"
done
