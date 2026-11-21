/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body - Multiplexed 7-segment display version
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private typedef -----------------------------------------------------------*/
typedef struct
{
  GPIO_PinState candidate;
  GPIO_PinState stable;
  uint32_t changed_ms;
} ButtonState;

/* Private define ------------------------------------------------------------*/
#define BLINK_HALF_PERIOD_MS 250U
#define BUTTON_COUNT 3U
#define BUTTON_SCAN_MS 10U
#define BUTTON_DEBOUNCE_MS 20U
#define DISPLAY_SCAN_MS 250U // Giu moi LED 250ms (4 LED = 1 giay) de mat nhin ro tung so tren Proteus

/* Private variables ---------------------------------------------------------*/
TIM_HandleTypeDef htim2;

volatile uint8_t mode = 1U;
static ButtonState buttons[BUTTON_COUNT];
static const uint16_t button_pins[BUTTON_COUNT] = {BTN1_Pin, BTN2_Pin, BTN3_Pin};
static uint32_t buttons_previous_ms = 0;

static const uint8_t LED7_HEX[10] = {
  0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F
};

static const uint16_t seg_pins[7] = {
  a_Pin, b_Pin, c_Pin, d_Pin, e_Pin, f_Pin, g_Pin
};

static const uint16_t en_pins[4] = {
  EN0_Pin, EN1_Pin, EN2_Pin, EN3_Pin
};

static uint8_t display_buffer[4] = {0, 0, 0, 0};
static uint8_t current_digit = 0U;
static uint32_t display_prev_ms = 0;

static uint8_t red_duration = 5U;
static uint8_t amber_duration = 2U;
static uint8_t green_duration = 3U;

static uint8_t temp_red_duration = 5U;
static uint8_t temp_amber_duration = 2U;
static uint8_t temp_green_duration = 3U;

typedef enum
{
  TRAFFIC_RED = 0,
  TRAFFIC_GREEN,
  TRAFFIC_AMBER
} TrafficState;

static TrafficState road1_state = TRAFFIC_RED;
static TrafficState road2_state = TRAFFIC_GREEN;
static uint8_t road1_countdown = 5U;
static uint8_t road2_countdown = 3U;
static uint32_t traffic_1s_prev_ms = 0;

static uint8_t blink_state = 1U;
static uint32_t blink_prev_ms = 0;

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_TIM2_Init(void);

static uint32_t Timer_NowMs(void)
{
  return HAL_GetTick();
}

static uint8_t Timer_HasElapsed(uint32_t *previous_ms, uint32_t period_ms)
{
  uint32_t now = Timer_NowMs();
  if ((period_ms != 0U) && ((now - *previous_ms) >= period_ms))
  {
    *previous_ms = now;
    return 1U;
  }
  return 0U;
}

static void Traffic_AllOff(void)
{
  HAL_GPIO_WritePin(GPIOA, RED_1_Pin | AMBER_1_Pin | GREEN_1_Pin |
                           RED_2_Pin | AMBER_2_Pin | GREEN_2_Pin,
                           GPIO_PIN_SET);
}

static void Traffic_SetNormalLeds(void)
{
  Traffic_AllOff();

  /* Road 1 */
  if (road1_state == TRAFFIC_RED)
    HAL_GPIO_WritePin(GPIOA, RED_1_Pin, GPIO_PIN_RESET);
  else if (road1_state == TRAFFIC_GREEN)
    HAL_GPIO_WritePin(GPIOA, GREEN_1_Pin, GPIO_PIN_RESET);
  else
    HAL_GPIO_WritePin(GPIOA, AMBER_1_Pin, GPIO_PIN_RESET);

  /* Road 2 */
  if (road2_state == TRAFFIC_RED)
    HAL_GPIO_WritePin(GPIOA, RED_2_Pin, GPIO_PIN_RESET);
  else if (road2_state == TRAFFIC_GREEN)
    HAL_GPIO_WritePin(GPIOA, GREEN_2_Pin, GPIO_PIN_RESET);
  else
    HAL_GPIO_WritePin(GPIOA, AMBER_2_Pin, GPIO_PIN_RESET);
}

static void Traffic_InitNormal(void)
{
  road1_state = TRAFFIC_RED;
  road2_state = TRAFFIC_GREEN;
  road1_countdown = red_duration;
  road2_countdown = green_duration;
  Traffic_SetNormalLeds();
}

static void Traffic_Tick1s(void)
{
  if (road1_countdown > 1U)
  {
    road1_countdown--;
  }
  else
  {
    if (road1_state == TRAFFIC_RED)
    {
      road1_state = TRAFFIC_GREEN;
      road1_countdown = green_duration;
    }
    else if (road1_state == TRAFFIC_GREEN)
    {
      road1_state = TRAFFIC_AMBER;
      road1_countdown = amber_duration;
    }
    else
    {
      road1_state = TRAFFIC_RED;
      road1_countdown = red_duration;
    }
  }

  if (road2_countdown > 1U)
  {
    road2_countdown--;
  }
  else
  {
    if (road2_state == TRAFFIC_GREEN)
    {
      road2_state = TRAFFIC_AMBER;
      road2_countdown = amber_duration;
    }
    else if (road2_state == TRAFFIC_AMBER)
    {
      road2_state = TRAFFIC_RED;
      road2_countdown = red_duration;
    }
    else
    {
      road2_state = TRAFFIC_GREEN;
      road2_countdown = green_duration;
    }
  }

  Traffic_SetNormalLeds();
}

