#include "button_manager.h"

static uint32_t Button_ReadLevel(ButtonHandle *handle)
{
    if ((handle == NULL) || (handle->readLevel == NULL))
    {
        return handle->config.activeLevel;
    }

    return handle->readLevel(handle->config.buttonId, handle->context);
}

static uint32_t Button_GetTick(ButtonHandle *handle)
{
    if ((handle == NULL) || (handle->getTick == NULL))
    {
        return 0U;
    }

    return handle->getTick(handle->context);
}

void ButtonManager_Init(ButtonHandle *handle,
                        const ButtonConfig *config,
                        ButtonReadLevelFn readLevel,
                        ButtonGetTickFn getTick,
                        void *context)
{
    uint32_t initialLevel;

    if ((handle == NULL) || (config == NULL))
    {
        return;
    }

    handle->config = *config;
    handle->readLevel = readLevel;
    handle->getTick = getTick;
    handle->context = context;

    initialLevel = Button_ReadLevel(handle);
    handle->lastRawLevel = initialLevel;
    handle->stableLevel = initialLevel;
    handle->lastChangeTick = Button_GetTick(handle);
}

void ButtonManager_Scan(ButtonHandle *handle)
{
    uint32_t currentLevel;
    uint32_t currentTick;

    if (handle == NULL)
    {
        return;
    }

    currentLevel = Button_ReadLevel(handle);
    currentTick = Button_GetTick(handle);

    if (currentLevel != handle->lastRawLevel)
    {
        handle->lastRawLevel = currentLevel;
        handle->lastChangeTick = currentTick;
        return;
    }

    if ((currentTick - handle->lastChangeTick) < handle->config.debounceMs)
    {
        return;
    }

    if (currentLevel == handle->stableLevel)
    {
        return;
    }

    handle->stableLevel = currentLevel;

    if (currentLevel == handle->config.activeLevel)
    {
        Event_PostSimple(EVENT_TYPE_KEY_DOWN,
                         handle->config.buttonId,
                         currentLevel,
                         currentTick,
                         handle);
    }
    else
    {
        Event_PostSimple(EVENT_TYPE_KEY_UP,
                         handle->config.buttonId,
                         currentLevel,
                         currentTick,
                         handle);
    }
}
