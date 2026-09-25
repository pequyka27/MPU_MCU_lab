#include "main.h"
#include "lab_app.h"

static uint32_t last_tick;

/* Active-low LEDs: index 0 = 12 o'clock (PA4), index 11 = PA15. */
void clearAllClock(void)
{
    HAL_GPIO_WritePin(GPIOA, CLOCK_MASK, GPIO_PIN_SET);
}

void setNumberOnClock(int num)
{
    if (num < 0 || num > 11) return;
    HAL_GPIO_WritePin(GPIOA, (uint16_t)(1u << (num + 4)), GPIO_PIN_RESET);
}

void clearNumberOnClock(int num)
{
    if (num < 0 || num > 11) return;
    HAL_GPIO_WritePin(GPIOA, (uint16_t)(1u << (num + 4)), GPIO_PIN_SET);
}

static uint8_t step;

void Lab_Init(void)
{
    step = 0u;
    last_tick = HAL_GetTick();
    clearAllClock();
    setNumberOnClock(0);
}

static void advanceDemo(void)
{
    clearAllClock();
    step = (uint8_t)((step + 1u) % 12u);
    setNumberOnClock(step);
}

void Lab_Update(void)
{
    uint32_t now = HAL_GetTick();
    while ((uint32_t)(now - last_tick) >= 1000u) {
        last_tick += 1000u;
        advanceDemo();
    }
}
