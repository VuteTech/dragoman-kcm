#!/usr/bin/env bash
# SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
# SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
# SPDX-License-Identifier: GPL-3.0-or-later
#
# Extracts the translatable strings of the C++ sources into
# $podir/kcm_dragomand.pot, following KDE's Messages.sh convention (XGETTEXT and
# podir come from the environment). scripts/update-translations.sh runs it
# and adds the strings of the QML files.
# shellcheck disable=SC2154
$XGETTEXT $(find . -name '*.cpp' -o -name '*.h' | sort) -o "$podir/kcm_dragomand.pot"
