#include "applogger.h"

#include <QCoreApplication>
#include <QDateTime>
#include <QDir>
#include <QFile>
#include <QMutex>
#include <QMutexLocker>
#include <QStandardPaths>
#include <QThread>
#include <QtGlobal>
#include <cstdlib>

namespace {
QMutex s_logMutex;
QString s_logPath;
QtMessageHandler s_previousHandler = nullptr;
constexpr qint64 MaxLogBytes = 5 * 1024 * 1024;

void writeLog(QtMsgType type, const QMessageLogContext &context, const QString &message)
{
    const char *level = "INFO";
    switch (type) {
    case QtDebugMsg: level = "DEBUG"; break;
    case QtInfoMsg: level = "INFO"; break;
    case QtWarningMsg: level = "WARN"; break;
    case QtCriticalMsg: level = "ERROR"; break;
    case QtFatalMsg: level = "FATAL"; break;
    }

    const QString line = QStringLiteral("%1 [%2] [%3] %4 (%5:%6)\n")
        .arg(QDateTime::currentDateTime().toString(Qt::ISODateWithMs),
             QString::fromLatin1(level),
             QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()), 16),
             message,
             QString::fromUtf8(context.file ? context.file : ""),
             QString::number(context.line));

    {
        QMutexLocker locker(&s_logMutex);
        QFile file(s_logPath);
        if (file.exists() && file.size() >= MaxLogBytes) {
            QFile::remove(s_logPath + QStringLiteral(".1"));
            file.rename(s_logPath + QStringLiteral(".1"));
        }
        if (file.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            file.write(line.toUtf8());
            file.close();
        }
    }

    if (s_previousHandler)
        s_previousHandler(type, context, message);
    if (type == QtFatalMsg)
        abort();
}
}

void AppLogger::install()
{
    const QString root = QStandardPaths::writableLocation(QStandardPaths::AppLocalDataLocation);
    QDir().mkpath(root + QStringLiteral("/logs"));
    s_logPath = root + QStringLiteral("/logs/monitoring.log");
    s_previousHandler = qInstallMessageHandler(writeLog);
    qInfo() << "日志文件:" << s_logPath;
}

QString AppLogger::logFilePath()
{
    return s_logPath;
}
