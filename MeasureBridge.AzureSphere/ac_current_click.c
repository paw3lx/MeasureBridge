#include "applibs_versions.h"
#include "ac_current_click.h"

#include <applibs/spi.h>
#include <applibs/log.h>

extern int spiFd;
void HAL_Delay(int delayTime);

int read_ac_current_bytes(uint8_t* byte_1, uint8_t* byte_2)
{
	static const uint8_t sampleCmd = 0x00;
	static const size_t transferCount = 2;
	SPIMaster_Transfer transfers[transferCount];

	int result = SPIMaster_InitTransfers(transfers, transferCount);
	if (result != 0) {
		return -1;
	}

	uint8_t byte1, byte2;

	//transfers[0].flags = SPI_TransferFlags_Write;
	//transfers[0].writeData = &sampleCmd;
	//transfers[0].length = sizeof(sampleCmd);

	transfers[0].flags = SPI_TransferFlags_Read;
	transfers[0].readData = &byte1;
	transfers[0].length = sizeof(byte1);

	transfers[1].flags = SPI_TransferFlags_Read;
	transfers[1].readData = &byte2;
	transfers[1].length = sizeof(byte2);

	ssize_t transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);

	*byte_1 = byte1;
	*byte_2 = byte2;

	return transferredBytes;
}

float get_current_adc(void)
{
	uint8_t byte_1 = 0, byte_2 = 0;
	
	read_ac_current_bytes(&byte_1, &byte_2);

	Log_Debug("byte1 = %d, byte2 = %d \n", byte_1, byte_2);

	uint16_t msb_1 = byte_2;
	msb_1 = msb_1 >> 1; // shift right 1 bit to remove B01

	uint16_t msb_0 = byte_1 & 0b00011111; // mask the 2 unknown bits and the null bit
	msb_0 = msb_0 << 7; // shift left 7 bits

	uint16_t msb = msb_0 + msb_1;

	return msb;
}

float get_current_ac(uint8_t measurements)
{
	float average = 0.0;
	for (uint8_t i = 0; i < measurements; i++) {
		float adc = get_current_adc();
		Log_Debug("adc = %f \n", adc);
		average += adc;
		HAL_Delay(100);
	}

	average = (float) average / measurements;

	float ac = (float)(average / 4095.0) * 2.048;

	ac = (float)ac / 8,5 * 30.0;

	return ac;
}

void HAL_Delay(int delayTime)
{
	struct timespec ts;
	ts.tv_sec = 0;
	ts.tv_nsec = delayTime * 10000;
	nanosleep(&ts, NULL);
}