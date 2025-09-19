

#ifndef DS2431_DRIVER_DS2431_H_
#define DS2431_DRIVER_DS2431_H_


#include <stdint.h>
#include "stm32f4xx_hal.h"	//modify to match your STM32 MCU family
#include <stdio.h>
#include <string.h>

//Onewire driver defines
#define ONE_WIRE_GPIO_PORT GPIOA		//enter GPIO port used here
#define ONE_WIRE_PIN GPIO_PIN_9			//enter the GPIO pin used for 1 wire communication
#define boundaryaddress 0x007F			//last address of main EEPROM page3

void OneWire_Reset(void);
void OneWire_WriteByte(uint8_t byte);
uint8_t OneWire_ReadByte(void);
void OneWire_WriteMemory(uint16_t startaddress, uint8_t *pdata);
void OneWire_ReadMemory(uint16_t startaddress, size_t bytesToRead, uint8_t *pdataBuffer);
void Read_ScratchPad(uint8_t *pdata);
void Copy_ScratchPad();
void OneWire_init(void);


//microsecond delay function defines
#define TIMER_INSTANCE 	TIM6	//modify if using a different timer
#define TIMER_MAX  		0xFFFF	//modify to 0xFFFF if using 16bit timer, 0xFFFFFFFF if using 32bit timer

extern void Error_Handler(void);



#endif /* DS2431_DRIVER_DS2431_H_ */
