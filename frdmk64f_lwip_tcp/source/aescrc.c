/*****************************************************************************/
/* Includes:                                                                 */
/*****************************************************************************/
#include "lwip/memp.h"
#include "aescrc.h"

/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Private variables:                                                        */
/*****************************************************************************/

/*****************************************************************************/
/* Private functions:                                                        */
/*****************************************************************************/
void InitCRC32(void)
{
    crc_config_t config;

    config.polynomial         = 0x04C11DB7U;
    config.seed               = SEED;
    config.reflectIn          = true;
    config.reflectOut         = true;
    config.complementChecksum = true;
    config.crcBits            = kCrcBits32;
    config.crcResult          = kCrcFinalChecksum;

    CRC_Init(CRC0, &config);
}

void SetAESCRC(uint8_t *plaintext, uint8_t padded_msg[]){
	//AES variables
	struct AES_ctx ctx;
	size_t stringLen, padded_len;
	//uint8_t ciphertext[512] = {0};
	//CRC variables
	size_t cipherLen;
	uint32_t checkSum32;
	uint8_t *crc;

	//AES
	AES_init_ctx_iv(&ctx, KEY, IV);
	stringLen = strlen(plaintext);
	padded_len = stringLen + (16 - (stringLen%16) );
	memcpy(padded_msg, plaintext, stringLen);
	AES_CBC_encrypt_buffer(&ctx, padded_msg, padded_len);

	//CRC32
	InitCRC32();
	CRC_WriteData(CRC0, padded_msg, padded_len);
	checkSum32 = CRC_Get32bitResult(CRC0);
	PRINTF("CRC-32: 0x%08x\r\n", checkSum32);
	//Adding CRC-32 to the end of ciphertext
	crc = (uint8_t*)&checkSum32;
	crc++;
	crc++;
	crc++;
	for(int i=0;i<4;i++){
		padded_msg[padded_len+i] = *crc;
		crc--;
	}
}

void UnsetAESCRC(uint8_t *plaintext, uint8_t padded_msg[]){
	struct AES_ctx ctx;
	size_t test_string_len, padded_len;

	/* Init the AES context structure */
	AES_init_ctx_iv(&ctx, KEY, IV);
	/* To encrypt an array its lenght must be a multiple of 16 so we add zeros */
	test_string_len = strlen(plaintext);
	padded_len = test_string_len + (16 - (test_string_len%16) );
	memcpy(padded_msg, plaintext, test_string_len);

	AES_CBC_decrypt_buffer(&ctx, padded_msg, padded_len);
}
