


#include "ds2431.h"

static void displaydata(uint16_t startaddress, size_t bytesToRead, uint8_t *pdataBuffer);
static void GPIOMode_Input(void);
static void GPIOMode_Output(void);
static void TIM6_init(void);
static void tim_MSP_init(void);

/*blocking delay in microseconds*/
static void delay_us(uint32_t delay);

/*
 * RxBuffer for scratchpad data
 * readScratchPad[0] - TA1
 * readScratchPad[1] - TA2
 * readScratchPad[2] - E/S BYTE
 * readScratchPad[10:3] - Scratch pad Data bytes
 */
static uint8_t readScratchPad[11];

TIM_HandleTypeDef htim6 = {0};

/*Call this function before initiating onewire communication */
void OneWire_init(void)
{
	/*
	 * Intialize GPIO pins for 1-wire communication
	 * Sets GPIOA_PIN9 as output open-drain mode.
	 * You can modify the GPIO number and port at ds2431.h
	 */
	GPIOMode_Output();

	/*Set GPIO9 idle at high*/
	HAL_GPIO_WritePin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN, GPIO_PIN_SET);

	/*Initialize TIM6 used for timebase generation for microsecond delay*/
	TIM6_init();
}

/*
 * @brief: Generates a reset pulse on the 1-Wire bus.
 */
void OneWire_Reset(void) {
	// Generate 1-Wire reset pulse

	//config GPIO to output for master to initiate reset pulse
	GPIOMode_Output();
	HAL_GPIO_WritePin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN, GPIO_PIN_RESET);
	delay_us(490);  // Reset pulse duration
	HAL_GPIO_WritePin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN, GPIO_PIN_SET);

	//configure GPIO as input so slave could take over the line
	GPIOMode_Input();
	delay_us(400);   // Presence pulse wait
}


/*
 * @brief: Writes one byte of data to the 1-Wire bus.
 * @param: uint8_t byte, the byte to write to the bus.
 */
void OneWire_WriteByte(uint8_t byte)
{

	/*configure GPIO as output so it could drive the 1-Wire bus*/
	GPIOMode_Output();

	// Send 8 bits (LSB first)
	for (int i = 0; i < 8; i++) {

		//initiate write time slot by pulling line to gnd for 6us. 1us <= tlow1 <= 15us DS limits
		HAL_GPIO_WritePin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN, GPIO_PIN_RESET);
		delay_us(6);

		//send data bit then wait 55us for slave to sample data.
		HAL_GPIO_WritePin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN, (byte & (1 << i)) ? GPIO_PIN_SET : GPIO_PIN_RESET);
		delay_us(55);  // Timing for bit transmission

		//pull the line back to IDLE state (high) and hold for 5us to prepare for next write time slot
		//60us <= tslot <= 120us DS limits.
		HAL_GPIO_WritePin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN, GPIO_PIN_SET);
		delay_us(5);
	}
}


/*
 * @brief: Reads one byte of data from the 1-Wire bus.
 * @return: uint8_t, the byte read from the bus.
 */
uint8_t OneWire_ReadByte(void) {
	uint8_t data = 0;

	// Read 8 bits (LSB first)
	for (int i = 0; i < 8; i++) {

		GPIOMode_Output();
		HAL_GPIO_WritePin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN, GPIO_PIN_RESET);
		delay_us(5); // Initiate read time slot
		HAL_GPIO_WritePin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN, GPIO_PIN_SET);

		/*Set GPIO as input to let slave take over the bus*/
		GPIOMode_Input();
		delay_us(10); // Data acquisition timing

		/*If read bit is equal to 1, set the corresponding bitfield in data variable*/
		if (HAL_GPIO_ReadPin(ONE_WIRE_GPIO_PORT, ONE_WIRE_PIN) == 1) {
			data |= (1 << i);
		}

		delay_us(50); // Ensure proper slot timing
	}

	return data;
}

/*
 * @brief: Writes data into DS2431 EEPROM memory.
 ***********EEPROM WRITING GUIDELINES**************
 * 1. Write in 8-byte chunks: You can only write full 8-byte rows at a time.
 * 2. Use 8-byte aligned addresses: Writes must start at addresses divisible by 8 (e.g., 0x00, 0x08, 0x10…).
 *
 * @param: uint16_t startaddress, start address of EEPROM write.
 * @param: uint8_t *pdata, pointer to data source buffer.
 */
