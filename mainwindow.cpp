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
      git(new Git)
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
    delete ui;
}

void MainWindow::centerWindow()
{
    QRect screenGeometry = QApplication::primaryScreen()->geometry();
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
    ui->pushRestore->setText(tr("Restore entire snapshot"));
    ui->pushSnapshot->setText(tr("Create snapshot for entire directory"));
}

void MainWindow::editCurrent_done()
{
    const QString &new_dir = ui->editCurrentDir->text();
    if (QFileInfo::exists(new_dir) && new_dir != currentDir.path()) {
        currentDir.setPath(new_dir);
        emit dirChanged();
    } else {
        ui->editCurrentDir->setText(currentDir.path());
    }
}

void MainWindow::createSnapshot()
{
    bool ok {};
    QString message
        = QInputDialog::getText(nullptr, "New snapshot", "Enter a snapshot label:", QLineEdit::Normal, "", &ok);

    if (ok && !message.isEmpty()) {
        if (ui->pushSnapshot->text() == tr("Create snapshot for entire directory")) {
            git->commit(listSelectedFiles(), message);
        }
        listSnapshots();
    }
}

void MainWindow::onDirChanged()
{
    ui->listChanges->clear();
    ui->pushRestore->setDisabled(true);
    ui->pushRestore->setText(tr("Restore entire snapshot"));
    ui->pushSnapshot->setText(tr("Create snapshot for entire directory"));
    QDir::setCurrent(currentDir.path());
    ui->editCurrentDir->setText(currentDir.path());
    ui->pushUp->setDisabled(currentDir.path() == "/");
    ui->pushBack->setDisabled(history.isEmpty());
    ui->pushForward->setDisabled(backHistory.isEmpty());
    history.push(currentDir.path());
    listSnapshots();
}

void MainWindow::setConnections()
{
    connect(this, &MainWindow::dirChanged, this, &MainWindow::onDirChanged);
    connect(ui->editCurrentDir, &QLineEdit::editingFinished, this, &MainWindow::editCurrent_done);
    connect(ui->listChanges, &QListWidget::customContextMenuRequested, this, &MainWindow::contextMenuChanges);
    connect(ui->listSnapshots, &QListWidget::itemSelectionChanged, this, &MainWindow::snapshotSelection_changed);
    connect(ui->pushAbout, &QPushButton::clicked, this, &MainWindow::pushAbout_clicked);
    connect(ui->pushBack, &QPushButton::clicked, this, &MainWindow::pushBack_clicked);
    connect(ui->pushCD, &QPushButton::clicked, this, &MainWindow::pushCD_clicked);
    connect(ui->pushCancel, &QPushButton::pressed, this, &MainWindow::close);
    connect(ui->pushForward, &QPushButton::clicked, this, &MainWindow::pushForward_clicked);
    connect(ui->pushHelp, &QPushButton::clicked, this, &MainWindow::pushHelp_clicked);
    connect(ui->pushRefresh, &QPushButton::clicked, this, &MainWindow::pushRefresh_clicked);
    connect(ui->pushRestore, &QPushButton::clicked, this, &MainWindow::restoreSnapshot);
    connect(ui->pushSnapshot, &QPushButton::clicked, this, &MainWindow::createSnapshot);
    connect(ui->pushUp, &QPushButton::clicked, this, &MainWindow::pushUp_clicked);
    ui->listChanges->setContextMenuPolicy(Qt::CustomContextMenu);
}

