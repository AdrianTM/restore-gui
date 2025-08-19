/**********************************************************************
 *  mainwindow.h
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

#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QCommandLineParser>
#include <QDir>
#include <QMessageBox>
#include <QProcess>
#include <QSettings>
#include <QStack>

#include "git.h"

class Git;

namespace Ui
{
class MainWindow;
}

class MainWindow : public QDialog
{
    Q_OBJECT

public:
    explicit MainWindow(const QCommandLineParser &arg_parser, QWidget *parent = nullptr);
    ~MainWindow() override;
    void centerWindow();
    void setup();

private slots:
    void contextMenuChanges(QPoint pos);
    void createSnapshot();
    void editCurrent_done();
    void listCheckpoints();
    void onDirChanged();
    void pushAbout_clicked();
    void pushBack_clicked();
    void pushCD_clicked();
    void pushDelete_clicked();
    void pushDiff_clicked();
    void pushForward_clicked();
    void pushHelp_clicked();
    void pushRefresh_clicked();
    void pushSchedule_clicked();
    void pushUp_clicked();
    void restoreSnapshot();
    void setConnections();
    void showDiff();
    void checkpointSelection_changed();

signals:
    void dirChanged();

private:
    Ui::MainWindow *ui;
    Git *git;
    QProcess proc;
    QSettings settings;
    QStack<QString> history;
    QStack<QString> backHistory;
    QDir currentDir {QDir::current()};

    [[nodiscard]] QStringList listSelectedFiles();
    [[nodiscard]] bool anyFileSelected();
    [[nodiscard]] bool checkGitConfig();
    [[nodiscard]] static QVector<QPair<QString, QString>> splitLog(const QStringList &log);
    void displayChanges(const QStringList &list);
};

#endif
