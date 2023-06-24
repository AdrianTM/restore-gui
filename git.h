/**********************************************************************
 *
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
#ifndef GIT_H
#define GIT_H

#include <QObject>
#include <QString>

#include "cmd.h"

class Git : public QObject
{
    Q_OBJECT
public:
    explicit Git(QObject *parent = nullptr);
    QString createBackupBranch();
    QString resetToCommit(const QString &commit);
    QStringList getStatus(const QString &commit);
    QStringList listCommits();
    bool hasModifiedFiles();
    static bool needElevation();
    void add(const QStringList &files);
    void commit(const QStringList &files, const QString &message);
    void revertFiles(const QString &commit, const QStringList &files);
    void stash(const QStringList &files);

private:
    Cmd cmd;
    QString getCurrentBranch();
    bool initialize();
    static bool isInitialized();
    bool isLargeDirectory();
};

#endif // GIT_H
