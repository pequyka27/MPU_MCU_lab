#include "main.h"
#include "lab_app.h"


uint16_t ledPins[12] = {
    GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6, GPIO_PIN_7,
    GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_11,
    GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15
};

void clearAllClock(void)
{
    for (int i=0; i<12; i++) {
        HAL_GPIO_WritePin(GPIOA, ledPins[i], GPIO_PIN_SET);
    }
}

void setNumberOnClock(int num)
{
    if (num>=0 && num<12) {
        HAL_GPIO_WritePin(GPIOA, ledPins[num], GPIO_PIN_RESET);
    }
}

void clearNumberOnClock(int num)
{
    if (num>=0 && num<12) {
        HAL_GPIO_WritePin(GPIOA, ledPins[num], GPIO_PIN_SET);
    }
}


int hour=10;
int minute=10;
int second=0;

void runExercise(void)
{
    clearAllClock();
    setNumberOnClock(hour % 12);
    setNumberOnClock(minute / 5);
    setNumberOnClock(second / 5);

    HAL_Delay(1000);

    second++;
    if (second==60) {
        second=0;
        minute++;
    }
    if (minute==60) {
        minute=0;
        hour++;
    }
    if (hour==24) {
        hour=0;
    }
}
