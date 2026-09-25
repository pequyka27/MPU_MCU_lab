#include "main.h"
#include "lab_app.h"

void display7SEG(int num)
{

    int digits[10][7] = {
        {0,0,0,0,0,0,1}, // 0
        {1,0,0,1,1,1,1}, // 1
        {0,0,1,0,0,1,0}, // 2
        {0,0,0,0,1,1,0}, // 3
        {1,0,0,1,1,0,0}, // 4
        {0,1,0,0,1,0,0}, // 5
        {0,1,0,0,0,0,0}, // 6
        {0,0,0,1,1,1,1}, // 7
        {0,0,0,0,0,0,0}, // 8
        {0,0,0,0,1,0,0}  // 9
    };
    uint16_t pins[7] = {
        GPIO_PIN_0, GPIO_PIN_1, GPIO_PIN_2, GPIO_PIN_3,
        GPIO_PIN_4, GPIO_PIN_5, GPIO_PIN_6
    };

    HAL_GPIO_WritePin(GPIOB, SEG_MASK, GPIO_PIN_SET);
    if (num<0 || num>9) {
        return;
    }
    for (int i=0; i<7; i++) {
        if (digits[num][i]==0) {
            HAL_GPIO_WritePin(GPIOB, pins[i], GPIO_PIN_RESET);
        } else {
            HAL_GPIO_WritePin(GPIOB, pins[i], GPIO_PIN_SET);
        }
    }
}

void runExercise(void)
{

    HAL_GPIO_WritePin(GPIOA, TRAFFIC_MASK, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, NS_RED_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, EW_GREEN_Pin, GPIO_PIN_RESET);
    for (int count=5; count>=3; count--) {
        display7SEG(count);
        HAL_Delay(1000);
    }


    HAL_GPIO_WritePin(GPIOA, EW_GREEN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, EW_YELLOW_Pin, GPIO_PIN_RESET);
    for (int count=2; count>=1; count--) {
        display7SEG(count);
        HAL_Delay(1000);
    }


    HAL_GPIO_WritePin(GPIOA, TRAFFIC_MASK, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, NS_GREEN_Pin, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOA, EW_RED_Pin, GPIO_PIN_RESET);
    for (int count=3; count>=1; count--) {
        display7SEG(count);
        HAL_Delay(1000);
    }


    HAL_GPIO_WritePin(GPIOA, NS_GREEN_Pin, GPIO_PIN_SET);
    HAL_GPIO_WritePin(GPIOA, NS_YELLOW_Pin, GPIO_PIN_RESET);
    for (int count=2; count>=1; count--) {
        display7SEG(count);
        HAL_Delay(1000);
    }
}
