#ifndef EVENT_H
#define EVENT_H

#include <stdint.h>
#include <stddef.h>

/*
 * Portable event core.
 * This layer is platform-agnostic and does not depend on STM32 HAL.
 *
 * Platform-specific code should only use the event APIs below to post
 * input, timer, or custom events into the queue.
 */

#ifndef EVENT_MAX_QUEUE_SIZE
#define EVENT_MAX_QUEUE_SIZE 8U
#endif

#ifndef EVENT_MAX_HANDLERS
#define EVENT_MAX_HANDLERS 8U
#endif

#ifndef EVENT_ENTER_CRITICAL
#define EVENT_ENTER_CRITICAL() do { } while (0)
#endif

#ifndef EVENT_EXIT_CRITICAL
#define EVENT_EXIT_CRITICAL() do { } while (0)
#endif

typedef enum
{
    /* Reserved value used to indicate an empty/invalid event type. */
    EVENT_TYPE_NONE = 0,
    /* System lifecycle events. */
    EVENT_TYPE_SYSTEM_START,
    EVENT_TYPE_SYSTEM_READY,
    /* Periodic tick event for polling or timing based work. */
    EVENT_TYPE_TICK,
    /* Generic key state events. */
    EVENT_TYPE_KEY_DOWN,
    EVENT_TYPE_KEY_UP,
    /* User defined event types can start from here. */
    EVENT_TYPE_CUSTOM = 0x8000U
} EventType;

typedef struct
{
    /* Event category. */
    EventType type;
    /* Optional source ID, for example key index or peripheral index. */
    uint32_t source;
    /* Optional payload value. */
    uint32_t value;
    /* Timestamp in milliseconds provided by the caller. */
    uint32_t timestamp_ms;
    /* Optional pointer for passing extra context. */
    void *context;
} Event;

/* Callback signature used by the dispatcher. */
typedef void (*EventHandler)(const Event *event, void *userData);

/* Reset queue and handler registry. Call once during startup. */
void Event_Init(void);
/* Register one handler for a specific event type. */
uint8_t Event_RegisterHandler(EventType type, EventHandler handler, void *userData);
/* Remove a previously registered handler. */
uint8_t Event_UnregisterHandler(EventType type, EventHandler handler, void *userData);
/* Push a full event object into the queue. */
uint8_t Event_Post(const Event *event);
/* Convenience helper for posting a small event without building a struct first. */
uint8_t Event_PostSimple(EventType type, uint32_t source, uint32_t value, uint32_t timestamp_ms, void *context);
/* Drain the queue and dispatch all pending events. */
void Event_Process(void);
/* Return non-zero when the queue still contains pending events. */
uint8_t Event_HasPending(void);

#endif
