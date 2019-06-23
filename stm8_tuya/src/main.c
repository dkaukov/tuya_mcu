#define MAINWORK_GLOBALS

#include <stm8s.h>
#include <stm8s_it.h>

#include "include.h"

#define FREQUENCY_UPDATE 5000000

extern void Delay(uint16_t nCount);

extern TYPE_BUFFER_S FlashBuffer;

extern bool storeeprom = 0;

extern uint32_t frequency = 0;

extern uint16_t overflow = 0;

extern uint8_t edges = 0;

uint16_t button[MAX_BUTTONS][2] = { {BUTTON_1, LED_1}, {BUTTON_2, LED_2}, {BUTTON_3, LED_3} };
uint8_t button_debounce[MAX_BUTTONS] = {0};
uint32_t button_counter[MAX_BUTTONS] = {0};
bool button_state[MAX_BUTTONS] = {0};
bool last_button_state[MAX_BUTTONS] = {0};
bool input = 0;

void main(void)
{
  uint8_t i = 0;
  GPIO_Config();
  CLK_Config();
  EEPROM_Config();
  UART_Config();
  TIM1_Config();
  EXTI_Config();
  ITC_Config();
  wifi_protocol_init();
  enableInterrupts();
  EEPROM_Read(0, (uint8_t *)&FlashBuffer, sizeof(FlashBuffer));
  for (i = 0; i < MAX_BUTTONS; ++i){
    last_button_state[i] = button_state[i] = GPIO_ReadInputData(BUTTON_PORT) & button[i][0];
  }
  while (1)
  {
    wifi_uart_service();

    for (i = 0; i < MAX_BUTTONS; ++i){
      input = GPIO_ReadInputData(BUTTON_PORT) & button[i][0];
      if(input != last_button_state[i]){
        button_debounce[i] = 0;
      }
      if(++button_debounce[i] >= DEBOUNCE) {
        if(!input){
          button_counter[i] += TIM1_GetCounter();
          if(FlashBuffer.power_switch){
            if(button_counter[i] >= (50000)){ // 100 ms
              if((button[i][0] == BUTTON_1) && FlashBuffer.brightness <= 254) {
                FlashBuffer.brightness+=1;
                LED_PORT->ODR ^= button[i][1];
                brightness_update();
              }
              else if((button[i][0] == BUTTON_3) && FlashBuffer.brightness > 25) {
                FlashBuffer.brightness-=1;
                LED_PORT->ODR ^= button[i][1];
                brightness_update();
              }
              button_counter[i] = 0;
            }
          }
          else{
            if(button_counter[i] >= (7500000)){ // 15 s
              if((button[i][0] == BUTTON_3)) {
                GPIO_WriteLow(MISC_PORT, ESP_GPIO0);
              }
              button_counter[i] = 0;
            }
          }
        }
        else{
          button_counter[i] = 0;
        }
        
        if(input != button_state[i]){
          button_state[i] = input;
          if(button_state[i]){
            if((button[i][0] == BUTTON_2)){
              FlashBuffer.power_switch=FlashBuffer.power_switch?0:1;
              storeeprom = 1;
              switch_update();
            }
            GPIO_WriteLow(LED_PORT, LED_1);
            GPIO_WriteLow(LED_PORT, LED_3);
            GPIO_WriteHigh(MISC_PORT, ESP_GPIO0);
          }
        }
      }
      last_button_state[i] = input;
    }

/*
    if(FlashBuffer.power_switch){
      GPIO_WriteHigh(LED_PORT, LED_2);
    }
    else {
      GPIO_WriteLow(LED_PORT, LED_2);
    }
*/

    if(storeeprom) {
      EEPROM_Program(0, (uint8_t *)&FlashBuffer, sizeof(FlashBuffer));
      storeeprom = 0;
    }

    Delay(0xFF >> 3);
  }
}

void Delay(uint16_t nCount)
{
  uint8_t i;
  for(; nCount !=0 ; nCount--) {
    for(i=255;i!=0;i--) {}
  }
}
