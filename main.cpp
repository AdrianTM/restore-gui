/**********************************************************************
 *  main.cpp
 **********************************************************************
 * Copyright (C) 2023 MX Authors
 *
 * Authors: Adrian
 *          MX Linux <http://mxlinux.org>
 *
 * This is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this package. If not, see <http://www.gnu.org/licenses/>.
 **********************************************************************/

#include <QApplication>
#include <QCommandLineParser>
#include <QDebug>
#include <QFile>
#include <QIcon>
#include <QLibraryInfo>
#include <QLocale>
#include <QTranslator>

#include "mainwindow.h"
#include <unistd.h>

int main(int argc, char *argv[])
{
    if (getuid() == 0) {
        qputenv("XDG_RUNTIME_DIR", "/run/user/0");
        qunsetenv("SESSION_MANAGER");
    }
    QApplication app(argc, argv);
    if (getuid() == 0) {
        qputenv("HOME", "/root");
    }
    QApplication::setOrganizationName(QStringLiteral("MX-Linux"));
    QIcon appIcon = QIcon::fromTheme(QApplication::applicationName(), QIcon(":/icons/fallback-icon.png"));
    QApplication::setWindowIcon(appIcon);

    QTranslator qtTran;
    if (qtTran.load("qt_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtTran);
    }

    QTranslator qtBaseTran;
    if (qtBaseTran.load("qtbase_" + QLocale::system().name(), QLibraryInfo::location(QLibraryInfo::TranslationsPath))) {
        QApplication::installTranslator(&qtBaseTran);
    }

    QTranslator appTran;
    if (appTran.load(QApplication::applicationName() + "_" + QLocale::system().name(),
                     "/usr/share/" + QApplication::applicationName() + "/locale")) {
        QApplication::installTranslator(&appTran);
    }

    QCommandLineParser parser;
    parser.setApplicationDescription(QObject::tr(
        "Program that uses underlying git functionality to provide file snapshoting/restoring in a simplified "
        "graphical interface.\n\nIf you need more options, use git directly or other git GUI programs."));
    parser.addHelpOption();
    parser.addVersionOption();
    parser.addPositionalArgument(("<dir>"), QObject::tr("Starting path you want this app to display"));
    parser.process(app);

    // if (getuid() != 0) {
    MainWindow w(parser);
    w.show();
    return QApplication::exec();
    //    } else {
    //        QApplication::beep();
    //        QMessageBox::critical(nullptr, QString(), QObject::tr("You must run this program as normal user."));
    //        return EXIT_FAILURE;
    //    }
}