void OneWire_WriteMemory(uint16_t startaddress, uint8_t *pdata)
{
	printf("\n Writing into 0x%X to 0x%X\n", startaddress, (startaddress+7) );

	uint16_t CRC16;

	//send reset pulse
	OneWire_Reset();

	//write skip ROM command
	OneWire_WriteByte(0xCC);

	//write scratch pad command
	OneWire_WriteByte(0x0F);

	//master TX TA1
	OneWire_WriteByte( (uint8_t) (startaddress & 0xFF) ); // TA1

	//master TX TA2
	OneWire_WriteByte( (uint8_t) ((startaddress >> 8) & 0xFF) ); // TA2

	//write 8 bytes of data into scratch pad
	for(uint8_t i = 0 ; i < 8 ; i++)
		OneWire_WriteByte(pdata[i]); // Data Byte

	//receive generated 16bit CRC by DS2431
	CRC16 = OneWire_ReadByte();
	CRC16 |= (OneWire_ReadByte() << 8);


	printf(" !!!Scratchpad written!!!\n Display Scratchpad Data\n");

	//Read scratchpad data to verify if it matches with user data
	Read_ScratchPad(pdata);

	//Copy scratchpad data to main EEPROM
	Copy_ScratchPad();

}

/*
 * @brief: Reads and displays data from DS2431 EEPROM memory.
 * @param: uint16_t startaddress, start address of EEPROM read.
 * @param: size_t bytesToRead, number of bytes to read from start address.
 * @param: uint8_t *pdataBuffer, pointer to data buffer where read data will be stored.
 */
void OneWire_ReadMemory(uint16_t startaddress, size_t bytesToRead, uint8_t *pdataBuffer)
{

	OneWire_Reset();

	//skip ROM command
	OneWire_WriteByte(0xCC);

	//Read memory command
	OneWire_WriteByte(0xF0);

	OneWire_WriteByte( (uint8_t) (startaddress & 0xFF) ); // TA1

	OneWire_WriteByte( (uint8_t) ((startaddress >> 8) & 0xFF) ); // TA2

	for(uint32_t i=0 ; i < bytesToRead ; i++)
	{
		//Save read byte to rxbuffer
		pdataBuffer[i] = OneWire_ReadByte();

	}
	OneWire_Reset();

	displaydata(startaddress, bytesToRead, pdataBuffer);
}


static void displaydata(uint16_t startaddress, size_t bytesToRead, uint8_t *pdataBuffer)
{

	printf("\n Read Data from Address: 0x%X to 0x%X\n", startaddress, ( (startaddress + bytesToRead) -1) );

	for(uint32_t i=0 ; i < bytesToRead ; i++)
	{
		printf(" 0x%X\n", pdataBuffer[i]);
	}
}

/*Sets 1-Wire GPIO as input mode*/
static void GPIOMode_Input(void)
{
	// Reconfigure the pin as input to allow slave to pull the bus low
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = ONE_WIRE_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_INPUT;  // Set pin to input mode
	GPIO_InitStruct.Pull = GPIO_NOPULL;     // No pull-up or pull-down
	HAL_GPIO_Init(ONE_WIRE_GPIO_PORT, &GPIO_InitStruct);
}

/*Sets 1-Wire GPIO as output mode*/
static void GPIOMode_Output(void)
{
	// Reconfigure the pin as input to allow slave to pull the bus low
	GPIO_InitTypeDef GPIO_InitStruct = {0};

	GPIO_InitStruct.Pin = ONE_WIRE_PIN;
	GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_OD;  // Set pin to input mode
	GPIO_InitStruct.Pull = GPIO_NOPULL;     // No pull-up or pull-down
	HAL_GPIO_Init(ONE_WIRE_GPIO_PORT, &GPIO_InitStruct);
}

/*
 * @brief: Reads and verifies scratchpad data from DS2431 EEPROM.
 * @param: uint8_t *pdata, pointer data source buffer to verify against scratchpad data.
 */
