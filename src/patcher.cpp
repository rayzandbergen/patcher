/*! \file patcher.cpp
 *  \brief Contains the administrative process.
 *
 *  Copyright 2013 Raymond Zandbergen (ray.zandbergen@gmail.com)
 */

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

const int maxSubProcesses = 2; //!< Max sub processes managed by this process.

//! \brief A process name and a PID.
class Process
{
public:
    std::string m_name; //!<    Process name.
    pid_t   m_pid;      //!<    PID.
};

//! \brief Handler for all signals.
void signalHandler(int sigNum, siginfo_t *sigInfo, void *unused);

//! \brief Admin process for the core and the curses tasks.
struct Admin
{
    Process m_process[maxSubProcesses]; //!<    Process table.
    bool m_terminate;                   //!<    Raised on SIGTERM and SIGINT.
public:
    //! \brief Check termination flag.
    bool terminate() const { return m_terminate; }
    //! \brief Start a process in slot i.
    void startProcess(int i, const char *name, int renice = 0)
    {
        pid_t pid = fork();
        if (pid == 0)
        {
            if (renice != 0)
                nice(renice);
            std::cout << "launching " << name << " " << getpid() << std::endl;
            execl(name, name, 0);
            std::cout << "execl " << name << ": "
                << strerror(errno) << "\n";
            exit(1);
        }
        m_process[i].m_name = std::string(name);
        m_process[i].m_pid = pid;
    }
    //! \brief The number of managed processes.
    size_t nofProcesses() const
    {
        size_t n = 0;
        for (int i=0; i<maxSubProcesses; i++)
            if (m_process[i].m_pid != -1)
                n++;
        return n;
    }
    //! \brief Drop a process from the process table.
    void dropProcess(pid_t pid)
    {
        for (int i=0; i<maxSubProcesses; i++)
            if (m_process[i].m_pid == pid)
            {
                m_process[i].m_pid = -1;
                break;
            }
    }
    //! \brief Kill every process in the table.
    void killAll()
    {
        for (int i=0; i<maxSubProcesses; i++)
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
    //! \brief Default constructor.
    Admin(): m_terminate(false)
    {
        for (int i=0; i<maxSubProcesses; i++)
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

Admin admin;    //!<    Global Admin object.

//! \brief Handler for all SIGCHLD, SIGTERM and SIGINT.
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

//! \brief Patcher task main entry.
int main(int argc, char **argv)
{
    bool startCursesClient = true;
    for (;;)
    {
        int opt = getopt(argc, argv, "ch:");
        if (opt == -1)
            break;
        switch (opt)
        {
            case 'c':
                startCursesClient = false;
                break;
            default:
                std::cerr << "\npatcher [-h|?] [-d <dir>] [-c]\n\n"
                    "  -h|?     This message\n"
                    "  -c       Do no lauch curses client\n";
                return 1;
                break;
        }
    }
    if (argc > optind)
    {
        std::cerr << "unrecognised trailing arguments, try -h\n";
    }
    Queue q;
    q.create();
    int nofSubProcesses = 1;
    admin.startProcess(0, "./patcher_core");
    if (startCursesClient)
    {
        nofSubProcesses++;
        admin.startProcess(1, "./curses_client", 10);
    }
    std::cout << "waiting\n";
    for (;;)
    {
        if (admin.nofProcesses() != (size_t)nofSubProcesses)
            break;
        if (admin.terminate())
            break;
        pause();
        std::cout << "caught signal\n";
    }
    admin.killAll();
    q.unlink();
    std::cout << "exit admin\n";
    return 0;
}
