# **********************************************************************
# * Copyright (C) 2023 MX Authors
# *
# * Authors: Adrian
# *          MX Linux <http://mxlinux.org>
# *
# * This is free software: you can redistribute it and/or modify
# * it under the terms of the GNU General Public License as published by
# * the Free Software Foundation, either version 3 of the License, or
# * (at your option) any later version.
# *
# * This program is distributed in the hope that it will be useful,
# * but WITHOUT ANY WARRANTY; without even the implied warranty of
# * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# * GNU General Public License for more details.
# *
# * You should have received a copy of the GNU General Public License
# * along with this package. If not, see <http://www.gnu.org/licenses/>.
# **********************************************************************/

QT       += core gui widgets
CONFIG   += debug_and_release warn_on strict_c c++17

CONFIG(release, debug|release) {
    DEFINES += NDEBUG
    QMAKE_CXXFLAGS += -flto=auto
    QMAKE_LFLAGS += -flto=auto
    QMAKE_CXXFLAGS_RELEASE = -O3
}

QMAKE_CXXFLAGS += -Wpedantic -pedantic -Werror=return-type -Werror=switch
QMAKE_CXXFLAGS += -Werror=uninitialized -Werror=return-local-addr -Werror

TARGET = restore-gui
TEMPLATE = app

# The following define makes your compiler warn you if you use any
# feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

SOURCES += main.cpp\
    cmd.cpp \
    git.cpp \
    mainwindow.cpp \
    about.cpp

HEADERS  += \
    cmd.h \
    git.h \
    mainwindow.h \
    version.h \
    about.h

FORMS    += \
    mainwindow.ui

TRANSLATIONS += translations/restore-gui_ca.ts \
                translations/restore-gui_de.ts \
                translations/restore-gui_el.ts \
                translations/restore-gui_en.ts \
                translations/restore-gui_es.ts \
                translations/restore-gui_fr.ts \
                translations/restore-gui_fr_BE.ts \
                translations/restore-gui_it.ts \
                translations/restore-gui_ja.ts \
                translations/restore-gui_nl.ts \
                translations/restore-gui_ro.ts \
                translations/restore-gui_sv.ts

RESOURCES += \
    images.qrc
