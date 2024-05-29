#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <time.h>
#include <unistd.h>

#define TIMEOUT 15

typedef struct
{
    int x;
    int isFReady;
    int isGReady;
    int fResult;
    int gResult;
} CalculationsData;

int f(int x)
{
    if (x % 2 == 0)
    {
        while (1)
        {
        }
    }

    if (x > 10)
    {
        sleep(x % 5);
        return x * 2;
    }

    return -x * 5;
}

int g(int x)
{
    if (x < 0)
    {
        return -x;
    }

    if (x % 2 == 1)
    {
        return x - 100;
    }

    sleep(x % 10);

    return x + 1;
}

void *runF(void *arg)
{
    int shmid = *((int *)arg);

    CalculationsData *data = (CalculationsData *)shmat(shmid, NULL, 0);

    data->fResult = f(data->x);
    data->isFReady = 1;
    shmdt(data);

    return NULL;
}

void *runG(void *arg)
{
    int shmid = *((int *)arg);

    CalculationsData *data = (CalculationsData *)shmat(shmid, NULL, 0);

    data->gResult = g(data->x);
    data->isGReady = 1;
    shmdt(data);

    return NULL;
}

int main()
{
    int shmid = shmget(IPC_PRIVATE, 1024, IPC_CREAT | 0666);
    CalculationsData *data = (CalculationsData *)shmat(shmid, NULL, 0);

    printf("Enter x -> ");
    scanf("%d", &(data->x));
    data->isFReady = data->isGReady = 0;

    pthread_t thread1, thread2;
    pthread_create(&thread1, NULL, runF, &shmid);
    pthread_create(&thread2, NULL, runG, &shmid);

    pthread_detach(thread1);
    pthread_detach(thread2);

    time_t lastTimeoutCheckTime = time(NULL);

    int shouldStop = 0;
    int result;

    while (!shouldStop)
    {
        if (data->isFReady && data->isGReady)
        {
            result = data->fResult || data->gResult;
            break;
        }
        else if (data->isFReady && data->fResult || data->isGReady && data->gResult)
        {
            result = 1;
            break;
        }

        int isTimeout = difftime(time(NULL), lastTimeoutCheckTime) >= TIMEOUT;

        if (isTimeout)
        {
            char action;

            printf("One of the functions seem to be in a cycle? Do you want to stop execution? (y/N): ");
            scanf("%c", &action);

            lastTimeoutCheckTime = time(NULL);

            if (action == 'y')
            {
                shouldStop = 1;
            };
        }
    }

    pthread_cancel(thread1);
    pthread_cancel(thread2);

    shmdt(data);
    shmctl(shmid, IPC_RMID, NULL);

    if (!shouldStop)
    {
        printf("Result: %d\n", result);
    }

    return 0;
}