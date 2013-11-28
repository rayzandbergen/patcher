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

static struct
{
    pid_t m_corePid;
    pid_t m_queue_listenerPid;
    bool m_terminate;
    int m_nofChildren;
} global;

void sigChldHandler(int x)
{
    printf("sigChldHandler\n");
    waitpid(-1, 0, WNOHANG);
    global.m_nofChildren--;
}

void sigTermHandler(int x)
{
    global.m_terminate = true;
    printf("sigTermHandler\n");
}

void signalHandler(int sigNum, siginfo_t *sigInfo, void *data)
{
    abort();
}

int main(void)
{
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
    global.m_corePid = fork();
    global.m_nofChildren = 2;
    if (global.m_corePid == 0)
    {
        printf("launching patcher\n");
        execl("./patcher", "./patcher");
        std::cout << "fail ./patcher\n";
        return 0;
    }
    if (global.m_corePid == -1)
    {
        global.m_nofChildren--;
    }
    global.m_queue_listenerPid = fork();
    if (global.m_queue_listenerPid == 0)
    {
        sleep(1);
        printf("launching queue_listener\n");
        execl("./queue_listener", "./queue_listener");
        std::cout << "fail ./queue_listener\n";
        return 0;
    }
    if (global.m_corePid == -1)
    {
        global.m_nofChildren--;
    }
    printf("waiting\n");
    for (;;)
    {
        pause();
        std::cout << global.m_corePid << " "
            << global.m_queue_listenerPid  << " "
            << global.m_nofChildren << "\n";
        if (global.m_nofChildren < 2)
            break;
        if (global.m_terminate)
            break;
    }
    kill(global.m_queue_listenerPid, SIGTERM);
    kill(global.m_corePid, SIGTERM);
    std::cout << "exit parent\n";
    return 0;
}
