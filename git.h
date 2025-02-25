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
    [[nodiscard]] QString createBackupBranch();
    [[nodiscard]] QString getEmailGit();
    [[nodiscard]] QString getUserGit();
    [[nodiscard]] QString resetToCommit(const QString &commit);
    [[nodiscard]] QStringList getStatus(const QString &commit);
    [[nodiscard]] bool hasModifiedFiles();
    [[nodiscard]] static bool needElevation();
    [[nodiscard]] QStringList listCommits();
    void add(const QStringList &files);
    void commit(const QStringList &files, const QString &message);
    void popStash();
    void rebaseToPrevious(const QString &commit);
    void revertFiles(const QString &commit, const QStringList &files);
    void setEmailGit(const QString &email);
    void setUserGit(const QString &name);
    void stash(const QStringList &files = QStringList());

private:
    Cmd cmd;

    [[nodiscard]] QString getCurrentBranch();
    [[nodiscard]] bool initialize();
    [[nodiscard]] static bool isInitialized();
    [[nodiscard]] bool isLargeDirectory();
};

#endif // GIT_H
