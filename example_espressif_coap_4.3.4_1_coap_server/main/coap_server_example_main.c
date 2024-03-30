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

#include "esp_netif_ip_addr.h"
#include "esp_mac.h"
#include "esp_netif.h"
#include "mdns.h"
#include "netdb.h"
#include "driver/gpio.h"

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

#define CONFIG_MDNS_HOSTNAME "esp32-mdns-Noe"
#define CONFIG_MDNS_INSTANCE "ESP32 with mDNS"
#define EXAMPLE_MDNS_INSTANCE CONFIG_MDNS_INSTANCE
#define CONFIG_MDNS_BUTTON_GPIO 0
#define EXAMPLE_BUTTON_GPIO   CONFIG_MDNS_BUTTON_GPIO

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

static char *generate_hostname(void)
{
#ifndef CONFIG_MDNS_ADD_MAC_TO_HOSTNAME
    return strdup(CONFIG_MDNS_HOSTNAME);
#else
    uint8_t mac[6];
    char   *hostname;
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
    if (-1 == asprintf(&hostname, "%s-%02X%02X%02X", CONFIG_MDNS_HOSTNAME, mac[3], mac[4], mac[5])) {
        abort();
    }
    return hostname;
#endif
}

static void initialise_mdns(void)
{
    char *hostname = generate_hostname();

    //initialize mDNS
    ESP_ERROR_CHECK( mdns_init() );
    //set mDNS hostname (required if you want to advertise services)
    ESP_ERROR_CHECK( mdns_hostname_set(hostname) );
    ESP_LOGI(TAG, "mdns hostname set to: [%s]", hostname);
    //set default mDNS instance name
    ESP_ERROR_CHECK( mdns_instance_name_set(EXAMPLE_MDNS_INSTANCE) );

    //structure with TXT records
    mdns_txt_item_t serviceTxtData[3] = {
        {"board", "esp32"},
        {"u", "user"},
        {"p", "password"}
    };

    //initialize service
    ESP_ERROR_CHECK( mdns_service_add("ESP32-WebServer", "_http", "_tcp", 80, serviceTxtData, 3) );
    ESP_ERROR_CHECK( mdns_service_subtype_add_for_host("ESP32-WebServer", "_http", "_tcp", NULL, "_server") );
#if CONFIG_MDNS_MULTIPLE_INSTANCE
    ESP_ERROR_CHECK( mdns_service_add("ESP32-WebServer1", "_http", "_tcp", 80, NULL, 0) );
#endif

#if CONFIG_MDNS_PUBLISH_DELEGATE_HOST
    char *delegated_hostname;
    if (-1 == asprintf(&delegated_hostname, "%s-delegated", hostname)) {
        abort();
    }

    mdns_ip_addr_t addr4, addr6;
    esp_netif_str_to_ip4("10.0.0.1", &addr4.addr.u_addr.ip4);
    addr4.addr.type = ESP_IPADDR_TYPE_V4;
    esp_netif_str_to_ip6("fd11:22::1", &addr6.addr.u_addr.ip6);
    addr6.addr.type = ESP_IPADDR_TYPE_V6;
    addr4.next = &addr6;
    addr6.next = NULL;
    ESP_ERROR_CHECK( mdns_delegate_hostname_add(delegated_hostname, &addr4) );
    ESP_ERROR_CHECK( mdns_service_add_for_host("test0", "_http", "_tcp", delegated_hostname, 1234, serviceTxtData, 3) );
    free(delegated_hostname);
#endif // CONFIG_MDNS_PUBLISH_DELEGATE_HOST

    //add another TXT item
    ESP_ERROR_CHECK( mdns_service_txt_item_set("_http", "_tcp", "path", "/foobar") );
    //change TXT item value
    ESP_ERROR_CHECK( mdns_service_txt_item_set_with_explicit_value_len("_http", "_tcp", "u", "admin", strlen("admin")) );
    free(hostname);
}

/* these strings match mdns_ip_protocol_t enumeration */
static const char *ip_protocol_str[] = {"V4", "V6", "MAX"};

