#ifndef SIGHANDLER_H
#define SIGHANDLER_H
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/statvfs.h>
#include <cstring>
#include <stdio.h>

volatile int shutdown_signal;
volatile int alarm_signal;

void sig_handler(int sig)
{
    shutdown_signal = 1;
}

void alarm_handler(int sig)
{
    alarm_signal = 1;
}

void add_signal_handlers()
{
    struct sigaction sia;

    memset(&sia, 0, sizeof(sia));
    sia.sa_handler = sig_handler;

    if (sigaction(SIGINT, &sia, NULL) < 0) {
        perror("sigaction(SIGINT)");
        _exit(1);
    }

    memset(&sia, 0, sizeof(sia));
    sia.sa_handler = alarm_handler;

    if (sigaction(SIGALRM, &sia, NULL) < 0) {
        perror("sigaction(SIGALRM)");
        _exit(1);
    }
}

#endif
