/*****************************************************************************/
/* Includes:                                                                 */
/*****************************************************************************/
#include "crc.h"

/*****************************************************************************/
/* Defines:                                                                  */
/*****************************************************************************/

/*****************************************************************************/
/* Private variables:                                                        */
/*****************************************************************************/

/*****************************************************************************/
/* Private functions:                                                        */
/*****************************************************************************/

/*!
 * @brief Init for CRC-32.
 * @details Init CRC peripheral module for CRC-32 protocol.
 *          width=32 poly=0x04c11db7 init=0xffffffff refin=true refout=true xorout=0xffffffff check=0xcbf43926
 *          name="CRC-32"
 *          http://reveng.sourceforge.net/crc-catalogue/
 */
void InitCrc32(CRC_Type *base, uint32_t seed)
{
    crc_config_t config;

    config.polynomial         = 0x04C11DB7U;
    config.seed               = seed;
    config.reflectIn          = true;
    config.reflectOut         = true;
    config.complementChecksum = true;
    config.crcBits            = kCrcBits32;
    config.crcResult          = kCrcFinalChecksum;

    CRC_Init(base, &config);
}

/*!
 * brief Writes data to the CRC module.
 *
 * Writes input data buffer bytes to the CRC data register.
 * The configured type of transpose is applied.
 *
 * param base CRC peripheral address.
 * param data Input data stream, MSByte in data[0].
 * param dataSize Size in bytes of the input data buffer.
 */
void Write_CRC(CRC_Type *base, const uint8_t *data, size_t dataSize)
{
	CRC_WriteData(base, data, dataSize);
}

/*!
 * brief Reads the 32-bit checksum from the CRC module.
 *
 * Reads the CRC data register (either an intermediate or the final checksum).
 * The configured type of transpose and complement is applied.
 *
 * param base CRC peripheral address.
 * return An intermediate or the final 32-bit checksum, after configured transpose and complement operations.
 */
uint32_t Get_CRC(CRC_Type *base)
{
	return CRC_Get32bitResult(base);
}