static void mdns_print_results(mdns_result_t *results)
{
    mdns_result_t *r = results;
    mdns_ip_addr_t *a = NULL;
    int i = 1, t;
    while (r) {
        if (r->esp_netif) {
            printf("%d: Interface: %s, Type: %s, TTL: %" PRIu32 "\n", i++, esp_netif_get_ifkey(r->esp_netif),
                   ip_protocol_str[r->ip_protocol], r->ttl);
        }
        if (r->instance_name) {
            printf("  PTR : %s.%s.%s\n", r->instance_name, r->service_type, r->proto);
        }
        if (r->hostname) {
            printf("  SRV : %s.local:%u\n", r->hostname, r->port);
        }
        if (r->txt_count) {
            printf("  TXT : [%zu] ", r->txt_count);
            for (t = 0; t < r->txt_count; t++) {
                printf("%s=%s(%d); ", r->txt[t].key, r->txt[t].value ? r->txt[t].value : "NULL", r->txt_value_len[t]);
            }
            printf("\n");
        }
        a = r->addr;
        while (a) {
            if (a->addr.type == ESP_IPADDR_TYPE_V6) {
                printf("  AAAA: " IPV6STR "\n", IPV62STR(a->addr.u_addr.ip6));
            } else {
                printf("  A   : " IPSTR "\n", IP2STR(&(a->addr.u_addr.ip4)));
            }
            a = a->next;
        }
        r = r->next;
    }
}

static void query_mdns_service(const char *service_name, const char *proto)
{
    ESP_LOGI(TAG, "Query PTR: %s.%s.local", service_name, proto);

    mdns_result_t *results = NULL;
    esp_err_t err = mdns_query_ptr(service_name, proto, 3000, 20,  &results);
    if (err) {
        ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
        return;
    }
    if (!results) {
        ESP_LOGW(TAG, "No results found!");
        return;
    }

    mdns_print_results(results);
    mdns_query_results_free(results);
}

#if CONFIG_MDNS_PUBLISH_DELEGATE_HOST
static void lookup_mdns_delegated_service(const char *service_name, const char *proto)
{
    ESP_LOGI(TAG, "Lookup delegated service: %s.%s.local", service_name, proto);

    mdns_result_t *results = NULL;
    esp_err_t err = mdns_lookup_delegated_service(NULL, service_name, proto, 20, &results);
    if (err) {
        ESP_LOGE(TAG, "Lookup Failed: %s", esp_err_to_name(err));
        return;
    }
    if (!results) {
        ESP_LOGW(TAG, "No results found!");
        return;
    }

    mdns_print_results(results);
    mdns_query_results_free(results);
}
#endif // CONFIG_MDNS_PUBLISH_DELEGATE_HOST

static void lookup_mdns_selfhosted_service(const char *service_name, const char *proto)
{
    ESP_LOGI(TAG, "Lookup selfhosted service: %s.%s.local", service_name, proto);
    mdns_result_t *results = NULL;
    esp_err_t err = mdns_lookup_selfhosted_service(NULL, service_name, proto, 20, &results);
    if (err) {
        ESP_LOGE(TAG, "Lookup Failed: %s", esp_err_to_name(err));
        return;
    }
    if (!results) {
        ESP_LOGW(TAG, "No results found!");
        return;
    }
    mdns_print_results(results);
    mdns_query_results_free(results);
}

static bool check_and_print_result(mdns_search_once_t *search)
{
    // Check if any result is available
    mdns_result_t *result = NULL;
    if (!mdns_query_async_get_results(search, 0, &result, NULL)) {
        return false;
    }

    if (!result) {   // search timeout, but no result
        return true;
    }

    // If yes, print the result
    mdns_ip_addr_t *a = result->addr;
    while (a) {
        if (a->addr.type == ESP_IPADDR_TYPE_V6) {
            printf("  AAAA: " IPV6STR "\n", IPV62STR(a->addr.u_addr.ip6));
        } else {
            printf("  A   : " IPSTR "\n", IP2STR(&(a->addr.u_addr.ip4)));
        }
        a = a->next;
    }
    // and free the result
    mdns_query_results_free(result);
    return true;
}

