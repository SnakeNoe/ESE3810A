/* CoAP server Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

/*
 * WARNING
 * libcoap is not multi-thread safe, so only this thread must make any coap_*()
 * calls.  Any external (to this thread) data transmitted in/out via libcoap
 * therefore has to be passed in/out by xQueue*() via this thread.
 */

#include <string.h>
#include <sys/socket.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_event.h"

#include "nvs_flash.h"

#include "protocol_examples_common.h"

#include "coap3/coap.h"

#ifndef CONFIG_COAP_SERVER_SUPPORT
#error COAP_SERVER_SUPPORT needs to be enabled
#endif /* COAP_SERVER_SUPPORT */

/* The examples use simple Pre-Shared-Key configuration that you can set via
   'idf.py menuconfig'.

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_COAP_PSK_KEY "some-agreed-preshared-key"

   Note: PSK will only be used if the URI is prefixed with coaps://
   instead of coap:// and the PSK must be one that the server supports
   (potentially associated with the IDENTITY)
*/
#define EXAMPLE_COAP_PSK_KEY CONFIG_EXAMPLE_COAP_PSK_KEY

/* The examples use CoAP Logging Level that
   you can set via 'idf.py menuconfig'.

   If you'd rather not, just change the below entry to a value
   that is between 0 and 7 with
   the config you want - ie #define EXAMPLE_COAP_LOG_DEFAULT_LEVEL 7

   Caution: Logging is enabled in libcoap only up to level as defined
   by 'idf.py menuconfig' to reduce code size.
*/
#define EXAMPLE_COAP_LOG_DEFAULT_LEVEL CONFIG_COAP_LOG_DEFAULT_LEVEL

const static char *TAG = "CoAP server";

uint8_t g_state = 0;
uint8_t g_monitor = 0;
uint8_t g_index = 0;
uint8_t g_unit = 0;
//Gets
static char feedback_data[100];
static int feedback_data_len = 0;
static char pollution_data[100];
static int pollution_data_len = 0;
static char radiation_data[100];
static int radiation_data_len = 0;
static char precipitation_data[100];
static int precipitation_data_len = 0;
static char wind_speed_data[100];
static int wind_speed_data_len = 0;
//Puts
static char state_data[100];
static int state_data_len = 0;
static char monitor_data[100];
static int monitor_data_len = 0;
static char anemometer_unit_data[100];
static int anemometer_unit_data_len = 0;

void get_next_index(void){
    g_index++;
    if(g_index>4){
        g_index = 0;
    }
}

void mock_get_pollution(char pollution[], uint8_t index){
    uint16_t data[5] = {1000, 1100, 1200, 1300, 1400};

    if(g_monitor){
        printf("Pollution: %d PPM\r\n", data[index]);
    }
    sprintf(pollution, "%d", data[index]);
    get_next_index();
}

void mock_get_radiation(char radiation[], uint8_t index){
    uint16_t data[5] = {2000, 2100, 2200, 2300, 2400};

    if(g_monitor){
        printf("Radiation: %d W/m2\r\n", data[index]);
    }
    sprintf(radiation, "%d", data[index]);
    get_next_index();
}

void mock_get_precipitation(char precipitation[], uint8_t index){
    uint16_t data[5] = {1, 2, 3, 4, 5};

    if(g_monitor){
        printf("Precipitation: %d mm/h\r\n", data[index]);
    }
    sprintf(precipitation, "%d", data[index]);
    get_next_index();
}

void mock_get_wind_speed(char wind_speed[], uint8_t index){
    uint16_t km[5] = {50, 60, 70, 80, 90};
    uint16_t m[5] = {25, 26, 27, 28, 29};

    if(g_unit){
        if(g_monitor){
            printf("Wind speed: %d MPH\r\n", m[index]);
        }
        sprintf(wind_speed, "%d", m[index]);
    }
    else{
        if(g_monitor){
            printf("Wind speed: %d Km/h\r\n", km[index]);
        }
        sprintf(wind_speed, "%d", km[index]);
    }
    get_next_index();
}

