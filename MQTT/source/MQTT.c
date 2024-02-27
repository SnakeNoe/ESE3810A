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
#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_RAW && LWIP_NETCONN && LWIP_DHCP && LWIP_DNS

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_phy.h"

#include "lwip/api.h"
#include "lwip/apps/mqtt.h"
#include "lwip/dhcp.h"
#include "lwip/netdb.h"
#include "lwip/netifapi.h"
#include "lwip/prot/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"
#include "lwip_mqtt_id.h"

#include "ctype.h"
#include "stdio.h"

#include "fsl_phyksz8081.h"
#include "fsl_enet_mdio.h"
#include "fsl_device_registers.h"
/*******************************************************************************
 * Definitions
 ******************************************************************************/
//To use driver.cloudmqtt.com broker otherwise, broker.hivemq.com
#define CLOUD_MQTT

#define TIME 30000000

#define TOTAL_POSTS 5

/* @TEST_ANCHOR */

/* MAC address configuration. */
#ifndef configMAC_ADDR
#define configMAC_ADDR                     \
    {                                      \
        0x02, 0x12, 0x13, 0x10, 0x15, 0x03 \
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

/* GPIO pin configuration. */
#define BOARD_LED_GPIO       BOARD_LED_GREEN_GPIO
#define BOARD_LED_GPIO_PIN   BOARD_LED_GREEN_GPIO_PIN
#define BOARD_SW_GPIO        BOARD_SW3_GPIO
#define BOARD_SW_GPIO_PIN    BOARD_SW3_GPIO_PIN
#define BOARD_SW_PORT        BOARD_SW3_PORT
#define BOARD_SW_IRQ         BOARD_SW3_IRQ
#define BOARD_SW_IRQ_HANDLER BOARD_SW3_IRQ_HANDLER

#ifndef EXAMPLE_NETIF_INIT_FN
/*! @brief Network interface initialization function. */
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif /* EXAMPLE_NETIF_INIT_FN */

#ifdef CLOUD_MQTT
#define EXAMPLE_MQTT_SERVER_HOST "driver.cloudmqtt.com"
#else
/*! @brief MQTT server host name or IP address. */
#define EXAMPLE_MQTT_SERVER_HOST "broker.hivemq.com"
#endif

/*! @brief MQTT server port number. */
#ifdef CLOUD_MQTT
#define EXAMPLE_MQTT_SERVER_PORT 18591
#else
#define EXAMPLE_MQTT_SERVER_PORT 1883
#endif

/*! @brief Stack size of the temporary lwIP initialization thread. */
#define INIT_THREAD_STACKSIZE 1024

/*! @brief Priority of the temporary lwIP initialization thread. */
#define INIT_THREAD_PRIO DEFAULT_THREAD_PRIO

/*! @brief Stack size of the temporary initialization thread. */
#define APP_THREAD_STACKSIZE 1024

/*! @brief Priority of the temporary initialization thread. */
#define APP_THREAD_PRIO DEFAULT_THREAD_PRIO
/*******************************************************************************
 * Prototypes
 ******************************************************************************/
void delay(uint32_t delay);
void stateFeedback(char state[]);
void mock_getPollution(char pollution[]);
void mock_getRadiation(char radiation[]);
void mock_getPrecipitation(char precipitation[]);
void mock_getWindSpeed(char speed[]);
static void connect_to_mqtt(void *ctx);

/*******************************************************************************
 * Variables
 ******************************************************************************/
bool gNewPost = false;
uint8_t gIndex = 0;
typedef struct{
	uint8_t state;				//Subscriber: 0 = Off, 1 = On
	uint16_t pollution;			//Publisher: 500ppm - 1500ppm
	uint16_t pyranometer;		//Publisher: 0 W/m2 - 1800 W/m2
	float pluviometer;			//Publisher: 0.1mm/h - 60mm/h
	uint8_t anemometer;			//Publisher: 0 km/h - 180km/h; - 0 MPH - 112 MPH
	uint8_t monitor;			//Subscriber: 0 = Monitor off, 1 = Monitor on
	uint8_t anemometerUnit;		//Subscriber: 0 = km/h, 1 = MPH
}topics;
topics sTopics = {0};

typedef struct{
	char *topic;
	char payload;
}topicSubscribed;
topicSubscribed sSubscribedInfo = {0};

static mdio_handle_t mdioHandle = {.ops = &EXAMPLE_MDIO_OPS};
static phy_handle_t phyHandle   = {.phyAddr = EXAMPLE_PHY_ADDRESS, .mdioHandle = &mdioHandle, .ops = &EXAMPLE_PHY_OPS};

/*! @brief MQTT client data. */
static mqtt_client_t *mqtt_client;

/*! @brief MQTT client ID string. */
static char client_id[40];

/*! @brief MQTT client information. */
static const struct mqtt_connect_client_info_t mqtt_client_info = {
    .client_id   = (const char *)&client_id[0],
#ifdef CLOUD_MQTT
    .client_user = "noe_p2024",
    .client_pass = "p2024",
#else
	.client_user = NULL,
	.client_pass = NULL,
#endif
    .keep_alive  = 100,
    .will_topic  = NULL,
    .will_msg    = NULL,
    .will_qos    = 0,
    .will_retain = 0,
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    .tls_config = NULL,
#endif
};

/*! @brief MQTT broker IP address. */
static ip_addr_t mqtt_addr;

/*! @brief Indicates connection to MQTT broker. */
static volatile bool connected = false;

/*******************************************************************************
 * Code
 ******************************************************************************/
void initStation(void){
	sSubscribedInfo.topic = malloc(50);
}

void delay(uint32_t delay){
    volatile uint32_t i = delay;
    for (;i>0;i--){
        __asm("NOP"); /* delay */
    }
}

void extractSubscribedData(void){
	if(!strcmp(sSubscribedInfo.topic, "noe_station/state/")){
		sTopics.state = (uint8_t)sSubscribedInfo.payload - 48;
	}
	else if(!strcmp(sSubscribedInfo.topic, "noe_station/monitor/")){
		sTopics.monitor = (uint8_t)sSubscribedInfo.payload - 48;
	}
	else if(!strcmp(sSubscribedInfo.topic, "noe_station/sensors/anemometer/unit/")){
		sTopics.anemometerUnit = (uint8_t)sSubscribedInfo.payload - 48;
	}
	else{
		PRINTF("ERROR: Topic [%s] not recognized!", sSubscribedInfo.topic);
	}
	gNewPost = false;
}

/*!
 * @brief Return state value integer to string
 * @param feedback Array where stores state (on/off) converted to string ready to be transmitted by MQTT
 */
void getStateFeedback(char feedback[]){
	delay(8000000);
	sprintf(feedback, "%d", sTopics.state);
}

/*!
 * @brief Imitates a function that senses pollution
 * @param pollution Array where stores pollution value converted to string ready to be transmitted by MQTT
 */
void mock_getPollution(char pollution[]){
	static const uint16_t sensor[5] = {600, 602, 578, 588, 610};

	delay(8000000);
	sTopics.pollution = sensor[gIndex];

	sprintf(pollution, "%d", sTopics.pollution);

	//Subscribed topic: noe_station/monitor
	if(sTopics.monitor){
		PRINTF("CO: %s\r\n", pollution);
	}
}

/*!
 * @brief Imitates a function that senses solar radiation
 * @param radiation Array where stores radiation value converted to string ready to be transmitted by MQTT
 */
void mock_getRadiation(char radiation[]){
	static const uint16_t sensor[5] = {1000, 1002, 1001, 999, 1003};

	delay(8000000);
	sTopics.pyranometer = sensor[gIndex];

	sprintf(radiation, "%d", sTopics.pyranometer);

	//Subscribed topic: noe_station/monitor
	if(sTopics.monitor){
		PRINTF("Solar radiation: %s\r\n", radiation);
	}
}

/*!
 * @brief Imitates a function that senses precipitation
 * @param precipitation Array where stores precipitation value converted to string ready to be transmitted by MQTT
 */
void mock_getPrecipitation(char precipitation[]){
	static const float sensor[5] = {1.1, 1.3, 1.5, 1.7, 1.9};

	delay(8000000);
	sTopics.pluviometer = sensor[gIndex];

	sprintf(precipitation, "%.2f", sTopics.pluviometer);

	//Subscribed topic: noe_station/monitor
	if(sTopics.monitor){
		PRINTF("Precipitation: %s\r\n", precipitation);
	}
}

/*!
 * @brief Imitates a function that senses wind speed
 * @param speed Array where stores wind speed value converted to string ready to be transmitted by MQTT
 */
void mock_getWindSpeed(char speed[]){
	static const uint8_t kilometers[5] = {40, 47, 33, 39, 41};
	static const uint8_t miles[5] = {25, 26, 28, 19, 22};

	delay(8000000);
	//Subscribed topic: noe_station/sensors/anemometer/anemometerUnit
	if(!sTopics.anemometerUnit){
		sTopics.anemometer = kilometers[gIndex];
	}
	else if(sTopics.anemometerUnit){
		sTopics.anemometer = miles[gIndex];
	}
	else{
		sTopics.anemometerUnit = 0;
	}

	sprintf(speed, "%d", sTopics.anemometer);

	//Subscribed topic: noe_station/monitor
	if(sTopics.monitor){
		PRINTF("Wind speed: %s\r\n", speed);
	}

	//To change mock sensor values
	gIndex++;
	if(gIndex == TOTAL_POSTS){
		gIndex = 0;
	}
}

/*!
 * @brief Called when subscription request finishes.
 */
static void mqtt_topic_subscribed_cb(void *arg, err_t err)
{
    const char *topic = (const char *)arg;

    if (err == ERR_OK)
    {
        PRINTF("Subscribed to the topic \"%s\".\r\n", topic);
    }
    else
    {
        PRINTF("Failed to subscribe to the topic \"%s\": %d.\r\n", topic, err);
    }
}

/*!
 * @brief Called when there is a message on a subscribed topic.
 */
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    LWIP_UNUSED_ARG(arg);

    PRINTF("Received %u bytes from the topic \"%s\": \"", tot_len, topic);

    memcpy(sSubscribedInfo.topic, topic, strlen(topic)+1);
}

