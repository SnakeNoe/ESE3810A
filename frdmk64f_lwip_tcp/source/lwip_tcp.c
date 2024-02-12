/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include <stdio.h>
#include "lwip/opt.h"
#include "lwip/sys.h"
#include "lwip/api.h"

#if LWIP_NETCONN

#include "tcpecho.h"
#include "lwip/netifapi.h"
#include "lwip/tcpip.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_phy.h"

#include "fsl_phyksz8081.h"
#include "fsl_enet_mdio.h"
#include "fsl_device_registers.h"

#include "aescrc.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define CRC 4
#define PORT 7

/* @TEST_ANCHOR */

/* IP address configuration. */
#ifndef configIP_ADDR0
#define configIP_ADDR0 192
#endif
#ifndef configIP_ADDR1
#define configIP_ADDR1 168
#endif
#ifndef configIP_ADDR2
#define configIP_ADDR2 0
#endif
#ifndef configIP_ADDR3
#define configIP_ADDR3 102
#endif

/* Netmask configuration. */
#ifndef configNET_MASK0
#define configNET_MASK0 255
#endif
#ifndef configNET_MASK1
#define configNET_MASK1 255
#endif
#ifndef configNET_MASK2
#define configNET_MASK2 255
#endif
#ifndef configNET_MASK3
#define configNET_MASK3 0
#endif

/* Gateway address configuration. */
#ifndef configGW_ADDR0
#define configGW_ADDR0 192
#endif
#ifndef configGW_ADDR1
#define configGW_ADDR1 168
#endif
#ifndef configGW_ADDR2
#define configGW_ADDR2 0
#endif
#ifndef configGW_ADDR3
#define configGW_ADDR3 100
#endif

/* MAC address configuration. */
#ifndef configMAC_ADDR
#define configMAC_ADDR                     \
    {                                      \
        0x02, 0x12, 0x13, 0x10, 0x15, 0x11 \
    }
#endif

/* Address of PHY interface. */
#define EXAMPLE_PHY_ADDRESS BOARD_ENET0_PHY_ADDRESS

/* MDIO operations. */
#define EXAMPLE_MDIO_OPS enet_ops

/* PHY operations. */
#define EXAMPLE_PHY_OPS phyksz8081_ops

/* ENET clock frequency. */
#define EXAMPLE_CLOCK_FREQ CLOCK_GetFreq(kCLOCK_CoreSysClk)

#ifndef EXAMPLE_NETIF_INIT_FN
/*! @brief Network interface initialization function. */
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif /* EXAMPLE_NETIF_INIT_FN */

/*! @brief Stack size of the temporary lwIP initialization thread. */
#define INIT_THREAD_STACKSIZE 1024

/*! @brief Priority of the temporary lwIP initialization thread. */
#define INIT_THREAD_PRIO DEFAULT_THREAD_PRIO

/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void aescrc_test(void);
//void bytes2Byte(uint32_t, uint8_t *);
void decToHex(uint32_t, uint8_t *);

/*******************************************************************************
 * Variables
 ******************************************************************************/
static mdio_handle_t mdioHandle = {.ops = &EXAMPLE_MDIO_OPS};
static phy_handle_t phyHandle   = {.phyAddr = EXAMPLE_PHY_ADDRESS, .mdioHandle = &mdioHandle, .ops = &EXAMPLE_PHY_OPS};

/*******************************************************************************
 * Code
 ******************************************************************************/

/*!
 * @brief Initializes lwIP stack.
 *
 * @param arg unused
 */
static void stack_init(void *arg)
{
    static struct netif netif;
    ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
    ethernetif_config_t enet_config = {
        .phyHandle  = &phyHandle,
        .macAddress = configMAC_ADDR,
    };

    LWIP_UNUSED_ARG(arg);

    mdioHandle.resource.csrClock_Hz = EXAMPLE_CLOCK_FREQ;

    IP4_ADDR(&netif_ipaddr, configIP_ADDR0, configIP_ADDR1, configIP_ADDR2, configIP_ADDR3);
    IP4_ADDR(&netif_netmask, configNET_MASK0, configNET_MASK1, configNET_MASK2, configNET_MASK3);
    IP4_ADDR(&netif_gw, configGW_ADDR0, configGW_ADDR1, configGW_ADDR2, configGW_ADDR3);

    tcpip_init(NULL, NULL);

    netifapi_netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN,
                       tcpip_input);
    netifapi_netif_set_default(&netif);
    netifapi_netif_set_up(&netif);

    PRINTF("\r\n************************************************\r\n");
    PRINTF(" TCP Echo example\r\n");
    PRINTF("************************************************\r\n");
    PRINTF(" IPv4 Address     : %u.%u.%u.%u\r\n", ((u8_t *)&netif_ipaddr)[0], ((u8_t *)&netif_ipaddr)[1],
           ((u8_t *)&netif_ipaddr)[2], ((u8_t *)&netif_ipaddr)[3]);
    PRINTF(" IPv4 Subnet mask : %u.%u.%u.%u\r\n", ((u8_t *)&netif_netmask)[0], ((u8_t *)&netif_netmask)[1],
           ((u8_t *)&netif_netmask)[2], ((u8_t *)&netif_netmask)[3]);
    PRINTF(" IPv4 Gateway     : %u.%u.%u.%u\r\n", ((u8_t *)&netif_gw)[0], ((u8_t *)&netif_gw)[1],
           ((u8_t *)&netif_gw)[2], ((u8_t *)&netif_gw)[3]);
    PRINTF("************************************************\r\n\n");

    aescrc_test();

    vTaskDelete(NULL);
}