static void
hnd_feedback_get(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    char feedback_str[6];

    sprintf(feedback_str, "%d", g_state);
    snprintf(feedback_data, sizeof(feedback_data), feedback_str);
    feedback_data_len = strlen(feedback_data);

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
    coap_add_data_large_response(resource, session, request, response,
                                 query, COAP_MEDIATYPE_TEXT_PLAIN, 60, 0,
                                 (size_t)feedback_data_len,
                                 (const u_char *)feedback_data,
                                 NULL, NULL);
    if(g_state){
        printf("Published response to client: state on\r\n");
    }
    else{
        printf("Published response to client: state off\r\n");
    }
}

static void
hnd_pollution_get(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    char pollution_str[6];
    
    mock_get_pollution(pollution_str, g_index);
    snprintf(pollution_data, sizeof(pollution_data), pollution_str);
    pollution_data_len = strlen(pollution_data);

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
    coap_add_data_large_response(resource, session, request, response,
                                query, COAP_MEDIATYPE_TEXT_PLAIN, 60, 0,
                                (size_t)pollution_data_len,
                                (const u_char *)pollution_data,
                                NULL, NULL);
    printf("Published response to client: %s PPM\r\n", pollution_data);
}

static void
hnd_radiation_get(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    char radiation_str[6];

    mock_get_radiation(radiation_str, g_index);
    snprintf(radiation_data, sizeof(radiation_data), radiation_str);
    radiation_data_len = strlen(radiation_data);

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
    coap_add_data_large_response(resource, session, request, response,
                                 query, COAP_MEDIATYPE_TEXT_PLAIN, 60, 0,
                                 (size_t)radiation_data_len,
                                 (const u_char *)radiation_data,
                                 NULL, NULL);
    printf("Published response to client: %s W/m2\r\n", radiation_data);
}

static void
hnd_precipitation_get(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    char precipitation_str[6];

    mock_get_precipitation(precipitation_str, g_index);
    snprintf(precipitation_data, sizeof(precipitation_data), precipitation_str);
    precipitation_data_len = strlen(precipitation_data);

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
    coap_add_data_large_response(resource, session, request, response,
                                 query, COAP_MEDIATYPE_TEXT_PLAIN, 60, 0,
                                 (size_t)precipitation_data_len,
                                 (const u_char *)precipitation_data,
                                 NULL, NULL);
    printf("Published response to client: %s mm/h\r\n", precipitation_data);
}

static void
hnd_wind_speed_get(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    char wind_speed_str[4];

    mock_get_wind_speed(wind_speed_str, g_index);
    snprintf(wind_speed_data, sizeof(wind_speed_data), wind_speed_str);
    wind_speed_data_len = strlen(wind_speed_data);

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CONTENT);
    coap_add_data_large_response(resource, session, request, response,
                                 query, COAP_MEDIATYPE_TEXT_PLAIN, 60, 0,
                                 (size_t)wind_speed_data_len,
                                 (const u_char *)wind_speed_data,
                                 NULL, NULL);
    if(g_unit){
        printf("Published response to client: %s MPH\r\n", wind_speed_data);
    }
    else{
        printf("Published response to client: %s Km/h\r\n", wind_speed_data);
    }
}

static void
hnd_state_put(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    size_t size;
    size_t offset;
    size_t total;
    const unsigned char *data;

    coap_resource_notify_observers(resource, NULL);

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);

    /* coap_get_data_large() sets size to 0 on error */
    (void)coap_get_data_large(request, &size, &data, &offset, &total);

    if (size == 0) {      /* re-init */
        snprintf(state_data, sizeof(state_data), "0");
        state_data_len = strlen(state_data);
    } else {
        state_data_len = size > sizeof (state_data) ? sizeof (state_data) : size;
        memcpy(state_data, data, state_data_len);
    }
    g_state = atoi(state_data);
    if(g_state){
        printf("State on\r\n");
    }
    else{
        printf("State off\r\n");
    }
}

