#include "event.h"

typedef struct
{
    /* One registered callback entry. */
    EventHandler handler;
    void *userData;
    EventType type;
    uint8_t inUse;
} EventBinding;

typedef struct
{
    /* Simple ring buffer used as the event queue. */
    Event buffer[EVENT_MAX_QUEUE_SIZE];
    uint8_t head;
    uint8_t tail;
    uint8_t count;
} EventQueue;

/* Global queue and handler table owned by the core event module. */
static EventQueue g_queue;
static EventBinding g_bindings[EVENT_MAX_HANDLERS];

static uint8_t Event_IsCustomType(EventType type)
{
    return (type >= EVENT_TYPE_CUSTOM) ? 1U : 0U;
}

static uint8_t Event_TypeMatches(EventType registeredType, EventType incomingType)
{
    if (registeredType == incomingType)
    {
        return 1U;
    }

    if (registeredType == EVENT_TYPE_CUSTOM && Event_IsCustomType(incomingType))
    {
        return 1U;
    }

    return 0U;
}

static uint8_t Event_QueuePush(const Event *event)
{
    if ((event == NULL) || (g_queue.count >= EVENT_MAX_QUEUE_SIZE))
    {
        return 0U;
    }

    /* Queue access is wrapped in critical section macros so the port layer
     * can map them to interrupts disable/enable on the target MCU.
     */
    EVENT_ENTER_CRITICAL();
    g_queue.buffer[g_queue.head] = *event;
    g_queue.head = (uint8_t)((g_queue.head + 1U) % EVENT_MAX_QUEUE_SIZE);
    g_queue.count++;
    EVENT_EXIT_CRITICAL();

    return 1U;
}

static uint8_t Event_QueuePop(Event *event)
{
    if ((event == NULL) || (g_queue.count == 0U))
    {
        return 0U;
    }

    EVENT_ENTER_CRITICAL();
    *event = g_queue.buffer[g_queue.tail];
    g_queue.tail = (uint8_t)((g_queue.tail + 1U) % EVENT_MAX_QUEUE_SIZE);
    g_queue.count--;
    EVENT_EXIT_CRITICAL();

    return 1U;
}

void Event_Init(void)
{
    uint32_t index;

    /* Clear the queue and all handler slots. */
    EVENT_ENTER_CRITICAL();
    g_queue.head = 0U;
    g_queue.tail = 0U;
    g_queue.count = 0U;

    for (index = 0U; index < EVENT_MAX_HANDLERS; index++)
    {
        g_bindings[index].handler = NULL;
        g_bindings[index].userData = NULL;
        g_bindings[index].type = EVENT_TYPE_NONE;
        g_bindings[index].inUse = 0U;
    }
    EVENT_EXIT_CRITICAL();
}

uint8_t Event_RegisterHandler(EventType type, EventHandler handler, void *userData)
{
    uint32_t index;

    if (handler == NULL)
    {
        return 0U;
    }

    /* First free slot wins. This keeps the implementation small and predictable. */
    EVENT_ENTER_CRITICAL();
    for (index = 0U; index < EVENT_MAX_HANDLERS; index++)
    {
        if (g_bindings[index].inUse == 0U)
        {
            g_bindings[index].handler = handler;
            g_bindings[index].userData = userData;
            g_bindings[index].type = type;
            g_bindings[index].inUse = 1U;
            EVENT_EXIT_CRITICAL();
            return 1U;
        }
    }
    EVENT_EXIT_CRITICAL();

    return 0U;
}

uint8_t Event_UnregisterHandler(EventType type, EventHandler handler, void *userData)
{
    uint32_t index;

    /* Remove the exact binding requested by the caller. */
    EVENT_ENTER_CRITICAL();
    for (index = 0U; index < EVENT_MAX_HANDLERS; index++)
    {
        if ((g_bindings[index].inUse != 0U) &&
            (g_bindings[index].type == type) &&
            (g_bindings[index].handler == handler) &&
            (g_bindings[index].userData == userData))
        {
            g_bindings[index].handler = NULL;
            g_bindings[index].userData = NULL;
            g_bindings[index].type = EVENT_TYPE_NONE;
            g_bindings[index].inUse = 0U;
            EVENT_EXIT_CRITICAL();
            return 1U;
        }
    }
    EVENT_EXIT_CRITICAL();

    return 0U;
}

uint8_t Event_Post(const Event *event)
{
    /* Post a pre-built event to the queue. */
    return Event_QueuePush(event);
}

uint8_t Event_PostSimple(EventType type, uint32_t source, uint32_t value, uint32_t timestamp_ms, void *context)
{
    Event event;

    event.type = type;
    event.source = source;
    event.value = value;
    event.timestamp_ms = timestamp_ms;
    event.context = context;

    return Event_QueuePush(&event);
}

void Event_Process(void)
{
    Event event;
    uint32_t index;

    /* Drain the queue completely so all pending events are handled. */
    while (Event_QueuePop(&event) != 0U)
    {
        for (index = 0U; index < EVENT_MAX_HANDLERS; index++)
        {
            if ((g_bindings[index].inUse != 0U) &&
                Event_TypeMatches(g_bindings[index].type, event.type) != 0U)
            {
                g_bindings[index].handler(&event, g_bindings[index].userData);
            }
        }
    }
}

uint8_t Event_HasPending(void)
{
    return (g_queue.count != 0U) ? 1U : 0U;
}
