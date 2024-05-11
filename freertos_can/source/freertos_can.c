/*
 * Copyright (c) 2015, Freescale Semiconductor, Inc.
 * Copyright 2016-2017 NXP
 * All rights reserved.
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/* FreeRTOS kernel includes. */
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "timers.h"

/* Freescale includes. */
#include "fsl_device_registers.h"
#include "fsl_debug_console.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"

#include "fsl_flexcan.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define EXAMPLE_CAN            CAN0
#define EXAMPLE_CAN_CLK_SOURCE (kFLEXCAN_ClkSrc1)
#define EXAMPLE_CAN_CLK_FREQ   CLOCK_GetFreq(kCLOCK_BusClk)
/* Set USE_IMPROVED_TIMING_CONFIG macro to use api to calculates the improved CAN / CAN FD timing values. */
#define USE_IMPROVED_TIMING_CONFIG (1U)
#define EXAMPLE_FLEXCAN_IRQn       CAN0_ORed_Message_buffer_IRQn
#define EXAMPLE_FLEXCAN_IRQHandler CAN0_ORed_Message_buffer_IRQHandler
#define RX_MESSAGE_BUFFER_0x10     (9)
#define RX_MESSAGE_BUFFER_0x11     (10)
#define TX_MESSAGE_BUFFER_NUM      (8)
#define DLC                        (2)
#define MAX_VAL_UINT12				4095
#define MIN_VAL_UINT12				0
#define STEP						200
#define FLAG						1U
#define QUEUE_ITEMS					10

/* Fix MISRA_C-2012 Rule 17.7. */
#define LOG_INFO (void)PRINTF

/* Task priorities. */
#define can_tx_task_PRIORITY (configMAX_PRIORITIES - 1)
#define can_rx_task_PRIORITY (configMAX_PRIORITIES - 1)
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
static void can_tx_task(void *pvParameters);
static void can_rx_task(void *pvParameters);
void can_init(void);
void changeEndianess(uint32_t *word0, uint32_t *word1);

/*******************************************************************************
 * Global variables
 ******************************************************************************/
flexcan_frame_t txFrame, rxFrame;
uint16_t period = 500;

static QueueHandle_t log_queue = NULL;

/*******************************************************************************
 * Interruptions
 ******************************************************************************/
void EXAMPLE_FLEXCAN_IRQHandler(void)
{
    /* If new data arrived. */
    if (0U != FLEXCAN_GetMbStatusFlags(EXAMPLE_CAN, FLAG << RX_MESSAGE_BUFFER_0x10))
    {
        FLEXCAN_ClearMbStatusFlags(EXAMPLE_CAN, FLAG << RX_MESSAGE_BUFFER_0x10);
        (void)FLEXCAN_ReadRxMb(EXAMPLE_CAN, RX_MESSAGE_BUFFER_0x10, &rxFrame);
        rxFrame.id = 0x10;
    }
    if (0U != FLEXCAN_GetMbStatusFlags(EXAMPLE_CAN, FLAG << RX_MESSAGE_BUFFER_0x11))
	{
		FLEXCAN_ClearMbStatusFlags(EXAMPLE_CAN, FLAG << RX_MESSAGE_BUFFER_0x11);
		(void)FLEXCAN_ReadRxMb(EXAMPLE_CAN, RX_MESSAGE_BUFFER_0x11, &rxFrame);
		rxFrame.id = 0x11;
	}
    xQueueSendFromISR(log_queue, (void *)&rxFrame, 0);

    SDK_ISR_EXIT_BARRIER;
}

/*******************************************************************************
 * Code
 ******************************************************************************/
/*!
 * @brief Application entry point.
 */
int main(void)
{
	uint8_t period = 1;
	uint16_t adc = 0;

	/* Define the init structure for the input switch pin */
	gpio_pin_config_t sw_config = {
		kGPIO_DigitalInput,
		0,
	};

	gpio_pin_config_t led_config = {
		kGPIO_DigitalOutput,
		0,
	};

	log_queue = xQueueCreate(QUEUE_ITEMS, sizeof(rxFrame));
	/* Enable queue view in MCUX IDE FreeRTOS TAD plugin. */
	if (log_queue != NULL)
	{
		vQueueAddToRegistry(log_queue, "LogQ");
	}

    /* Init board hardware. */
	NVIC_SetPriority(CAN0_ORed_Message_buffer_IRQn, 5);
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();

    /* Init output LEDs GPIO */
	GPIO_PinInit(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, &led_config);
	GPIO_PinInit(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PIN, &led_config);
	GPIO_PinInit(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PIN, &led_config);

	/* Turn off all LEDs */
	GPIO_PinWrite(BOARD_LED_RED_GPIO, BOARD_LED_RED_GPIO_PIN, 1);
	GPIO_PinWrite(BOARD_LED_GREEN_GPIO, BOARD_LED_GREEN_GPIO_PIN, 1);
	GPIO_PinWrite(BOARD_LED_BLUE_GPIO, BOARD_LED_BLUE_GPIO_PIN, 1);

    can_init();

    if (xTaskCreate(can_tx_task, "CAN_Tx_task", configMINIMAL_STACK_SIZE + 100, NULL, can_tx_task_PRIORITY, NULL) !=
        pdPASS)
    {
        PRINTF("Task creation failed!.\r\n");
        while (1)
            ;
    }
    if (xTaskCreate(can_rx_task, "CAN_Rx_task", configMINIMAL_STACK_SIZE + 100, NULL, can_rx_task_PRIORITY, NULL) !=
		pdPASS)
	{
		PRINTF("Task creation failed!.\r\n");
		while (1)
			;
	}
    vTaskStartScheduler();
    for (;;)
        ;
}