static void
hnd_monitor_put(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    size_t size;
    size_t offset;
    size_t total;
    const unsigned char *data;

    coap_resource_notify_observers(resource, NULL);

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);

    /* coap_get_data_large() sets size to 0 on error */
    (void)coap_get_data_large(request, &size, &data, &offset, &total);

    if (size == 0) {      /* re-init */
        snprintf(monitor_data, sizeof(monitor_data), "0");
        monitor_data_len = strlen(monitor_data);
    } else {
        monitor_data_len = size > sizeof (monitor_data) ? sizeof (monitor_data) : size;
        memcpy(monitor_data, data, monitor_data_len);
    }
    g_monitor = atoi(monitor_data);
    if(g_monitor){
        printf("Monitor on\r\n");
    }
    else{
        printf("Monitor off\r\n");
    }
}

static void
hnd_anemometer_unit_put(coap_resource_t *resource,
                  coap_session_t *session,
                  const coap_pdu_t *request,
                  const coap_string_t *query,
                  coap_pdu_t *response)
{
    size_t size;
    size_t offset;
    size_t total;
    const unsigned char *data;

    coap_resource_notify_observers(resource, NULL);

    coap_pdu_set_code(response, COAP_RESPONSE_CODE_CHANGED);

    /* coap_get_data_large() sets size to 0 on error */
    (void)coap_get_data_large(request, &size, &data, &offset, &total);

    if (size == 0) {      /* re-init */
        snprintf(anemometer_unit_data, sizeof(anemometer_unit_data), "0");
        anemometer_unit_data_len = strlen(anemometer_unit_data);
    } else {
        anemometer_unit_data_len = size > sizeof (anemometer_unit_data) ? sizeof (anemometer_unit_data) : size;
        memcpy(anemometer_unit_data, data, anemometer_unit_data_len);
    }
    g_unit = atoi(anemometer_unit_data);
    if(g_unit){
        printf("Anemometer unit to MPH\r\n");
    }
    else{
        printf("Anemometer unit to Km/h\r\n");
    }
}

static void
coap_log_handler (coap_log_t level, const char *message)
{
    uint32_t esp_level = ESP_LOG_INFO;
    const char *cp = strchr(message, '\n');

    while (cp) {
        ESP_LOG_LEVEL(esp_level, TAG, "%.*s", (int)(cp - message), message);
        message = cp + 1;
        cp = strchr(message, '\n');
    }
    if (message[0] != '\000') {
        ESP_LOG_LEVEL(esp_level, TAG, "%s", message);
    }
}

