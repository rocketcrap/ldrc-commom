#include "rwlock.h"

ReadWriteLock::ReadWriteLock()
{
    sem = xSemaphoreCreateCountingStatic(MAX_READERS, MAX_READERS, &semaphoreBuffer);
    mutex = xSemaphoreCreateMutexStatic(&mutexBuffer);
}

ReadWriteLock::~ReadWriteLock()
{
}

void ReadWriteLock::RLock()
{
    xSemaphoreTake(sem, portMAX_DELAY);
}

void ReadWriteLock::RUnlock()
{
    xSemaphoreGive(sem);
}

void ReadWriteLock::Lock()
{
    uint_fast8_t count;

    xSemaphoreTake(mutex, portMAX_DELAY);
    for (count = 0; count < MAX_READERS; count++)
    {
        xSemaphoreTake(sem, portMAX_DELAY);
    }
}

void ReadWriteLock::UnLock()
{
    uint_fast8_t count;
    for (count = 0; count < MAX_READERS; count++)
    {
        xSemaphoreGive(sem);
    }
    xSemaphoreGive(mutex);
}
