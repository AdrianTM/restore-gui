/**********************************************************************
 *  mainwindow.cpp
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
#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QAction>
#include <QCheckBox>
#include <QDebug>
#include <QDirIterator>
#include <QFileDialog>
#include <QInputDialog>
#include <QMenu>
#include <QPair>
#include <QPlainTextEdit>
#include <QScreen>
#include <QScrollBar>
#include <QTextBlock>
#include <QTextStream>

#include "about.h"

MainWindow::MainWindow(const QCommandLineParser &arg_parser, QWidget *parent)
    : QDialog(parent),
      ui(new Ui::MainWindow),
      git(new Git(this))
{
    ui->setupUi(this);
    const auto &arg_list = arg_parser.positionalArguments();
    if (!arg_list.empty()) {
        if (QFileInfo::exists(arg_list.first())) {
            currentDir.setPath(arg_list.first());
        }
    }
    setWindowFlags(Qt::Window); // for the close, min and max buttons

    const QSize size = this->size();
    if (settings.contains(QStringLiteral("geometry"))) {
        restoreGeometry(settings.value(QStringLiteral("geometry")).toByteArray());
        if (this->isMaximized()) { // add option to resize if maximized
            this->resize(size);
            centerWindow();
        }
    }

    setConnections();
    setup();
}

MainWindow::~MainWindow()
{
    settings.setValue(QStringLiteral("geometry"), saveGeometry());
    delete git;
    delete ui;
}

void MainWindow::centerWindow()
{
    const QRect screenGeometry = QApplication::primaryScreen()->geometry();
    const int x = (screenGeometry.width() - this->width()) / 2;
    const int y = (screenGeometry.height() - this->height()) / 2;
    this->move(x, y);
}

void MainWindow::setup()
{
    this->adjustSize();
    QFont font(QStringLiteral("monospace"));
    font.setStyleHint(QFont::Monospace);
    ui->listChanges->setFont(font);
    ui->editCurrentDir->setText(currentDir.path());
    onDirChanged();
    ui->editCurrentDir->setFocus();
    ui->pushCancel->setEnabled(true);
    ui->pushUp->setIcon(QIcon::fromTheme("go-up-symbolic", QIcon(":/icons/images/go-up-symbolic.svg")));
    ui->pushBack->setIcon(QIcon::fromTheme("go-previous-symbolic", QIcon(":/icons/images/go-previous-symbolic.svg")));
    ui->pushForward->setIcon(QIcon::fromTheme("go-next-symbolic", QIcon(":/icons/images/go-next-symbolic.svg")));
    ui->pushRefresh->setIcon(
        QIcon::fromTheme("view-refresh-symbolic", QIcon(":/icons/images/view-refresh-symbolic.svg")));
    ui->pushBack->setDisabled(true);
    ui->pushForward->setDisabled(true);
    ui->pushRestore->setText(tr("Restore to selected checkpoint"));
    ui->pushSnapshot->setText(tr("Create checkpoint for entire directory"));
}

void MainWindow::editCurrent_done()
{
    const QString new_dir = ui->editCurrentDir->text();
    if (QFileInfo::exists(new_dir) && new_dir != currentDir.path()) {
        currentDir.setPath(new_dir);
        emit dirChanged();
    } else {
        ui->editCurrentDir->setText(currentDir.path());
    }
}

void MainWindow::createSnapshot()
{
    bool isOk {};
    const QString message = QInputDialog::getText(this, tr("New checkpoint"), tr("Enter a checkpoint label:"),
                                                  QLineEdit::Normal, QString(), &isOk);

    if (isOk && !message.isEmpty()) {
        if (ui->pushSnapshot->text() == tr("Create checkpoint for entire directory")) {
            git->commit(listSelectedFiles(), message);
        }
        listCheckpoints();
    }
}

void MainWindow::onDirChanged()
{
    // Reset UI state
    ui->listChanges->clear();
    ui->pushRestore->setDisabled(true);
    ui->pushRestore->setText(tr("Restore to selected checkpoint"));
    ui->pushSnapshot->setText(tr("Create checkpoint for entire directory"));

    // Update current directory
    QDir::setCurrent(currentDir.path());
    ui->editCurrentDir->setText(currentDir.path());

    // Update navigation buttons
    ui->pushUp->setDisabled(currentDir.path() == "/");
    ui->pushBack->setDisabled(history.isEmpty());
    ui->pushForward->setDisabled(backHistory.isEmpty());

    history.push(currentDir.path());
    listCheckpoints();
}

void MainWindow::setConnections()
{
    // Directory and file operations
    connect(this, &MainWindow::dirChanged, this, &MainWindow::onDirChanged);
    connect(ui->editCurrentDir, &QLineEdit::editingFinished, this, &MainWindow::editCurrent_done);
    connect(ui->listChanges, &QListWidget::customContextMenuRequested, this, &MainWindow::contextMenuChanges);
    connect(ui->listCheckpoints, &QListWidget::itemSelectionChanged, this, &MainWindow::checkpointSelection_changed);

    // Button clicks
    connect(ui->pushAbout, &QPushButton::clicked, this, &MainWindow::pushAbout_clicked);
    connect(ui->pushBack, &QPushButton::clicked, this, &MainWindow::pushBack_clicked);
    connect(ui->pushCD, &QPushButton::clicked, this, &MainWindow::pushCD_clicked);
    connect(ui->pushCancel, &QPushButton::pressed, this, &MainWindow::close);
    connect(ui->pushDiff, &QPushButton::pressed, this, &MainWindow::pushDiff_clicked);
    connect(ui->pushForward, &QPushButton::clicked, this, &MainWindow::pushForward_clicked);
    connect(ui->pushHelp, &QPushButton::clicked, this, &MainWindow::pushHelp_clicked);
    connect(ui->pushRefresh, &QPushButton::clicked, this, &MainWindow::pushRefresh_clicked);
    connect(ui->pushRestore, &QPushButton::clicked, this, &MainWindow::restoreSnapshot);
    connect(ui->pushSnapshot, &QPushButton::clicked, this, &MainWindow::createSnapshot);
    connect(ui->pushUp, &QPushButton::clicked, this, &MainWindow::pushUp_clicked);

    // Context menu policy
    ui->listChanges->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::showDiff()
{
    auto *item = qobject_cast<QCheckBox *>(ui->listChanges->itemWidget(ui->listChanges->currentItem()));
    if (!ui->listCheckpoints->currentItem()) {
        return;
    }
    const QString file = item != nullptr ? item->text().section('\t', 1) : QString();

    QDialog dialog(this);
    dialog.setWindowTitle(file.isEmpty() ? tr("Current ..") + ui->listCheckpoints->currentItem()->text() : file);

    auto *layout = new QVBoxLayout(&dialog);
    auto *textEdit = new QPlainTextEdit(&dialog);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);
    dialog.resize(800, 600);
    dialog.setLayout(layout);

    const QString commit = ui->listCheckpoints->currentItem()->data(Qt::UserRole).toString();
    QProcess proc;
    if (file.isEmpty()) {
        proc.start("git", {"diff", commit});
    } else {
        proc.start("git", {"diff", commit, file});
    }
    proc.waitForFinished();
    textEdit->setPlainText(QString::fromUtf8(proc.readAllStandardOutput()));

    QFont font(QStringLiteral("monospace"));
    font.setStyleHint(QFont::Monospace);
    dialog.setFont(font);

    // Define text formats for diff highlighting
    QTextCharFormat addedFormat;
    addedFormat.setForeground(Qt::darkGreen);
    addedFormat.setFontWeight(QFont::Bold);

    QTextCharFormat locationFormat;
    locationFormat.setForeground(QColor(35, 140, 216));
    locationFormat.setFontWeight(QFont::Bold);

    QTextCharFormat removedFormat;
    removedFormat.setForeground(QColor(187, 15, 30));
    removedFormat.setFontWeight(QFont::Bold);

    // Apply syntax highlighting
    QTextCursor cursor(textEdit->document());
    cursor.setPosition(0);
    while (!cursor.atEnd()) {
        const QString lineText = cursor.block().text();
        if (lineText.startsWith("@@")) {
            while (!cursor.atBlockEnd()) {
                cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
                const QString selectedWord = cursor.selectedText();
                if (selectedWord.contains(QRegularExpression("@@.*@@"))) {
                    break;
                }
            }
            cursor.mergeCharFormat(locationFormat);
        } else if (lineText.startsWith('+')) {
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(addedFormat);
        } else if (lineText.startsWith('-')) {
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(removedFormat);
        }
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
    }

    cursor.setPosition(0);
    dialog.exec();
    dialog.adjustSize();
}

void MainWindow::checkpointSelection_changed()
{
    ui->pushRestore->setText(tr("Restore to selected checkpoint"));
    ui->pushSnapshot->setText(tr("Create checkpoint for entire directory"));

    const auto selectedItems = ui->listCheckpoints->selectedItems();
    if (!selectedItems.isEmpty() && selectedItems.at(0)->text() != tr("No checkpoints")) {
        ui->listChanges->clear();
        const auto list = git->getStatus(selectedItems.at(0)->data(Qt::UserRole).toString());
        if (!list.isEmpty()) {
            displayChanges(list);
        }
    }

    const bool noChanges = ui->listChanges->count() == 0
                           || ui->listChanges->item(0)->text() == tr("*** No changes from latest checkpoint ***");

    ui->pushRestore->setDisabled(noChanges);
    ui->pushDiff->setDisabled(noChanges);
}

bool MainWindow::anyFileSelected()
{
    for (int row = 0; row < ui->listChanges->count(); ++row) {
        auto *check = qobject_cast<QCheckBox *>(ui->listChanges->itemWidget(ui->listChanges->item(row)));
        if (check != nullptr && check->checkState() == Qt::Checked) {
            return true;
        }
    }
    return false;
}

QVector<QPair<QString, QString>> MainWindow::splitLog(const QStringList &log)
{
    QVector<QPair<QString, QString>> result;
    result.reserve(log.size());

    for (const QString &item : log) {
        const QStringList parts = item.split('|');
        if (parts.size() >= 2) {
            result.append(qMakePair(parts.at(0), parts.at(1)));
        }
    }
    return result;
}

void MainWindow::displayChanges(const QStringList &list)
{
    ui->listChanges->clear();
    for (const auto &file : list) {
        if (file.isEmpty()) {
            continue;
        }
        auto *item = new QListWidgetItem(ui->listChanges);
        auto *check = new QCheckBox(file);
        ui->listChanges->setItemWidget(item, check);

        connect(check, &QCheckBox::clicked, [this, check] {
            const bool isChecked = check->checkState() == Qt::Checked;
            const bool hasSelected = isChecked || anyFileSelected();

            ui->pushSnapshot->setText(hasSelected ? tr("Create checkpoint for selected files")
                                                  : tr("Create checkpoint for entire directory"));
            ui->pushRestore->setText(hasSelected ? tr("Restore selected files") : tr("Restore to selected checkpoint"));
        });
    }

    if (ui->listChanges->count() == 0) {
        ui->listChanges->addItem(tr("*** No changes from latest checkpoint ***"));
        ui->pushDiff->setDisabled(true);
    }
}

QStringList MainWindow::listSelectedFiles()
{
    QStringList selected;
    selected.reserve(ui->listChanges->count());

    for (int row = 0; row < ui->listChanges->count(); ++row) {
        auto *check = qobject_cast<QCheckBox *>(ui->listChanges->itemWidget(ui->listChanges->item(row)));
        if (check != nullptr && check->checkState() == Qt::Checked) {
            selected << check->text().section("\t", 1);
        }
    }
    return selected;
}

void MainWindow::listCheckpoints()
{
    ui->listCheckpoints->clear();
    ui->listChanges->clear();

    const QStringList list = git->listCommits();
    const QVector<QPair<QString, QString>> pairList = splitLog(list);

    if (!list.isEmpty()) {
        for (const QPair<QString, QString> &pair : pairList) {
            auto *item = new QListWidgetItem(pair.second);
            item->setData(Qt::UserRole, pair.first);
            ui->listCheckpoints->addItem(item);
        }
    } else {
        ui->listCheckpoints->insertItem(0, tr("No checkpoints"));
        ui->pushRestore->setDisabled(true);
        ui->pushRestore->setText(tr("Restore to selected checkpoint"));
        ui->pushSnapshot->setText(tr("Create checkpoint for entire directory"));
    }

    ui->listCheckpoints->setCurrentRow(0);

    const bool hasModifiedFiles = git->hasModifiedFiles();
    ui->pushSnapshot->setDisabled(!hasModifiedFiles);
    ui->pushSnapshot->setToolTip(
        hasModifiedFiles ? QString()
                         : tr("No changes since last checkpoint, there's no need to create another checkpoint"));

    checkpointSelection_changed();
}

void MainWindow::contextMenuChanges(QPoint pos)
{
    QListWidgetItem *selectedItem = ui->listChanges->itemAt(pos);
    if (selectedItem == nullptr) {
        return;
    }

    QMenu contextMenu(this);
    QAction *actionDiff = contextMenu.addAction(tr("Show diff from selected checkpoint to current version"));
    connect(actionDiff, &QAction::triggered, this, &MainWindow::showDiff);
    contextMenu.exec(ui->listChanges->mapToGlobal(pos));
}

void MainWindow::pushAbout_clicked()
{
    this->hide();
    displayAboutMsgBox(
        tr("About %1").arg(tr("Restore GUI")),
        R"(<p align="center"><b><h2>Restore GUI</h2></b></p><p align="center">)" + tr("Version: ")
            + QApplication::applicationVersion() + "</p><p align=\"center\"><h3>"
            + tr("Program that uses underlying git functionality to provide file checkpoint/restore in a "
                 "simplified graphical interface.")
            + R"(</h3></p><p align="center"><a href="http://mxlinux.org">http://mxlinux.org</a><br /></p><p align="center">)"
            + tr("Copyright (c) MX Linux") + "<br /><br /></p>",
        QStringLiteral("/usr/share/doc/restore-gui/license.html"), tr("%1 License").arg(this->windowTitle()));
    this->show();
}

void MainWindow::pushCD_clicked()
{
    const QString selected
        = QFileDialog::getExistingDirectory(this, tr("Select Directory"), QString(), QFileDialog::ShowDirsOnly);

    if (!selected.isEmpty() && QFileInfo::exists(selected)) {
        currentDir.setPath(selected);
        emit dirChanged();
    }
}

void MainWindow::pushDiff_clicked()
{
    showDiff();
}

void MainWindow::pushHelp_clicked()
{
    const QString url = QStringLiteral("google.com");
    displayDoc(url, tr("%1 Help").arg(this->windowTitle()));
}

void MainWindow::pushRefresh_clicked()
{
    onDirChanged();
}

void MainWindow::pushUp_clicked()
{
    currentDir.cdUp();
    ui->editCurrentDir->setText(currentDir.path());
    emit dirChanged();
}

void MainWindow::restoreSnapshot()
{
    const auto response = QMessageBox::question(
        this, tr("Confirmation"),
        tr("Do you want to revert the current changes? Any changes that were not in a checkpoint will be lost."));

    if (response != QMessageBox::Yes) {
        return;
    }

    const QString successTitle = tr("Success");
    const QString stashMessage
        = tr("You restored the original version of the files. The files that are not included "
             "in the checkpoint are 'stashed', preserved with a 'git stash' command, if you need "
             "to recover those changes see 'git stash --help'");

    // Handle different restore scenarios
    if (ui->listCheckpoints->currentRow() == 0) {
        // Restore to clean state
        git->stash(listSelectedFiles());
        QMessageBox::information(this, successTitle, stashMessage);
    } else {
        const QString commitId = ui->listCheckpoints->currentItem()->data(Qt::UserRole).toString();

        if (ui->pushRestore->text() == tr("Restore to selected checkpoint")) {
            // Reset entire repo to previous checkpoint
            const QString backup = git->resetToCommit(commitId);
            QMessageBox::information(this, successTitle,
                                     tr("You switched to a previous checkpoint, all newer checkpoints were backed up "
                                        "to a git branch named %1")
                                         .arg(backup));
        } else if (ui->pushRestore->text() == tr("Restore selected files")) {
            // Restore only selected files
            git->revertFiles(commitId, listSelectedFiles());
            QMessageBox::information(this, successTitle, stashMessage);
        }
    }

    listCheckpoints();
}

void MainWindow::pushBack_clicked()
{
    if (!history.isEmpty()) {
        backHistory.push(history.pop());
    }
    if (!history.isEmpty()) {
        ui->editCurrentDir->setText(history.pop());
    }
    editCurrent_done();
}

void MainWindow::pushForward_clicked()
{
    if (!backHistory.isEmpty()) {
        ui->editCurrentDir->setText(backHistory.pop());
        editCurrent_done();
    }
}