void Read_ScratchPad(uint8_t *pdata)
{
	uint16_t CRC16;

	OneWire_Reset();

	//ROM skip command
	OneWire_WriteByte(0xCC);

	//read scratch pad data
	OneWire_WriteByte(0xAA);

	//receive TA1, TA2, E/S BYTE & 8 BYTES OF DATA
	for(uint32_t i=0 ; i < 11 ; i++)
		readScratchPad[i] = OneWire_ReadByte();


	//receive generated 16bit CRC by DS2431
	CRC16 = OneWire_ReadByte();
	CRC16 |= (OneWire_ReadByte() << 8);

	OneWire_Reset();


	//Display scratchpad data
	for(uint32_t i=0 ; i < 11 ; i++)
		printf(" 0x%X\n",readScratchPad[i]);


	/*verify if user data matches with scratchpad data*/
	for(uint32_t i=0 ; i<8 ; i++)
	{
		if(readScratchPad[i+3] != pdata[i])
		{
			printf(" Error! scratch pad data does not match with written data!!\n");
			return;
		}
	}

	printf(" !!!User Data match with scratchpad data!!!\n");

}

/*Copies scratchpad data into main EEPROM*/
void Copy_ScratchPad()
{
	printf(" Writing into EEPROM in progress\n");

	OneWire_Reset();

	//ROM skip command
	OneWire_WriteByte(0xCC);

	//copy scratch pad data
	OneWire_WriteByte(0x55);

	//send TA1
	OneWire_WriteByte(readScratchPad[0]);

	//send TA2
	OneWire_WriteByte(readScratchPad[1]);

	//send E/S BYTE
	OneWire_WriteByte(readScratchPad[2]);

	//tprog allocation max 10ms.
	HAL_Delay(10);

	//check if copy data to memory address is success
	if(OneWire_ReadByte() == 0xAA)
	{
		OneWire_Reset();
		printf(" Successfully written data at memory address: 0x%X\n to 0x%X\n", ( readScratchPad[0] | (readScratchPad[1] << 8) ),\
																			  (( readScratchPad[0] | (readScratchPad[1] << 8) )+7) );

	}
	else
	{
		OneWire_Reset();
		printf(" Error writing at memory address: 0x%X\n to 0x%X\n",  ( readScratchPad[0] | (readScratchPad[1] << 8) ),\
				                                                     (( readScratchPad[0] | (readScratchPad[1] << 8) )+7) );

	}
}

/*
 * Blocking delay in microseconds
 */
void delay_us(uint32_t delay)
{
	volatile uint32_t timeStart = TIMER_INSTANCE->CNT;

	while (1) {
		volatile uint32_t currentTime = TIMER_INSTANCE->CNT;


		if (currentTime >= timeStart) {
			if ( (currentTime - timeStart) >= delay)
			{
				break;
			}
		}else{ 	// If timeStart > currentTime, Handle timer overflow
			if ( ( (TIMER_MAX - timeStart) + currentTime + 1) >= delay)
			{
				break;
			}
		}
	}
}


static void TIM6_init(void)
{
	htim6.Instance = TIMER_INSTANCE;
	htim6.Init.Prescaler = ( (HAL_RCC_GetPCLK1Freq() * 2) / 1e6) - 1; //GET 1Mhz TIM6 cnt_clk for 1us period
	htim6.Init.CounterMode = TIM_COUNTERMODE_UP;
	htim6.Init.Period = TIMER_MAX; //max counter value
	htim6.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;

	//implement low level MSP inits internal to driver not having to code in stm32 MSP init file
	tim_MSP_init();

	if (HAL_TIM_Base_Init(&htim6) != HAL_OK)
	{
		Error_Handler();
	}

	//start TIM2 upcounting, 1us period timebase
	HAL_TIM_Base_Start(&htim6);
}


//low level initialization for timer peripheral
static void tim_MSP_init(void)
{
	//if using other timers, modify the instance to desired timer
	if(htim6.Instance==TIMER_INSTANCE)
	{
		//ENABLE TIM2 PERIPHERAL CLOCK
		__HAL_RCC_TIM6_CLK_ENABLE();


	}
}