static void coap_example_server(void *p)
{
    coap_context_t *ctx = NULL;
    coap_resource_t *resource_feedback = NULL;
    coap_resource_t *resource_pollution = NULL;
    coap_resource_t *resource_radiation = NULL;
    coap_resource_t *resource_precipitation = NULL;
    coap_resource_t *resource_wind_speed = NULL;
    coap_resource_t *resource_state = NULL;
    coap_resource_t *resource_monitor = NULL;
    coap_resource_t *resource_anemometer_unit = NULL;
    int have_ep = 0;
    uint16_t u_s_port = atoi(CONFIG_EXAMPLE_COAP_LISTEN_PORT);
    uint16_t s_port = 0;
    uint16_t ws_port = 0;
    uint16_t ws_s_port = 0;
    uint32_t scheme_hint_bits;

    /* Initialize libcoap library */
    coap_startup();

    coap_set_log_handler(coap_log_handler);
    coap_set_log_level(EXAMPLE_COAP_LOG_DEFAULT_LEVEL);

    while (1) {
        unsigned wait_ms;
        coap_addr_info_t *info = NULL;
        coap_addr_info_t *info_list = NULL;

        ctx = coap_new_context(NULL);
        if (!ctx) {
            ESP_LOGE(TAG, "coap_new_context() failed");
            goto clean_up;
        }
        coap_context_set_block_mode(ctx,
                                    COAP_BLOCK_USE_LIBCOAP | COAP_BLOCK_SINGLE_BODY);
        coap_context_set_max_idle_sessions(ctx, 20);

        /* set up the CoAP server socket(s) */
        scheme_hint_bits =
            coap_get_available_scheme_hint_bits(
                0,
                0,
                0);

        info_list = coap_resolve_address_info(coap_make_str_const("0.0.0.0"), u_s_port, s_port,
                                            ws_port, ws_s_port,
                                            0,
                                            scheme_hint_bits,
                                            COAP_RESOLVE_TYPE_LOCAL);
        if (info_list == NULL) {
            ESP_LOGE(TAG, "coap_resolve_address_info() failed");
            goto clean_up;
        }

        for (info = info_list; info != NULL; info = info->next) {
            coap_endpoint_t *ep;

            ep = coap_new_endpoint(ctx, &info->addr, info->proto);
            if (!ep) {
                ESP_LOGW(TAG, "cannot create endpoint for proto %u", info->proto);
            } else {
                have_ep = 1;
            }
        }
        coap_free_address_info(info_list);
        if (!have_ep) {
            ESP_LOGE(TAG, "No endpoints available");
            goto clean_up;
        }

        resource_feedback = coap_resource_init(coap_make_str_const("feedback"), 0);
        if (!resource_feedback) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }
        resource_pollution = coap_resource_init(coap_make_str_const("pollution"), 0);
        if (!resource_pollution) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }
        resource_radiation = coap_resource_init(coap_make_str_const("radiation"), 0);
        if (!resource_radiation) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }
        resource_precipitation = coap_resource_init(coap_make_str_const("precipitation"), 0);
        if (!resource_precipitation) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }
        resource_wind_speed = coap_resource_init(coap_make_str_const("wind_speed"), 0);
        if (!resource_wind_speed) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }
        resource_state = coap_resource_init(coap_make_str_const("state"), 0);
        if (!resource_state) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }
        resource_monitor = coap_resource_init(coap_make_str_const("monitor"), 0);
        if (!resource_monitor) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }
        resource_anemometer_unit = coap_resource_init(coap_make_str_const("anemometer_unit"), 0);
        if (!resource_anemometer_unit) {
            ESP_LOGE(TAG, "coap_resource_init() failed");
            goto clean_up;
        }
        coap_register_handler(resource_feedback, COAP_REQUEST_GET, hnd_feedback_get);
        coap_register_handler(resource_pollution, COAP_REQUEST_GET, hnd_pollution_get);
        coap_register_handler(resource_radiation, COAP_REQUEST_GET, hnd_radiation_get);
        coap_register_handler(resource_precipitation, COAP_REQUEST_GET, hnd_precipitation_get);
        coap_register_handler(resource_wind_speed, COAP_REQUEST_GET, hnd_wind_speed_get);
        coap_register_handler(resource_state, COAP_REQUEST_PUT, hnd_state_put);
        coap_register_handler(resource_monitor, COAP_REQUEST_PUT, hnd_monitor_put);
        coap_register_handler(resource_anemometer_unit, COAP_REQUEST_PUT, hnd_anemometer_unit_put);
        /* We possibly want to Observe the GETs */
        coap_resource_set_get_observable(resource_feedback, 1);
        coap_add_resource(ctx, resource_feedback);
        coap_resource_set_get_observable(resource_pollution, 1);
        coap_add_resource(ctx, resource_pollution);
        coap_resource_set_get_observable(resource_radiation, 1);
        coap_add_resource(ctx, resource_radiation);
        coap_resource_set_get_observable(resource_precipitation, 1);
        coap_add_resource(ctx, resource_precipitation);
        coap_resource_set_get_observable(resource_wind_speed, 1);
        coap_add_resource(ctx, resource_wind_speed);
        coap_resource_set_get_observable(resource_state, 1);
        coap_add_resource(ctx, resource_state);
        coap_resource_set_get_observable(resource_monitor, 1);
        coap_add_resource(ctx, resource_monitor);
        coap_resource_set_get_observable(resource_anemometer_unit, 1);
        coap_add_resource(ctx, resource_anemometer_unit);

        wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

        while (1) {
            int result = coap_io_process(ctx, wait_ms);
            if (result < 0) {
                break;
            } else if (result && (unsigned)result < wait_ms) {
                /* decrement if there is a result wait time returned */
                wait_ms -= result;
            }
            if (result) {
                /* result must have been >= wait_ms, so reset wait_ms */
                wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
            }
        }
    }
clean_up:
    coap_free_context(ctx);
    coap_cleanup();

    vTaskDelete(NULL);
}

void app_main(void)
{
    ESP_ERROR_CHECK( nvs_flash_init() );
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());

    xTaskCreate(coap_example_server, "coap", 8 * 1024, NULL, 5, NULL);
}
