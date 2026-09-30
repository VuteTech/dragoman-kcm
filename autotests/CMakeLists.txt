# SPDX-FileCopyrightText: 2026 Blagovest Petrov <blagovest@petrovs.info>
# SPDX-FileCopyrightText: 2026 Vute Tech Ltd. <https://vute.tech>
# SPDX-License-Identifier: GPL-3.0-or-later

include(ECMAddTests)

# Against libdragoman-qt's fake daemon on a private bus. Qt6::Gui registers
# the GUI types that QAbstractItemModelTester asks about.
foreach(test daemonconfigtest installedpairstest servicestatustest)
    ecm_add_test(${test}.cpp ${DragomanQt_TESTING_INCLUDE_DIR}/fakedaemon.h
        TEST_NAME ${test}
        LINK_LIBRARIES dragomankcmcore Qt6::Gui Qt6::Test)
    target_include_directories(${test} PRIVATE ${DragomanQt_TESTING_INCLUDE_DIR})
endforeach()