static void query_mdns_hosts_async(const char *host_name)
{
    ESP_LOGI(TAG, "Query both A and AAA: %s.local", host_name);

    mdns_search_once_t *s_a = mdns_query_async_new(host_name, NULL, NULL, MDNS_TYPE_A, 1000, 1, NULL);
    mdns_search_once_t *s_aaaa = mdns_query_async_new(host_name, NULL, NULL, MDNS_TYPE_AAAA, 1000, 1, NULL);
    while (s_a || s_aaaa) {
        if (s_a && check_and_print_result(s_a)) {
            ESP_LOGI(TAG, "Query A %s.local finished", host_name);
            mdns_query_async_delete(s_a);
            s_a = NULL;
        }
        if (s_aaaa && check_and_print_result(s_aaaa)) {
            ESP_LOGI(TAG, "Query AAAA %s.local finished", host_name);
            mdns_query_async_delete(s_aaaa);
            s_aaaa = NULL;
        }
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}
#ifdef CONFIG_LWIP_IPV4
static void query_mdns_host(const char *host_name)
{
    ESP_LOGI(TAG, "Query A: %s.local", host_name);

    struct esp_ip4_addr addr;
    addr.addr = 0;

    esp_err_t err = mdns_query_a(host_name, 2000,  &addr);
    if (err) {
        if (err == ESP_ERR_NOT_FOUND) {
            ESP_LOGW(TAG, "%s: Host was not found!", esp_err_to_name(err));
            return;
        }
        ESP_LOGE(TAG, "Query Failed: %s", esp_err_to_name(err));
        return;
    }

    ESP_LOGI(TAG, "Query A: %s.local resolved to: " IPSTR, host_name, IP2STR(&addr));
}
#endif // CONFIG_LWIP_IPV4

static void initialise_button(void)
{
    gpio_config_t io_conf = {0};
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.pin_bit_mask = BIT64(EXAMPLE_BUTTON_GPIO);
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_up_en = 1;
    io_conf.pull_down_en = 0;
    gpio_config(&io_conf);
}

static void check_button(void)
{
    static bool old_level = true;
    bool new_level = gpio_get_level(EXAMPLE_BUTTON_GPIO);
    if (!new_level && old_level) {
        query_mdns_hosts_async("esp32-mdns");
#ifdef CONFIG_LWIP_IPV4
        query_mdns_host("esp32");
#endif
        //query_mdns_service("_arduino", "_tcp");
        query_mdns_service("_http", "_tcp");
        //query_mdns_service("_printer", "_tcp");
        //query_mdns_service("_ipp", "_tcp");
        //query_mdns_service("_afpovertcp", "_tcp");
        //query_mdns_service("_smb", "_tcp");
        //query_mdns_service("_ftp", "_tcp");
        //query_mdns_service("_nfs", "_tcp");
#if CONFIG_MDNS_PUBLISH_DELEGATE_HOST
        lookup_mdns_delegated_service("_http", "_tcp");
#endif // CONFIG_MDNS_PUBLISH_DELEGATE_HOST
        //lookup_mdns_selfhosted_service("_http", "_tcp");
    }
    old_level = new_level;
}

static void mdns_example_task(void *pvParameters)
{
#if CONFIG_MDNS_RESOLVE_TEST_SERVICES == 1
    /* Send initial queries that are started by CI tester */
#ifdef CONFIG_LWIP_IPV4
    query_mdns_host("tinytester");
#endif
    query_mdns_host_with_gethostbyname("tinytester-lwip.local");
    query_mdns_host_with_getaddrinfo("tinytester-lwip.local");
#endif

    while (1) {
        check_button();
        vTaskDelay(50 / portTICK_PERIOD_MS);
    }
}

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

    initialise_mdns();

    /* This helper function configures Wi-Fi or Ethernet, as selected in menuconfig.
     * Read "Establishing Wi-Fi or Ethernet Connection" section in
     * examples/protocols/README.md for more information about this function.
     */
    ESP_ERROR_CHECK(example_connect());
    initialise_button();

    xTaskCreate(&mdns_example_task, "mdns_example_task", 2048*4, NULL, 5, NULL);
    xTaskCreate(coap_example_server, "coap", 8*1024, NULL, 5, NULL);
}