static void can_tx_task(void *pvParameters)
{
    for (;;){
    	PRINTF("Tx\r\n");
    	/* Prepare Tx Frame for sending. */
		txFrame.format = (uint8_t)kFLEXCAN_FrameFormatStandard;
		txFrame.type   = (uint8_t)kFLEXCAN_FrameTypeData;
		txFrame.id     = FLEXCAN_ID_STD(0x20);
		txFrame.length = (uint8_t)DLC;
		vTaskDelay(period);


        //vTaskSuspend(NULL);
    }
}

static void can_rx_task(void *pvParameters)
{
	flexcan_frame_t localBuffer;

    for (;;){
    	xQueueReceive(log_queue, &localBuffer, portMAX_DELAY);
    	changeEndianess(&localBuffer.dataWord0, &localBuffer.dataWord1);

    	switch(localBuffer.id){
    	case 0x10:
    		break;
    	}
    }
}

void can_init(){
	flexcan_config_t flexcanConfig;
	flexcan_rx_mb_config_t mbConfig;
	flexcan_rx_mb_config_t mbConfig1;

	LOG_INFO("\r\n==FlexCAN loopback functional example -- Start.==\r\n\r\n");

	/* Init FlexCAN module. */
	/*
	 * flexcanConfig.clkSrc                 = kFLEXCAN_ClkSrc0;
	 * flexcanConfig.baudRate               = 1000000U;
	 * flexcanConfig.baudRateFD             = 2000000U;
	 * flexcanConfig.maxMbNum               = 16;
	 * flexcanConfig.enableLoopBack         = false;
	 * flexcanConfig.enableSelfWakeup       = false;
	 * flexcanConfig.enableIndividMask      = false;
	 * flexcanConfig.disableSelfReception   = false;
	 * flexcanConfig.enableListenOnlyMode   = false;
	 * flexcanConfig.enableDoze             = false;
	 */
	FLEXCAN_GetDefaultConfig(&flexcanConfig);

	flexcanConfig.clkSrc = EXAMPLE_CAN_CLK_SOURCE;

	//flexcanConfig.enableLoopBack = true;
	flexcanConfig.baudRate             = 125000U;
	flexcan_timing_config_t timing_config;
	memset(&timing_config, 0, sizeof(flexcan_timing_config_t));
	if (FLEXCAN_CalculateImprovedTimingValues(EXAMPLE_CAN, flexcanConfig.baudRate, EXAMPLE_CAN_CLK_FREQ,
											  &timing_config))
	{
		/* Update the improved timing configuration*/
		memcpy(&(flexcanConfig.timingConfig), &timing_config, sizeof(flexcan_timing_config_t));
	}
	else
	{
		LOG_INFO("No found Improved Timing Configuration. Just used default configuration\r\n\r\n");
	}

	FLEXCAN_Init(EXAMPLE_CAN, &flexcanConfig, EXAMPLE_CAN_CLK_FREQ);

	/* Setup Rx Message Buffer. */
	mbConfig.format = kFLEXCAN_FrameFormatStandard;
	mbConfig.type   = kFLEXCAN_FrameTypeData;
	mbConfig.id     = FLEXCAN_ID_STD(0x10);

	FLEXCAN_SetRxMbConfig(EXAMPLE_CAN, RX_MESSAGE_BUFFER_0x10, &mbConfig, true);

	/* Setup Rx Message Buffer. */
	mbConfig1.format = kFLEXCAN_FrameFormatStandard;
	mbConfig1.type   = kFLEXCAN_FrameTypeData;
	mbConfig1.id     = FLEXCAN_ID_STD(0x11);

	FLEXCAN_SetRxMbConfig(EXAMPLE_CAN, RX_MESSAGE_BUFFER_0x11, &mbConfig1, true);

	/* Setup Tx Message Buffer. */
	FLEXCAN_SetTxMbConfig(EXAMPLE_CAN, TX_MESSAGE_BUFFER_NUM, true);

	/* Enable Rx Message Buffer interrupt. */
	FLEXCAN_EnableMbInterrupts(EXAMPLE_CAN, FLAG << RX_MESSAGE_BUFFER_0x10);
	FLEXCAN_EnableMbInterrupts(EXAMPLE_CAN, FLAG << RX_MESSAGE_BUFFER_0x11);
	(void)EnableIRQ(EXAMPLE_FLEXCAN_IRQn);
}

void changeEndianess(uint32_t *word0, uint32_t *word1){
	uint32_t tempWord;

	tempWord = (*word0 & 0xFF000000) >> 24;
	tempWord = (tempWord | (*word0 & 0x00FF0000) >> 8);
	tempWord = (tempWord | (*word0 & 0x0000FF00) << 8);
	tempWord = (tempWord | (*word0 & 0x000000FF) << 24);
	*word0 = tempWord;

	tempWord = (*word1 & 0xFF000000) >> 24;
	tempWord = (tempWord | (*word1 & 0x00FF0000) >> 8);
	tempWord = (tempWord | (*word1 & 0x0000FF00) << 8);
	tempWord = (tempWord | (*word1 & 0x000000FF) << 24);
	*word1 = tempWord;
}