void MainWindow::showDiff()
{
    auto *item = qobject_cast<QCheckBox *>(ui->listChanges->itemWidget(ui->listChanges->currentItem()));
    if (item == nullptr) {
        return;
    }
    QString file = item->text().section('\t', 1);

    QDialog dialog(this);
    dialog.setWindowTitle(file);
    auto *layout = new QVBoxLayout(&dialog);
    auto *textEdit = new QPlainTextEdit(&dialog);
    textEdit->setReadOnly(true);
    layout->addWidget(textEdit);
    dialog.resize(800, 600);
    dialog.setLayout(layout);

    const QString &commit = ui->listSnapshots->currentItem()->data(Qt::UserRole).toString();
    QProcess proc;
    proc.start("git", {"diff", commit, file});
    proc.waitForFinished();
    textEdit->setPlainText(QString::fromUtf8(proc.readAllStandardOutput()));

    QFont font(QStringLiteral("monospace"));
    font.setStyleHint(QFont::Monospace);
    dialog.setFont(font);

    QTextCharFormat addedFormat;
    QTextCharFormat locationFormat;
    QTextCharFormat removedFormat;

    addedFormat.setForeground(Qt::darkGreen);
    addedFormat.setFontWeight(QFont::Bold);

    locationFormat.setForeground(QColor(35, 140, 216));
    locationFormat.setFontWeight(QFont::Bold);

    removedFormat.setForeground(QColor(187, 15, 30));
    removedFormat.setFontWeight(QFont::Bold);

    QTextCursor cursor(textEdit->document());

    cursor.setPosition(0);
    while (!cursor.atEnd()) {
        if (cursor.block().text().startsWith("@@")) {
            while (!cursor.atBlockEnd()) {
                cursor.movePosition(QTextCursor::NextWord, QTextCursor::KeepAnchor);
                QString selectedWord = cursor.selectedText();
                if (selectedWord.contains(QRegularExpression("@@.*@@"))) {
                    break;
                }
            }
            cursor.mergeCharFormat(locationFormat);
        } else if (cursor.block().text().startsWith('+')) {
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(addedFormat);
        } else if (cursor.block().text().startsWith('-')) {
            cursor.movePosition(QTextCursor::EndOfLine, QTextCursor::KeepAnchor);
            cursor.mergeCharFormat(removedFormat);
        }
        cursor.movePosition(QTextCursor::NextBlock, QTextCursor::MoveAnchor);
    }
    cursor.setPosition(0);
    dialog.exec();
    dialog.adjustSize();
}

