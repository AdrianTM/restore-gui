#include "cmd.h"

#include <QDebug>
#include <QEventLoop>

Cmd::Cmd(QObject *parent)
    : QProcess(parent)
{
    connect(this, &Cmd::readyReadStandardOutput, [this] { emit outputAvailable(readAllStandardOutput()); });
    connect(this, &Cmd::readyReadStandardError, [this] { emit errorAvailable(readAllStandardError()); });
    connect(this, &Cmd::outputAvailable, [this](const QString &out) { out_buffer += out; });
    connect(this, &Cmd::errorAvailable, [this](const QString &out) { out_buffer += out; });
}

bool Cmd::run(const QString &cmd, bool quiet, bool elevate)
{
    QString output;
    return run(cmd, &output, quiet, elevate);
}

QString Cmd::getCmdOut(const QString &cmd, bool quiet, bool elevate)
{
    QString output;
    run(cmd, &output, quiet, elevate);
    return output;
}

bool Cmd::run(const QString &cmd, QString *output, bool quiet, bool elevate)
{
    out_buffer.clear();
    connect(this, QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished), this, &Cmd::finished);
    if (this->state() != QProcess::NotRunning) {
        qDebug() << "Process already running:" << this->program() << this->arguments();
        return false;
    }
    if (!quiet)
        qDebug().noquote() << cmd;
    QEventLoop loop;
    connect(this, &Cmd::finished, &loop, &QEventLoop::quit);
    if (elevate)
        start("pkexec", {"/bin/bash", "-c", cmd});
    else
        start("/bin/bash", {"-c", cmd});
    loop.exec();
    *output = out_buffer.trimmed();
    return (exitStatus() == QProcess::NormalExit && exitCode() == 0);
}
