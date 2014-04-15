#ifndef COMMAND_H
#define COMMAND_H

#include <QObject>
#include <QProcess>

class Command : public QObject
{
    Q_OBJECT
public:
    static Command *instance();
    virtual ~Command();
    Q_INVOKABLE void start(const QString &program);

private:
    Command();
    QProcess *m_process;
};

#endif // COMMAND_H