/*!
 * @brief Called when recieved incoming published message fragment.
 */
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    int i;

    LWIP_UNUSED_ARG(arg);

    for (i = 0; i < len; i++)
    {
        if (isprint(data[i]))
        {
            PRINTF("%c", (char)data[i]);
        }
        else
        {
            PRINTF("\\x%02x", data[i]);
        }
    }

    memcpy(&sSubscribedInfo.payload, data, len);
    gNewPost = true;

    if (flags & MQTT_DATA_FLAG_LAST)
    {
        PRINTF("\"\r\n");
    }
}

/*!
 * @brief Subscribe to MQTT topics.
 */
static void mqtt_subscribe_topics(mqtt_client_t *client)
{
    static const char *topics[] = {"noe_station/state/", "noe_station/monitor/", "noe_station/sensors/anemometer/unit/"};
    int qos[]                   = {0, 0, 0};
    err_t err;
    int i;

    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb,
                            LWIP_CONST_CAST(void *, &mqtt_client_info));

    for (i = 0; i < ARRAY_SIZE(topics); i++)
    {
        err = mqtt_subscribe(client, topics[i], qos[i], mqtt_topic_subscribed_cb, LWIP_CONST_CAST(void *, topics[i]));

        if (err == ERR_OK)
        {
            PRINTF("Subscribing to the topic \"%s\" with QoS %d...\r\n", topics[i], qos[i]);
        }
        else
        {
            PRINTF("Failed to subscribe to the topic \"%s\" with QoS %d: %d.\r\n", topics[i], qos[i], err);
        }
    }
}

