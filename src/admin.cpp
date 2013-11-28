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
#include "queue.h"

static struct
{
    pid_t m_corePid;
    pid_t m_queue_listenerPid;
    bool m_terminate;
    int m_nofChildren;
} global;

void sigChldHandler(int x)
{
    (void)x;
    printf("sigChldHandler\n");
    waitpid(-1, 0, WNOHANG);
    global.m_nofChildren--;
}

void sigTermHandler(int x)
{
    (void)x;
    global.m_terminate = true;
    printf("sigTermHandler\n");
}

void signalHandler(int sigNum, siginfo_t *sigInfo, void *data)
{
    (void)sigNum;
    (void)sigInfo;
    (void)data;
    abort();
}

int main(void)
{
    const char *coreProcess = "./patcher";
    const char *screenProcess = "./queue_listener";
    Queue q;
    q.create();
    static struct sigaction act;
    act.sa_sigaction = signalHandler;
    act.sa_handler = sigChldHandler;
    sigemptyset(&act.sa_mask);
    act.sa_flags = SA_SIGINFO;
    if (sigaction(SIGCHLD, &act, 0) < 0)
        perror("sigaction");
    act.sa_handler = sigTermHandler;
    if (sigaction(SIGTERM, &act, 0) < 0)
        perror("sigaction");
    if (sigaction(SIGINT, &act, 0) < 0)
        perror("sigaction");
    global.m_corePid = fork();
    global.m_nofChildren = 2;
    if (global.m_corePid == 0)
    {
        std::cout << "launching " << coreProcess << "\n";
        execl(coreProcess, coreProcess);
        std::cout << "cannot launch " << coreProcess << "\n";
        return -1;
    }
    if (global.m_corePid == -1)
    {
        global.m_nofChildren--;
    }
    global.m_queue_listenerPid = fork();
    if (global.m_queue_listenerPid == 0)
    {
        std::cout << "launching " << screenProcess << "\n";
        execl(screenProcess, screenProcess);
        std::cout << "cannot launch " << screenProcess << "\n";
        return -1;
    }
    if (global.m_corePid == -1)
    {
        global.m_nofChildren--;
    }
    std::cout << "waiting\n";
    for (;;)
    {
        pause();
        std::cout << "caught signal, " << global.m_nofChildren << " children left\n";
        if (global.m_terminate || global.m_nofChildren < 2)
            break;
    }
    kill(global.m_queue_listenerPid, SIGTERM);
    kill(global.m_corePid, SIGTERM);
    q.unlink();
    std::cout << "exit admin\n";
    return 0;
}
