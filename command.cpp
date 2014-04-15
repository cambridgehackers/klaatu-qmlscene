#include "command.h"

Command *Command::instance()
{
    static Command *_s_Command = 0;
    if (!_s_Command)
        _s_Command = new Command;
    return _s_Command;
}

Command::Command () :
    m_process(new QProcess(this))
{
}

Command::~Command()
{
}

void Command::start (const QString &program)
{
    m_process->start (program);
}