void MainWindow::snapshotSelection_changed()
{
    ui->pushRestore->setText(tr("Restore entire snapshot"));
    ui->pushSnapshot->setText(tr("Create snapshot for entire directory"));
    auto selectedItems = ui->listSnapshots->selectedItems();
    if (!selectedItems.isEmpty() && selectedItems.at(0)->text() != tr("No snapshots")) {
        ui->listChanges->clear();
        auto list = git->getStatus(selectedItems.at(0)->data(Qt::UserRole).toString());
        if (!list.isEmpty()) {
            displayChanges(list);
        }
        // ui->pushSnapshot->setDisabled(ui->listChanges->count() == 0);
    }
    ui->pushRestore->setDisabled(ui->listChanges->count() == 0
                                 || ui->listChanges->item(0)->text() == tr("*** No changes from latest snapshot ***"));
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
        QStringList parts = item.split('|');
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
            if (check->checkState() == Qt::Checked) {
                ui->pushSnapshot->setText(tr("Create snapshot for selected files"));
                ui->pushRestore->setText(tr("Restore selected files"));
                return;
            }
            if (anyFileSelected()) {
                ui->pushRestore->setText(tr("Restore selected files"));
                ui->pushSnapshot->setText(tr("Create snapshot for selected files"));
            } else {
                ui->pushRestore->setText(tr("Restore entire snapshot"));
                ui->pushSnapshot->setText(tr("Create snapshot for entire directory"));
            }
        });
    }
    if (ui->listChanges->count() == 0) {
        ui->listChanges->addItem(tr("*** No changes from latest snapshot ***"));
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

void MainWindow::listSnapshots()
{
    ui->listSnapshots->clear();
    ui->listChanges->clear();
    QStringList list = git->listCommits();
    QVector<QPair<QString, QString>> pairList = splitLog(list);
    if (!list.isEmpty()) {
        for (const QPair<QString, QString> &pair : pairList) {
            auto *item = new QListWidgetItem(pair.second);
            item->setData(Qt::UserRole, pair.first);
            ui->listSnapshots->addItem(item);
        }
    } else {
        ui->listSnapshots->insertItem(0, tr("No snapshots"));
        ui->pushRestore->setDisabled(true);
        ui->pushRestore->setText(tr("Restore entire snapshot"));
        ui->pushSnapshot->setText(tr("Create snapshot for entire directory"));
    }
    ui->listSnapshots->setCurrentRow(0);
    if (!git->hasModifiedFiles()) {
        ui->pushSnapshot->setDisabled(true);
        ui->pushSnapshot->setToolTip(
            tr("Most current snapshot is up-to-date, there's no need to create another snapshot"));
    } else {
        ui->pushSnapshot->setEnabled(true);
        ui->pushSnapshot->setToolTip("");
    }
    snapshotSelection_changed();
}

void MainWindow::contextMenuChanges(QPoint pos)
{
    QListWidgetItem *selectedItem = ui->listChanges->itemAt(pos);
    if (selectedItem != nullptr) {
        QMenu contextMenu(this);
        QAction *actionDiff = contextMenu.addAction("Show diff from selected snapshot to current version");
        connect(actionDiff, &QAction::triggered, this, &MainWindow::showDiff);
        contextMenu.exec(ui->listChanges->mapToGlobal(pos));
    }
}

void MainWindow::pushAbout_clicked()
{
    this->hide();
    displayAboutMsgBox(
        tr("About %1") + tr("Restore GUI"),
        R"(<p align="center"><b><h2>Restore GUI</h2></b></p><p align="center">)" + tr("Version: ")
            + QApplication::applicationVersion() + "</p><p align=\"center\"><h3>"
            + tr("Program that uses underlying git functionality to provide file snapshoting/restoring in a "
                 "simplified graphical interface.")
            + R"(</h3></p><p align="center"><a href="http://mxlinux.org">http://mxlinux.org</a><br /></p><p align="center">)"
            + tr("Copyright (c) MX Linux") + "<br /><br /></p>",
        QStringLiteral("/usr/share/doc/restore-gui/license.html"), tr("%1 License").arg(this->windowTitle()));

    this->show();
}

void MainWindow::pushCD_clicked()
{
    const QString &selected
        = QFileDialog::getExistingDirectory(this, tr("Select Directory"), QString(), QFileDialog::ShowDirsOnly);
    if (!selected.isEmpty() && QFileInfo::exists(selected)) {
        currentDir.setPath(selected);
        emit dirChanged();
    }
}

// Help button clicked
void MainWindow::pushHelp_clicked()
{
    const QString &url = QStringLiteral("google.com");
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
    if (QMessageBox::Yes
        == QMessageBox::question(this, tr("Confirmation"),
                                 tr("Do you want to revert the current changes? Any changes that were not "
                                    "snapshotted will be lost."))) {
        if (ui->listSnapshots->currentRow() == 0) { // if HEAD, ask and then stash files

            git->stash(listSelectedFiles());
            listSnapshots();
            QMessageBox::information(
                this, tr("Success"),
                tr("You restored the original version of the files. Unsnapshotted changes are 'stashed', preserved "
                   "with a 'git stash' command, if you need to recover those changes see 'git stash --help'"));

        } else if (ui->pushRestore->text() == tr("Restore entire snapshot")) {
            QString backup = git->resetToCommit(ui->listSnapshots->currentItem()->data(Qt::UserRole).toString());
            QMessageBox::information(
                this, tr("Success"),
                tr("You restored a previous snapshot, all the subsequent snapshots were backed up to "
                   "a git branch named %1")
                    .arg(backup));
        } else if (ui->pushRestore->text() == tr("Restore selected files")) {
            git->revertFiles(ui->listSnapshots->currentItem()->data(Qt::UserRole).toString(), listSelectedFiles());
            QMessageBox::information(
                this, tr("Success"),
                tr("You restored the selected version of the files. Unsnapshotted changes are 'stashed', preserved "
                   "with a 'git stash' command, if you need to recover those changes see 'git stash --help'"));
        }
        listSnapshots();
    }
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
