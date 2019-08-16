#include "applibs_versions.h"
#include "ac_current_click.h"

#include <applibs/spi.h>
#include <applibs/log.h>


extern int spiFd = -1;


int ReadACCurrentBytes(uint8_t* byte_1, uint8_t* byte_2)
{
	static const uint8_t sampleCmd = 0x00;
	static const size_t transferCount = 3;
	SPIMaster_Transfer transfers[transferCount];

	int result = SPIMaster_InitTransfers(transfers, transferCount);
	if (result != 0) {
		return -1;
	}

	uint8_t byte1, byte2;

	transfers[0].flags = SPI_TransferFlags_Write;
	transfers[0].writeData = &sampleCmd;
	transfers[0].length = sizeof(sampleCmd);

	transfers[1].flags = SPI_TransferFlags_Read;
	transfers[1].readData = &byte1;
	transfers[1].length = sizeof(byte1);

	transfers[2].flags = SPI_TransferFlags_Read;
	transfers[2].readData = &byte2;
	transfers[2].length = sizeof(byte2);

	ssize_t transferredBytes = SPIMaster_TransferSequential(spiFd, transfers, transferCount);

	*byte_1 = byte1;
	*byte_2 = byte2;

	return transferredBytes;
}

float GetCurrentAC(void)
{
	uint8_t byte_1 = 0, byte_2 = 0;
	
	ReadACCurrentBytes(&byte_1, &byte_2);

	uint16_t msb_1 = byte_2;
	msb_1 = msb_1 >> 1;

	uint16_t msb_0 = byte_1 & 0b00011111;
	msb_0 = msb_0 << 7;

	uint16_t msb = msb_0 + msb_1;

	float average = (float)(msb / 4095.0) * 2.048;

	Log_Debug("INFO: byte_1 %d, byte_2 %d, ac %f \n", byte_1, byte_2, average);

	return average;
}