/*!
 * @brief Called when connection state changes.
 */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    const struct mqtt_connect_client_info_t *client_info = (const struct mqtt_connect_client_info_t *)arg;

    connected = (status == MQTT_CONNECT_ACCEPTED);

    switch (status)
    {
        case MQTT_CONNECT_ACCEPTED:
            PRINTF("MQTT client \"%s\" connected.\r\n", client_info->client_id);
            mqtt_subscribe_topics(client);
            break;

        case MQTT_CONNECT_DISCONNECTED:
            PRINTF("MQTT client \"%s\" not connected.\r\n", client_info->client_id);
            /* Try to reconnect 1 second later */
            sys_timeout(1000, connect_to_mqtt, NULL);
            break;

        case MQTT_CONNECT_TIMEOUT:
            PRINTF("MQTT client \"%s\" connection timeout.\r\n", client_info->client_id);
            /* Try again 1 second later */
            sys_timeout(1000, connect_to_mqtt, NULL);
            break;

        case MQTT_CONNECT_REFUSED_PROTOCOL_VERSION:
        case MQTT_CONNECT_REFUSED_IDENTIFIER:
        case MQTT_CONNECT_REFUSED_SERVER:
        case MQTT_CONNECT_REFUSED_USERNAME_PASS:
        case MQTT_CONNECT_REFUSED_NOT_AUTHORIZED_:
            PRINTF("MQTT client \"%s\" connection refused: %d.\r\n", client_info->client_id, (int)status);
            /* Try again 10 seconds later */
            sys_timeout(10000, connect_to_mqtt, NULL);
            break;

        default:
            PRINTF("MQTT client \"%s\" connection status: %d.\r\n", client_info->client_id, (int)status);
            /* Try again 10 seconds later */
            sys_timeout(10000, connect_to_mqtt, NULL);
            break;
    }
}

