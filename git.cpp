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
    connect(&cmd, &Cmd::started, [] { QApplication::setOverrideCursor(QCursor(Qt::BusyCursor)); });
    connect(&cmd, &Cmd::finished, [] { QApplication::setOverrideCursor(QCursor(Qt::ArrowCursor)); });
}

void Git::add(const QStringList &files)
{
    if (files.size() == 1 && files.at(0) == '.') {
        cmd.run("git add .", false, needElevation());
    } else {
        cmd.run("git add " + files.join(' '), false, needElevation());
    }
};

void Git::commit(const QStringList &files, const QString &message)
{
    const QString &addList = files.isEmpty() ? "." : files.join(' ');
    if (!isInitialized()) {
        if (isLargeDirectory()) {
            if (QMessageBox::No
                == QMessageBox::question(nullptr, tr("Confirmation"),
                                         tr("You are trying to snapshot a large folder, are you sure? If you select "
                                            "'Yes' it might take a long time to process."))) {
                return;
            }
        }
        cmd.run("cd " + QDir::currentPath() + " && git init && git add " + addList + " && git commit -m '" + message
                    + "'",
                false, needElevation());
    } else {
        cmd.run("cd " + QDir::currentPath() + " && git add " + addList + " && git commit -m '" + message + "'", false,
                needElevation());
    }
}

void Git::stash(const QStringList &files)
{
    if (files.isEmpty()) {
        cmd.run("cd " + QDir::currentPath() + " && git stash", false, needElevation());
    } else {
        cmd.run("cd " + QDir::currentPath() + " && git stash push " + files.join(' ')
                    + " -m 'stash created by GUI program'",
                false, needElevation());
    }
}

// Stash, branch, and reset
QString Git::resetToCommit(const QString &commit)
{
    const QString &name = "bak_" + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    if (commit.isEmpty()) {
        return {};
    }
    cmd.run("cd " + QDir::currentPath() + " && git stash && git branch " + name + " && git reset --hard " + commit,
            false, needElevation());
    return name;
}

// Stash and revert
void Git::revertFiles(const QString &commit, const QStringList &files)
{
    if (files.isEmpty() || commit.isEmpty()) {
        return;
    }
    cmd.run("cd " + QDir::currentPath() + " && git stash && git checkout " + commit + " -- " + files.join(' ')
                + " && git commit -m 'Restored files: " + files.join(' ') + "'",
            false, needElevation());
}

QString Git::createBackupBranch()
{
    const QString &name = "bak_" + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd_HHmmss"));
    cmd.run("cd " + QDir::currentPath() + " && git branch " + name, false, needElevation());
    return name;
}

QStringList Git::getStatus(const QString &commit)
{
    return cmd.getCmdOut("git diff --name-status " + commit).split('\n');
}

QStringList Git::listCommits()
{
    if (!isInitialized()) {
        return {};
    }
    return cmd.getCmdOut("git log --pretty=format:'%h|%cr - %s'").split('\n');
}

bool Git::hasModifiedFiles()
{
    if (!isInitialized()) {
        return true;
    }
    return !cmd.getCmdOut("git status --porcelain").isEmpty();
}

bool Git::initialize()
{
    return cmd.run("cd " + QDir::currentPath() + "&& git init", false, needElevation());
}

bool Git::isInitialized()
{
    return QProcess::execute("git", {"rev-parse", "--is-inside-work-tree"}) == 0;
}

bool Git::needElevation()
{
    return (!QFileInfo(QDir::currentPath() + "/.").isWritable());
}

QString Git::getCurrentBranch()
{
    return cmd.getCmdOut("git branch --show-current");
}

// Try to guess if the directory has a lot of file in a quick way
bool Git::isLargeDirectory()
{
    return cmd.getCmdOut("find " + QDir::currentPath() + " -maxdepth 3 2>/dev/null | wc -l").toUInt() > 500;
}
