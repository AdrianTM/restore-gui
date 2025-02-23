#include "cmd.h"

#include <QDebug>
#include <QEventLoop>

Cmd::Cmd(QObject *parent)
    : QProcess(parent)
{
    connect(this, &Cmd::readyReadStandardOutput, this, &Cmd::handleStandardOutput);
    connect(this, &Cmd::readyReadStandardError, this, &Cmd::handleStandardError);
}

void Cmd::handleStandardOutput()
{
    const QString output = readAllStandardOutput();
    emit outputAvailable(output);
    out_buffer += output;
}

void Cmd::handleStandardError()
{
    const QString error = readAllStandardError();
    emit errorAvailable(error);
    out_buffer += error;
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
        qWarning() << "Process already running:" << this->program() << this->arguments();
        return false;
    }

    if (!quiet) {
        qDebug().noquote() << cmd;
    }

    QEventLoop loop;
    connect(this, &Cmd::finished, &loop, &QEventLoop::quit);

    const QString program = elevate ? "pkexec" : "bash";
    const QStringList args = elevate ? QStringList {"bash", "-c", cmd} : QStringList {"-c", cmd};
    start(program, args);
    loop.exec();
    disconnect(this, nullptr, &loop, nullptr);

    *output = out_buffer.trimmed();

    if (exitStatus() != QProcess::NormalExit) {
        qWarning() << "Process exited abnormally";
        return false;
    }

    if (exitCode() != 0) {
        qWarning() << "Exit code:" << exitCode();
        return false;
    }

    return true;
}