/*!
 * @brief Main function
 */
int main(void)
{
	uint8_t key[] = { 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06 };
	uint8_t iv[]  = { 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 };
    SYSMPU_Type *base = SYSMPU;
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    /* Disable SYSMPU. */
    base->CESR &= ~SYSMPU_CESR_VLD_MASK;

    /* Initialize lwIP from thread */
    if (sys_thread_new("main", stack_init, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
    {
        LWIP_ASSERT("main(): Task creation failed.", 0);
    }

    vTaskStartScheduler();

    /* Will not get here unless a task calls vTaskEndScheduler ()*/
    return 0;
}
#endif

void aescrc_test(void){
	struct netconn *conn, *newconn;
	struct netbuf *buf;
	err_t err;

	conn = netconn_new(NETCONN_TCP);
	netconn_bind(conn, IP_ADDR_ANY, PORT);
	netconn_listen(conn);

	while(1){
		err = netconn_accept(conn, &newconn);
		if(err == ERR_OK){
			void *data;
			u16_t len;
			uint8_t ciphertext[512] = {0};
			uint8_t deciphertext[512] = {0};
			uint8_t transBuffer[512] = {0};

		    while((err = netconn_recv(newconn, &buf)) == ERR_OK){
		    	do{
		    		netbuf_data(buf, &data, &len);
		            PRINTF("Received: %s\r\n", data);

		            //Encripting
		            SetAESCRC(data, ciphertext);
		            PRINTF("Encrypted Message: ");
					for(int i=0; i<20; i++) {
						PRINTF("0x%02x ", ciphertext[i]);
					}
					PRINTF("\r\n\n");

					//Decrypting
					UnsetAESCRC(ciphertext, deciphertext);
					PRINTF("Decrypted Message: ");
					for(int i=0; i<16; i++) {
						PRINTF("%c ", deciphertext[i]);
					}
					PRINTF("\r\n\n");


					PRINTF("Transmitted: ");
					for(int i=0; i<16+CRC; i++) {
						PRINTF("0x%02x ", ciphertext[i]);
					}
					PRINTF("\r\n\n");

					/* Transform bytes (uint32_t) to byte (uint8_t) before transmit */
					for(int i=0,j=0;i<16+CRC;i++){
						decToHex(ciphertext[i], &transBuffer[j]);
						//bytes2Byte(ciphertext[i], &transBuffer[j]);
						j += 2;
					}

					/*PRINTF("Transmitted in Hex: ");
					for(int i=0; i<(cipherLen+CRC)*2; i++) {
						PRINTF("0x%02x ", transBuffer[i]);
					}
					PRINTF("\r\n\n");*/

					/* Since encrypted data is of size uint32_t,
					 * but to transmit they were converted to char, is double of length */
					err = netconn_write(newconn, transBuffer, (16+CRC)*2, NETCONN_COPY);
					//err = netconn_write(newconn, ciphertext, cipherLen+CRC, NETCONN_COPY);
		        }while (netbuf_next(buf) >= 0);
		        netbuf_delete(buf);
		    }
			/* Close connection and discard connection identifier. */
			netconn_close(newconn);
			netconn_delete(newconn);
		}
	}
}

/*void bytes2Byte(uint32_t bytes, uint8_t *byte){
	uint16_t highMask = 0b11110000;
	uint8_t lowMask = 0b1111;

	*byte = (uint8_t)((bytes & highMask) >> 4);
	byte++;
	*byte = (uint8_t)(bytes & lowMask);
}*/

void decToHex(uint32_t decimal, uint8_t *result){
	uint8_t hexadecimal[2];
	uint32_t i = 0;

    if(decimal){
		while (decimal > 0){
			uint32_t remainder = decimal % 16;
			if (remainder < 10){
				hexadecimal[i++] = remainder + '0';
			}
			else{
				hexadecimal[i++] = remainder + 'A' - 10;
			}
			decimal /= 16;
		}
    }
    else{
    	hexadecimal[0] = 0;
    	hexadecimal[1] = 0;
    }

    *result = hexadecimal[1];
	result++;
	*result = hexadecimal[0];
}