static void Traffic_SetBlinkLeds(void)
{
  Traffic_AllOff();
  if (blink_state != 0U)
  {
    if (mode == 2U)
      HAL_GPIO_WritePin(GPIOA, RED_1_Pin | RED_2_Pin, GPIO_PIN_RESET);
    else if (mode == 3U)
      HAL_GPIO_WritePin(GPIOA, AMBER_1_Pin | AMBER_2_Pin, GPIO_PIN_RESET);
    else if (mode == 4U)
      HAL_GPIO_WritePin(GPIOA, GREEN_1_Pin | GREEN_2_Pin, GPIO_PIN_RESET);
  }
}

static void Display_UpdateBuffer(void)
{
  if (mode == 1U)
  {
    display_buffer[0] = (uint8_t)(road1_countdown / 10U);
    display_buffer[1] = (uint8_t)(road1_countdown % 10U);
    display_buffer[2] = (uint8_t)(road2_countdown / 10U);
    display_buffer[3] = (uint8_t)(road2_countdown % 10U);
  }
  else if (mode == 2U)
  {
    display_buffer[0] = (uint8_t)(temp_red_duration / 10U);
    display_buffer[1] = (uint8_t)(temp_red_duration % 10U);
    display_buffer[2] = 0U;
    display_buffer[3] = 2U;
  }
  else if (mode == 3U)
  {
    display_buffer[0] = (uint8_t)(temp_amber_duration / 10U);
    display_buffer[1] = (uint8_t)(temp_amber_duration % 10U);
    display_buffer[2] = 0U;
    display_buffer[3] = 3U;
  }
  else if (mode == 4U)
  {
    display_buffer[0] = (uint8_t)(temp_green_duration / 10U);
    display_buffer[1] = (uint8_t)(temp_green_duration % 10U);
    display_buffer[2] = 0U;
    display_buffer[3] = 4U;
  }
}

/**
 * @brief  Hàm quét LED 7 đoạn (Multiplexing)
 *         Chạy mỗi DISPLAY_SCAN_MS (10ms) để không làm nghẽn CPU Proteus
 */
static void Display_Scan(void)
{
  // 1. Tắt tất cả các LED (chống bóng ma - ghosting)
  // Mạch dùng transistor PNP 2N3906: Mức 1 là TẮT, Mức 0 là BẬT
  HAL_GPIO_WritePin(GPIOA, EN0_Pin | EN1_Pin | EN2_Pin | EN3_Pin, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, a_Pin | b_Pin | c_Pin | d_Pin | e_Pin | f_Pin | g_Pin, GPIO_PIN_SET);

  // 2. Xuất mã 7 đoạn của số hiện tại ra các chân a-g (Anode chung: Mức 0 sáng)
  uint8_t digit_value = display_buffer[current_digit];
  if (digit_value > 9U) digit_value = 0U;
  uint8_t pattern = LED7_HEX[digit_value];

  for (uint8_t i = 0U; i < 7U; i++)
  {
    GPIO_PinState state = ((pattern & (1U << i)) != 0U) ? GPIO_PIN_RESET : GPIO_PIN_SET;
    HAL_GPIO_WritePin(GPIOB, seg_pins[i], state);
  }

  // 3. Bật đúng LED của vị trí hiện tại
  HAL_GPIO_WritePin(GPIOA, en_pins[current_digit], GPIO_PIN_RESET);

  // 4. Chuyển sang LED tiếp theo cho lần quét sau
  current_digit = (uint8_t)((current_digit + 1U) % 4U);
}

static void Buttons_Init(void)
{
  uint32_t now_ms = Timer_NowMs();
  buttons_previous_ms = now_ms;
  for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
  {
    buttons[i].candidate = GPIO_PIN_SET;
    buttons[i].stable = GPIO_PIN_SET;
    buttons[i].changed_ms = now_ms;
  }
}

static uint8_t Buttons_ReadEvents(void)
{
  uint32_t now_ms = Timer_NowMs();
  uint8_t events = 0U;

  if ((uint32_t)(now_ms - buttons_previous_ms) < BUTTON_SCAN_MS)
  {
    return 0U;
  }
  buttons_previous_ms = now_ms;

  for (uint8_t i = 0U; i < BUTTON_COUNT; i++)
  {
    GPIO_PinState raw = HAL_GPIO_ReadPin(GPIOB, button_pins[i]);
    if (raw != buttons[i].candidate)
    {
      buttons[i].candidate = raw;
      buttons[i].changed_ms = now_ms;
    }
    else if ((raw != buttons[i].stable) &&
             ((uint32_t)(now_ms - buttons[i].changed_ms) >= BUTTON_DEBOUNCE_MS))
    {
      buttons[i].stable = raw;
      if (raw == GPIO_PIN_RESET)
      {
        events |= (uint8_t)(1U << i);
      }
    }
  }
  return events;
}

