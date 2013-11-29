#define _POSIX_SOURCE
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <iostream>
#include <stdio.h>
#include <errno.h>
#include <map>
#include "queue.h"

namespace
{

const int nofSubProcesses = 2;

class Process
{
public:
    std::string m_name;
    pid_t   m_pid;
};

void signalHandler(int sigNum, siginfo_t *sigInfo, void *unused);

struct Admin
{
    Process m_process[nofSubProcesses];
    bool m_terminate;
    void startProcess(int i, const char *name, int renice = 0)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            if (renice != 0)
                nice(renice);
            std::cout << "launching " << name << " " << getpid() << std::endl;
#if 1
            execl(name, name, 0);
#else
            int s = 2 + (getpid() % 4);
            std::cout << "sleep " << s << std::endl;
            sleep(s);
#endif
            std::cout << "execl " << name << ": "
                << strerror(errno) << "\n";
            exit(1);
        }
        m_process[i].m_name = std::string(name);
        m_process[i].m_pid = pid;
    }
    size_t nofProcesses()
    {
        size_t n = 0;
        for (int i=0; i<nofSubProcesses; i++)
            if (m_process[i].m_pid != -1)
                n++;
        return n;
    }
    void dropProcess(pid_t pid)
    {
        for (int i=0; i<nofSubProcesses; i++)
            if (m_process[i].m_pid == pid)
            {
                m_process[i].m_pid = -1;
                break;
            }
    }
    void killAll()
    {
        for (int i=0; i<nofSubProcesses; i++)
        {
            pid_t pid = m_process[i].m_pid;
            if (pid != -1)
            {
                std::cout << "sending SIGTERM to "
                    << m_process[i].m_name <<
                    " (" << pid << ")\n";
                kill(pid, SIGTERM);
            }
        }
    }
    Admin(): m_terminate(false)
    {
        for (int i=0; i<nofSubProcesses; i++)
            m_process[i].m_pid = -1;

        struct sigaction act;
        act.sa_sigaction = signalHandler;
        sigemptyset(&act.sa_mask);
        act.sa_flags = SA_SIGINFO;
        if (sigaction(SIGCHLD, &act, 0) < 0)
            perror("sigaction SIGCHLD");
        if (sigaction(SIGTERM, &act, 0) < 0)
            perror("sigaction SIGTERM");
        if (sigaction(SIGINT, &act, 0) < 0)
            perror("sigaction SIGINT");
    }
};

Admin admin;

void signalHandler(int sigNum, siginfo_t *sigInfo, void *unused)
{
    (void)unused;
    switch (sigNum)
    {
        case SIGCHLD:
            admin.dropProcess(sigInfo->si_pid);
            waitpid(-1, 0, WNOHANG);
            break;
        case SIGTERM:
        case SIGINT:
            admin.m_terminate = true;
            break;
        default:
            ;
    }
}

} // anonymous namespace

int main(void)
{
    Queue q;
    q.create();
    admin.startProcess(0, "./patcher_core");
    admin.startProcess(1, "./curses_client", 10);
    std::cout << "waiting\n";
    for (;;)
    {
        if (admin.nofProcesses() != nofSubProcesses)
            break;
        if (admin.m_terminate)
            break;
        pause();
        std::cout << "caught signal\n";
    }
    admin.killAll();
    q.unlink();
    std::cout << "exit admin\n";
    return 0;
}