/*!
 * @brief Starts connecting to MQTT broker. To be called on tcpip_thread.
 */
static void connect_to_mqtt(void *ctx)
{
    LWIP_UNUSED_ARG(ctx);

    PRINTF("Connecting to MQTT broker at %s...\r\n", ipaddr_ntoa(&mqtt_addr));

    mqtt_client_connect(mqtt_client, &mqtt_addr, EXAMPLE_MQTT_SERVER_PORT, mqtt_connection_cb,
                        LWIP_CONST_CAST(void *, &mqtt_client_info), &mqtt_client_info);
}

/*!
 * @brief Called when publish request finishes.
 */
static void mqtt_message_published_cb(void *arg, err_t err)
{
    const char *topic = (const char *)arg;

    if (err == ERR_OK)
    {
        PRINTF("Published to the topic \"%s\".\r\n", topic);
    }
    else
    {
        PRINTF("Failed to publish to the topic \"%s\": %d.\r\n", topic, err);
    }
}

/*!
 * @brief Publishes a message. To be called on tcpip_thread.
 */
static void publish_feedback(void *ctx)
{
	static const char *topic = "noe_station/feedbackState";

	LWIP_UNUSED_ARG(ctx);

    PRINTF("Going to publish to the topic \"%s\"...\r\n", topic);
    mqtt_publish(mqtt_client, topic, (char *)ctx, strlen((char *)ctx), 1, 0, mqtt_message_published_cb, (void *)topic);
}

static void publish_pollution(void *ctx)
{
	static const char *topic = "noe_station/sensors/pollution";

	LWIP_UNUSED_ARG(ctx);

    PRINTF("Going to publish to the topic \"%s\"...\r\n", topic);
    mqtt_publish(mqtt_client, topic, (char *)ctx, strlen((char *)ctx), 1, 0, mqtt_message_published_cb, (void *)topic);
}

static void publish_pyranometer(void *ctx)
{
	static const char *topic = "noe_station/sensors/pyranometer";

	LWIP_UNUSED_ARG(ctx);

    PRINTF("Going to publish to the topic \"%s\"...\r\n", topic);
    mqtt_publish(mqtt_client, topic, (char *)ctx, strlen((char *)ctx), 1, 0, mqtt_message_published_cb, (void *)topic);
}

static void publish_pluviometer(void *ctx)
{
	static const char *topic = "noe_station/sensors/pluviometer";

	LWIP_UNUSED_ARG(ctx);

    PRINTF("Going to publish to the topic \"%s\"...\r\n", topic);
    mqtt_publish(mqtt_client, topic, (char *)ctx, strlen((char *)ctx), 1, 0, mqtt_message_published_cb, (void *)topic);
}

static void publish_anemometer(void *ctx)
{
	static const char *topic = "noe_station/sensors/anemometer";

	LWIP_UNUSED_ARG(ctx);

    PRINTF("Going to publish to the topic \"%s\"...\r\n", topic);
    mqtt_publish(mqtt_client, topic, (char *)ctx, strlen((char *)ctx), 1, 0, mqtt_message_published_cb, (void *)topic);
}

/*!
 * @brief Application thread.
 */