static void Input_Process(void)
{
  uint8_t events = Buttons_ReadEvents();

  if ((events & 1U) != 0U)
  {
    mode = (mode == 4U) ? 1U : (uint8_t)(mode + 1U);
    if (mode == 1U)
    {
      Traffic_InitNormal();
      traffic_1s_prev_ms = Timer_NowMs();
    }
    else
    {
      if (mode == 2U) temp_red_duration = red_duration;
      else if (mode == 3U) temp_amber_duration = amber_duration;
      else if (mode == 4U) temp_green_duration = green_duration;

      blink_state = 1U;
      blink_prev_ms = Timer_NowMs();
      Traffic_SetBlinkLeds();
    }
    Display_UpdateBuffer();
  }

  if ((events & 2U) != 0U)
  {
    if (mode == 2U)
    {
      temp_red_duration = (temp_red_duration >= 99U) ? 1U : (uint8_t)(temp_red_duration + 1U);
      Display_UpdateBuffer();
    }
    else if (mode == 3U)
    {
      temp_amber_duration = (temp_amber_duration >= 99U) ? 1U : (uint8_t)(temp_amber_duration + 1U);
      Display_UpdateBuffer();
    }
    else if (mode == 4U)
    {
      temp_green_duration = (temp_green_duration >= 99U) ? 1U : (uint8_t)(temp_green_duration + 1U);
      Display_UpdateBuffer();
    }
  }

  if ((events & 4U) != 0U)
  {
    if (mode == 2U)
    {
      red_duration = temp_red_duration;
      Display_UpdateBuffer();
    }
    else if (mode == 3U)
    {
      amber_duration = temp_amber_duration;
      Display_UpdateBuffer();
    }
    else if (mode == 4U)
    {
      green_duration = temp_green_duration;
      Display_UpdateBuffer();
    }
  }
}

int main(void)
{
  HAL_Init();
  SystemClock_Config();

  MX_GPIO_Init();
  MX_TIM2_Init();

  HAL_TIM_Base_Start_IT(&htim2);

  Buttons_Init();
  traffic_1s_prev_ms = Timer_NowMs();
  blink_prev_ms = Timer_NowMs();
  display_prev_ms = Timer_NowMs();
  blink_state = 1U;

  Traffic_InitNormal();
  Display_UpdateBuffer();

  while (1)
  {
    Input_Process();

    // 1. Quet LED 7 doan moi 10ms (100 Hz quet tong the)
    if (Timer_HasElapsed(&display_prev_ms, DISPLAY_SCAN_MS))
    {
      Display_Scan();
    }

    // 2. May trang thai den giao thong
    if (mode == 1U)
    {
      if (Timer_HasElapsed(&traffic_1s_prev_ms, 1000U))
      {
        Traffic_Tick1s();
        Display_UpdateBuffer();
      }
    }
    else
    {
      if (Timer_HasElapsed(&blink_prev_ms, BLINK_HALF_PERIOD_MS))
      {
        blink_state = (uint8_t)(!blink_state);
        Traffic_SetBlinkLeds();
      }
    }
  }
}

void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

static void MX_TIM2_Init(void)
{
  __HAL_RCC_TIM2_CLK_ENABLE();

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7999;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }

  HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(TIM2_IRQn);
}

static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_AFIO_CLK_ENABLE();
  __HAL_AFIO_REMAP_SWJ_NOJTAG();

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /* Tat tat ca LED giao thong va LED 7 doan ban dau */
  HAL_GPIO_WritePin(GPIOA, RED_1_Pin | AMBER_1_Pin | GREEN_1_Pin |
                           RED_2_Pin | AMBER_2_Pin | GREEN_2_Pin |
                           EN0_Pin | EN1_Pin | EN2_Pin | EN3_Pin,
                           GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOB, a_Pin | b_Pin | c_Pin | d_Pin | e_Pin | f_Pin | g_Pin,
                           GPIO_PIN_SET);

  /* Cau hinh chan Traffic LEDs va EN0..EN3 tren Port A */
  GPIO_InitStruct.Pin = RED_1_Pin | AMBER_1_Pin | GREEN_1_Pin |
                        RED_2_Pin | AMBER_2_Pin | GREEN_2_Pin |
                        EN0_Pin | EN1_Pin | EN2_Pin | EN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /* Cau hinh chan Segments a..g tren Port B */
  GPIO_InitStruct.Pin = a_Pin | b_Pin | c_Pin | d_Pin | e_Pin | f_Pin | g_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /* Cau hinh nut nhan BTN1, BTN2, BTN3 tren Port B */
  GPIO_InitStruct.Pin = BTN1_Pin | BTN2_Pin | BTN3_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);
}

void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  if (htim->Instance == TIM2)
  {
  }
}

void Error_Handler(void)
{
  __disable_irq();
  while (1)
  {
  }
}
