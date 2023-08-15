#include "unistd.h"
#include "stdio.h"
#include "stdlib.h"

char *client = "./iperf3";
char *server = "./iperf3";
char *clientArgv[] = {
    "./iperf3"
    "-c",
    "127.0.0.1",
    "-p",
    "5001",
    "-t",
    "2",
    "-i",
    "0",
    "-u",
    "-b",
    "1000G",
};

char *serverArgv[] = {
    "./iperf3",
    "-s",
    "-p",
    "5001",
};

int main()
{
    dev(1, O_RDWR);
    dup(0);
    dup(0);
    printf("iperf test begin\n");
    int pid = fork();
    if (pid == 0)
    {
        // printf("%d\n", i);
        // printf("pid0\n");
        execve(client, clientArgv, NULL);
    }
    else
    {
        execve(server, serverArgv, NULL);
    }

    return 0;
}