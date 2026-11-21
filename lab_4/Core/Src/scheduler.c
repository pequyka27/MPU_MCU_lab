#include "scheduler.h"

static sTask SCH_tasks_G[SCH_MAX_TASKS];
static uint32_t current_task_id = 1;

void SCH_Init(void) {
    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        SCH_tasks_G[i].pTask = 0;
        SCH_tasks_G[i].Delay = 0;
        SCH_tasks_G[i].Period = 0;
        SCH_tasks_G[i].RunMe = 0;
        SCH_tasks_G[i].TaskID = 0;
    }
}

void SCH_Update(void) {
    if (SCH_tasks_G[0].pTask != 0) {
        if (SCH_tasks_G[0].Delay > 0) {
            SCH_tasks_G[0].Delay--;
        }
        if (SCH_tasks_G[0].Delay == 0) {
            SCH_tasks_G[0].RunMe = 1;
        }
    }
}

uint32_t SCH_Add_Task(void (*pFunction)(void), uint32_t DELAY, uint32_t PERIOD) {
    if (pFunction == 0) return 0;

    __disable_irq();

    uint8_t index = 0;
    while (index < SCH_MAX_TASKS && SCH_tasks_G[index].pTask != 0) {
        if (DELAY < SCH_tasks_G[index].Delay) {
            SCH_tasks_G[index].Delay -= DELAY;
            break;
        } else {
            DELAY -= SCH_tasks_G[index].Delay;
            index++;
        }
    }

    if (index == SCH_MAX_TASKS) {
        __enable_irq();
        return 0;
    }

    for (int8_t i = SCH_MAX_TASKS - 1; i > index; i--) {
        SCH_tasks_G[i] = SCH_tasks_G[i - 1];
    }

    SCH_tasks_G[index].pTask = pFunction;
    SCH_tasks_G[index].Delay = DELAY;
    SCH_tasks_G[index].Period = PERIOD;
    SCH_tasks_G[index].RunMe = (DELAY == 0) ? 1 : 0;
    SCH_tasks_G[index].TaskID = current_task_id++;

    __enable_irq();
    return SCH_tasks_G[index].TaskID;
}

uint8_t SCH_Delete_Task(uint32_t taskID) {
    if (taskID == 0) return 0;

    __disable_irq();

    for (uint8_t i = 0; i < SCH_MAX_TASKS; i++) {
        if (SCH_tasks_G[i].TaskID == taskID) {
            if (i + 1 < SCH_MAX_TASKS && SCH_tasks_G[i + 1].pTask != 0) {
                SCH_tasks_G[i + 1].Delay += SCH_tasks_G[i].Delay;
            }

            for (uint8_t j = i; j < SCH_MAX_TASKS - 1; j++) {
                SCH_tasks_G[j] = SCH_tasks_G[j + 1];
            }

            SCH_tasks_G[SCH_MAX_TASKS - 1].pTask = 0;
            SCH_tasks_G[SCH_MAX_TASKS - 1].Delay = 0;
            SCH_tasks_G[SCH_MAX_TASKS - 1].Period = 0;
            SCH_tasks_G[SCH_MAX_TASKS - 1].RunMe = 0;
            SCH_tasks_G[SCH_MAX_TASKS - 1].TaskID = 0;

            __enable_irq();
            return 1;
        }
    }

    __enable_irq();
    return 0;
}

void SCH_Dispatch_Tasks(void) {
    while (SCH_tasks_G[0].pTask != 0 && (SCH_tasks_G[0].RunMe > 0 || SCH_tasks_G[0].Delay == 0)) {
        void (*pFunction)(void) = SCH_tasks_G[0].pTask;
        uint32_t period = SCH_tasks_G[0].Period;

        (*pFunction)();

        SCH_Delete_Task(SCH_tasks_G[0].TaskID);

        if (period > 0) {
            SCH_Add_Task(pFunction, period, period);
        }
    }
}
