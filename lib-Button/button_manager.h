#ifndef BUTTON_MANAGER_H
#define BUTTON_MANAGER_H

#include <stdint.h>
#include "event.h"

/*
 * Portable button manager example.
 *
 * This module does not call HAL directly. The application supplies the
 * platform-specific read and time functions, so the logic can be reused on
 * other MCUs with minimal changes.
 */

typedef uint32_t (*ButtonReadLevelFn)(uint32_t buttonId, void *context);
typedef uint32_t (*ButtonGetTickFn)(void *context);

typedef struct
{
    uint32_t buttonId;
    uint32_t activeLevel;
    uint32_t debounceMs;
} ButtonConfig;

typedef struct
{
    ButtonConfig config;
    ButtonReadLevelFn readLevel;
    ButtonGetTickFn getTick;
    void *context;
    uint32_t lastRawLevel;
    uint32_t stableLevel;
    uint32_t lastChangeTick;
} ButtonHandle;

/* Initialize one button instance. */
void ButtonManager_Init(ButtonHandle *handle,
                        const ButtonConfig *config,
                        ButtonReadLevelFn readLevel,
                        ButtonGetTickFn getTick,
                        void *context);

/* Poll the button and post KEY_DOWN / KEY_UP events when a stable edge is detected. */
void ButtonManager_Scan(ButtonHandle *handle);

#endif