static void app_thread(void *arg)
{
    struct netif *netif = (struct netif *)arg;
    struct dhcp *dhcp;
    err_t err;
    int i;
    char publish[20];

    /* Wait for address from DHCP */
    initStation();

    PRINTF("Getting IP address from DHCP...\r\n");

    do
    {
        if (netif_is_up(netif))
        {
            dhcp = netif_dhcp_data(netif);
        }
        else
        {
            dhcp = NULL;
        }

        sys_msleep(20U);

    } while ((dhcp == NULL) || (dhcp->state != DHCP_STATE_BOUND));

    PRINTF("\r\nIPv4 Address     : %s\r\n", ipaddr_ntoa(&netif->ip_addr));
    PRINTF("IPv4 Subnet mask : %s\r\n", ipaddr_ntoa(&netif->netmask));
    PRINTF("IPv4 Gateway     : %s\r\n\r\n", ipaddr_ntoa(&netif->gw));

    /*
     * Check if we have an IP address or host name string configured.
     * Could just call netconn_gethostbyname() on both IP address or host name,
     * but we want to print some info if goint to resolve it.
     */
    if (ipaddr_aton(EXAMPLE_MQTT_SERVER_HOST, &mqtt_addr) && IP_IS_V4(&mqtt_addr))
    {
        /* Already an IP address */
        err = ERR_OK;
    }
    else
    {
        /* Resolve MQTT broker's host name to an IP address */
        PRINTF("Resolving \"%s\"...\r\n", EXAMPLE_MQTT_SERVER_HOST);
        err = netconn_gethostbyname(EXAMPLE_MQTT_SERVER_HOST, &mqtt_addr);
    }

    if (err == ERR_OK)
    {
        /* Start connecting to MQTT broker from tcpip_thread */
        err = tcpip_callback(connect_to_mqtt, NULL);
        if (err != ERR_OK)
        {
            PRINTF("Failed to invoke broker connection on the tcpip_thread: %d.\r\n", err);
        }
    }
    else
    {
        PRINTF("Failed to obtain IP address: %d.\r\n", err);
    }

    /* Publish some messages */
    for(i=0;i<5;)
    {
        if (connected)
        {
        	if(gNewPost){
        		extractSubscribedData();
        	}
        	//State feedback
        	getStateFeedback(publish);
			tcpip_callback(publish_feedback, publish);

			if(sTopics.state){
				//Pollution
				mock_getPollution(publish);
				tcpip_callback(publish_pollution, publish);
				//Pyranometer
				mock_getRadiation(publish);
				tcpip_callback(publish_pyranometer, publish);
				//Precipitation
				mock_getPrecipitation(publish);
				tcpip_callback(publish_pluviometer, publish);
				//Anemometer
				mock_getWindSpeed(publish);
				tcpip_callback(publish_anemometer, publish);
				delay(TIME);
				i++;
			}
        }
        sys_msleep(1000U);
    }

    free(sSubscribedInfo.topic);
    vTaskDelete(NULL);
}

static void generate_client_id(void)
{
    uint32_t mqtt_id[MQTT_ID_SIZE];
    int res;

    get_mqtt_id(&mqtt_id[0]);

    res = snprintf(client_id, sizeof(client_id), "nxp_%08lx%08lx%08lx%08lx", mqtt_id[3], mqtt_id[2], mqtt_id[1],
                   mqtt_id[0]);
    if ((res < 0) || (res >= sizeof(client_id)))
    {
        PRINTF("snprintf failed: %d\r\n", res);
        while (1)
        {
        }
    }
}

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
    generate_client_id();

    mdioHandle.resource.csrClock_Hz = EXAMPLE_CLOCK_FREQ;

    IP4_ADDR(&netif_ipaddr, 0U, 0U, 0U, 0U);
    IP4_ADDR(&netif_netmask, 0U, 0U, 0U, 0U);
    IP4_ADDR(&netif_gw, 0U, 0U, 0U, 0U);

    tcpip_init(NULL, NULL);

    LOCK_TCPIP_CORE();
    mqtt_client = mqtt_client_new();
    UNLOCK_TCPIP_CORE();
    if (mqtt_client == NULL)
    {
        PRINTF("mqtt_client_new() failed.\r\n");
        while (1)
        {
        }
    }

    netifapi_netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN,
                       tcpip_input);
    netifapi_netif_set_default(&netif);
    netifapi_netif_set_up(&netif);

    netifapi_dhcp_start(&netif);

    PRINTF("\r\n************************************************\r\n");
    PRINTF(" MQTT client example\r\n");
    PRINTF("************************************************\r\n");

    if (sys_thread_new("app_task", app_thread, &netif, APP_THREAD_STACKSIZE, APP_THREAD_PRIO) == NULL)
    {
        LWIP_ASSERT("stack_init(): Task creation failed.", 0);
    }

    vTaskDelete(NULL);
}

/*!
 * @brief Main function
 */
int main(void)
{
	/* Define the init structure for the output LED pin*/
	gpio_pin_config_t led_config = {
		kGPIO_DigitalOutput,
		0,
	};

    SYSMPU_Type *base = SYSMPU;
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    /* Disable SYSMPU. */
    base->CESR &= ~SYSMPU_CESR_VLD_MASK;

    GPIO_PinInit(BOARD_LED_GPIO, BOARD_LED_GPIO_PIN, &led_config);
	GPIO_PinWrite(BOARD_LED_GPIO, BOARD_LED_GPIO_PIN, 1U);

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
