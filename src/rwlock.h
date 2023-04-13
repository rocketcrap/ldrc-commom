#pragma once

#include <Arduino.h>
#include <stdint.h>

/**
 * @brief Reader/Writer lock
 *
 * @details Adapted from https://www.freertos.org/FreeRTOS_Support_Forum_Archive/May_2018/freertos_Readers_Writer_Lock_3ab8578cj.html
 *
 */
class ReadWriteLock
{
public:
    /**
     * @brief Construct a new ReadWriteLock object
     *
     */
    ReadWriteLock();

    virtual ~ReadWriteLock();

    /**
     * @brief Acquire a lock as a reader
     *
     */
    void RLock();

    /**
     * @brief Relinquish a reader lock
     *
     */
    void RUnlock();

    /**
     * @brief Acquire a lock as a writer
     *
     */
    void Lock();

    /**
     * @brief Relinquish a writer lock
     *
     */
    void UnLock();

private:
    constexpr static auto MAX_READERS = 8;
    StaticSemaphore_t semaphoreBuffer;
    SemaphoreHandle_t sem;
    StaticSemaphore_t mutexBuffer;
    SemaphoreHandle_t mutex;
};
