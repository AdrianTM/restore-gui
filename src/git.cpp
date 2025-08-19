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
#include "git.h"

#include <QApplication>
#include <QDateTime>
#include <QDir>
#include <QMessageBox>

Git::Git(QObject *parent)
    : QObject(parent)
{
    // Set busy cursor during git operations
    connect(&cmd, &Cmd::started, [] { QApplication::setOverrideCursor(QCursor(Qt::BusyCursor)); });
    connect(&cmd, &Cmd::done, [] { QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor)); });
}

void Git::add(const QStringList &files)
{
    // Handle both single file and multiple files cases
    const QString command = (files.size() == 1 && files.at(0) == ".") ? "git add ." : "git add " + files.join(' ');
    cmd.run(command, nullptr, nullptr, false, needElevation());
}

void Git::commit(const QStringList &files, const QString &message)
{
    const QString addList = files.isEmpty() ? "." : files.join(' ');

    if (!isInitialized()) {
        // Warn user before initializing git in large directories
        if (isLargeDirectory()
            && QMessageBox::No
                   == QMessageBox::question(nullptr, tr("Confirmation"),
                                            tr("You are trying to snapshot a large folder, are you sure? If you select "
                                               "'Yes' it might take a long time to process."))) {
            return;
        }
        // Initialize repository and make first commit
        cmd.run("git init && git add " + addList + " && git commit -m '" + message + "'", nullptr, nullptr, false,
                needElevation());
    } else {
        // Regular commit
        cmd.run("git add " + addList + " && git commit -m '" + message + "'", nullptr, nullptr, false, needElevation());
    }
}

void Git::popStash()
{
    cmd.run("git stash pop", nullptr, nullptr, false, needElevation());
}

void Git::rebaseToPrevious(const QString &commit)
{
    if (commit.isEmpty()) {
        return;
    }
    const QString command = QString("git rebase --onto $(git rev-parse %1^) %1").arg(commit);
    cmd.run(command, nullptr, nullptr, false, needElevation());
}

void Git::stash(const QStringList &files)
{
    const QString command
        = files.isEmpty() ? "git stash" : "git stash push " + files.join(' ') + " -m 'stash created by GUI program'";
    cmd.run(command, nullptr, nullptr, false, needElevation());
}

// Stash, branch, and reset
QString Git::resetToCommit(const QString &commit)
{
    const QString &name = "bak_" + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    if (commit.isEmpty()) {
        return {};
    }
    cmd.run("git stash && git branch " + name + " && git reset --hard " + commit, nullptr, nullptr, false,
            needElevation());
    return name;
}

// Stash and revert
void Git::revertFiles(const QString &commit, const QStringList &files)
{
    if (files.isEmpty() || commit.isEmpty()) {
        return;
    }
    QString command = "git stash && git checkout " + commit + " -- " + files.join(' ')
                      + " && git commit -m 'Restored files: " + files.join(' ') + "'";
    cmd.run(command, nullptr, nullptr, false, needElevation());
}

void Git::setEmailGit(const QString &email)
{
    cmd.run("git config --global user.email '" + email + "'");
}

void Git::setUserGit(const QString &name)
{
    cmd.run("git config --global user.name '" + name + "'");
}

QString Git::createBackupBranch()
{
    const QString name = "bak_" + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    cmd.run("git branch " + name, nullptr, nullptr, false, needElevation());
    return name;
}

QString Git::getEmailGit()
{
    return cmd.getOut("git config --global --get user.email 2>/dev/null", true);
}

QString Git::getUserGit()
{
    return cmd.getOut("git config --global --get user.name 2>/dev/null", true);
}

QStringList Git::getStatus(const QString &commit)
{
    // Get modified, added, deleted files from git diff
    QStringList status = cmd.getOut("git diff --name-status " + commit + " 2>/dev/null", true).split('\n');

    // Get untracked files
    QStringList untracked = cmd.getOut("git ls-files --others --exclude-standard 2>/dev/null", true).split('\n');

    // Add untracked files with "??" status prefix (same as git status)
    for (const QString &file : untracked) {
        if (!file.isEmpty()) {
            status.append("??\t" + file);
        }
    }

    return status;
}

QStringList Git::listCommits()
{
    if (!isInitialized()) {
        return {};
    }
    return cmd.getOut("git log --pretty=format:'%h|%cr - %s' 2>/dev/null", true).split('\n');
}

bool Git::hasModifiedFiles()
{
    if (!isInitialized()) {
        return true;
    }
    return !cmd.getOut("git status --porcelain 2>/dev/null", true).isEmpty();
}

bool Git::initialize()
{
    return cmd.run("git init", nullptr, nullptr, false, needElevation());
}

bool Git::isInitialized()
{
    return QProcess::execute("git", {"rev-parse", "--is-inside-work-tree"}) == 0;
}

bool Git::needElevation()
{
    return !QFileInfo(QDir::currentPath() + "/.").isWritable();
}

QString Git::getCurrentBranch()
{
    return cmd.getOut("git branch --show-current");
}

// Try to guess if the directory has a lot of file in a quick way
bool Git::isLargeDirectory()
{
    return cmd.getOut("find " + QDir::currentPath() + " -maxdepth 3 2>/dev/null | wc -l").toUInt() > 500;
}
