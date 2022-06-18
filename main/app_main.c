#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"
#include "sdkconfig.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include "driver/uart.h"

#include "C:/Users/admin/eclipse-workspace/tcp/New_LIB/dht11.h"

#include <stdio.h>
#include <stdint.h>
#include <stddef.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"
#include "protocol_examples_common.h"
#include "cJSON.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"

#include "esp_log.h"
#include "mqtt_client.h"
#define LED_BUILD_IN 			2
#define BUTTON_ON				4
#define BUTTON_OFF				5
#define GPIO_OUTPUT_PIN_SEL  	((1ULL << LED_BUILD_IN))
#define GPIO_INPUT_PIN_SEL  	(1ULL << BTN_GPIO)
#define ECHO_TEST_TXD 1
#define ECHO_TEST_RXD 3
#define ECHO_TEST_RTS (UART_PIN_NO_CHANGE)
#define ECHO_TEST_CTS (UART_PIN_NO_CHANGE)
float tem,hum;
#define ECHO_UART_PORT_NUM     2
#define ECHO_UART_BAUD_RATE     115200//(CONFIG_EXAMPLE_UART_BAUD_RATE)
#define ECHO_TASK_STACK_SIZE    2048//(CONFIG_EXAMPLE_TASK_STACK_SIZE)
#define LED_GPIO 2
#define BTN_GPIO 5
#define BTN_DHT 15
typedef enum {an0,an1,an2,ang} bt_state;
bt_state a=an0;
char b[9]="000000";
uint64_t t;
static struct dht11_reading dht,last_dht;
#define BUF_SIZE (1024)
void button_press_short_callback()
{
	t++;
	    			if(t>0 && t%2==1){ a=an1;}
	    			else if(t>0&&t%2==0) {a=an2;}
}
void button_pressing_timeout_callback()
{
	a=ang;

}

typedef enum
{
	BUTTON_READ,
	BUTTON_WAIT_DEBOUND,
	BUTTON_WAIT_RELEASE_AND_CHECK_LONG_PRESS,
	BUTTON_WAIT_RELEASE
}Button_State;

Button_State button_state = BUTTON_READ;

static const char *TAG = "MQTT_EXAMPLE";
static const char *TAG1 = "JSON";
uint8_t current_status =1,last_status =1;
uint32_t t_long_press = 0;
uint8_t is_long_press =0;
uint32_t time_debounce;
char h[12];

char *JSON_Types(int type) {
	if (type == cJSON_Invalid) return ("cJSON_Invalid");
	if (type == cJSON_False) return ("cJSON_False");
	if (type == cJSON_True) return ("cJSON_True");
	if (type == cJSON_NULL) return ("cJSON_NULL");
	if (type == cJSON_Number) return ("cJSON_Number");
	if (type == cJSON_String) return ("cJSON_String");
	if (type == cJSON_Array) return ("cJSON_Array");
	if (type == cJSON_Object) return ("cJSON_Object");
	if (type == cJSON_Raw) return ("cJSON_Raw");
	return NULL;
}


void JSON_Analyze(const cJSON * const root) {
	//ESP_LOGI(TAG, "root->type=%s", JSON_Types(root->type));
	cJSON *current_element = NULL;
	//ESP_LOGI(TAG, "roo->child=%p", root->child);
	//ESP_LOGI(TAG, "roo->next =%p", root->next);
	cJSON_ArrayForEach(current_element, root) {
		//ESP_LOGI(TAG, "type=%s", JSON_Types(current_element->type));
		//ESP_LOGI(TAG, "current_element->string=%p", current_element->string);
		if (current_element->string) {
			const char* string = current_element->string;
			ESP_LOGI(TAG, "[%s]", string);
		}
		if (cJSON_IsInvalid(current_element)) {
			ESP_LOGI(TAG, "Invalid");
		} else if (cJSON_IsFalse(current_element)) {
			ESP_LOGI(TAG, "False");
		} else if (cJSON_IsTrue(current_element)) {
			ESP_LOGI(TAG, "True");
		} else if (cJSON_IsNull(current_element)) {
			ESP_LOGI(TAG, "Null");
		} else if (cJSON_IsNumber(current_element)) {
			int valueint = current_element->valueint;
			double valuedouble = current_element->valuedouble;
			ESP_LOGI(TAG, "int=%d double=%f", valueint, valuedouble);
		} else if (cJSON_IsString(current_element)) {
			const char* valuestring = current_element->valuestring;
			ESP_LOGI(TAG, "%s", valuestring);
		} else if (cJSON_IsArray(current_element)) {
			//ESP_LOGI(TAG, "Array");
			JSON_Analyze(current_element);
		} else if (cJSON_IsObject(current_element)) {
			//ESP_LOGI(TAG, "Object");
			JSON_Analyze(current_element);
		} else if (cJSON_IsRaw(current_element)) {
			ESP_LOGI(TAG, "Raw(Not support)");
		}
	}
}

void handle_button()
{
		//doc trang thai nut nhan
	current_status = gpio_get_level(BTN_GPIO);

	switch(button_state)
	{
		case BUTTON_READ:
		{
			if((current_status == 0 && last_status == 1) )
			{
						time_debounce = esp_timer_get_time()/1000;
						button_state = BUTTON_WAIT_DEBOUND;
			}
		}
		break;
		case BUTTON_WAIT_DEBOUND:
		{
			if(esp_timer_get_time()/1000 - time_debounce>= 20)
			{
				if(current_status ==0 && last_status ==1)//nhan xuong
				{
					//button_pressing_callback();
					t_long_press = esp_timer_get_time()/1000;
					//last_status = current_status;
					last_status = 0;
					button_state = BUTTON_WAIT_RELEASE_AND_CHECK_LONG_PRESS;
				}
				else if(current_status ==1 && last_status ==0)//nha ra
				{
					t_long_press = esp_timer_get_time()/1000 - t_long_press;
					if(t_long_press <= 1000)
					{
						button_press_short_callback();
					}

					last_status = current_status;
					button_state = BUTTON_READ;
				}
				else //khong dung
				{
					last_status = 1;
					button_state = BUTTON_READ;
				}
			}
		}
		break;
		case BUTTON_WAIT_RELEASE_AND_CHECK_LONG_PRESS:
		{
				if(current_status == 1 && last_status == 0)
				{
					//truoc 3s dã nha phim
					button_state = BUTTON_WAIT_DEBOUND;
					time_debounce = esp_timer_get_time()/1000;
				}
				else if(esp_timer_get_time()/1000 - t_long_press >= 2000)
				{
					button_pressing_timeout_callback();
					button_state = BUTTON_WAIT_RELEASE;
				}
		}
		break;
		case BUTTON_WAIT_RELEASE:
		{
			if(current_status == 1 && last_status == 0)
			{
				button_state = BUTTON_WAIT_DEBOUND;
				time_debounce = esp_timer_get_time()/1000;
			}
		}
		break;
		default:

			break;
	}
}
void nhayled5(){
	for(int i=0;i<5;i++){
		gpio_set_level(LED_BUILD_IN, 1);
		        vTaskDelay(1000 / portTICK_PERIOD_MS);

		        gpio_set_level(LED_BUILD_IN, 0);
		        vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}
void nhayled10(){
	for(int i=0;i<10;i++){
		gpio_set_level(LED_BUILD_IN, 1);
		        vTaskDelay(1000 / portTICK_PERIOD_MS);

		        gpio_set_level(LED_BUILD_IN, 0);
		        vTaskDelay(1000 / portTICK_PERIOD_MS);
	}

}
void blink(){
	gpio_set_level(LED_BUILD_IN, 1);
			        vTaskDelay(5000 / portTICK_PERIOD_MS);

			        gpio_set_level(LED_BUILD_IN, 0);
}
static void echo_task(void *arg)
{
	DHT11_init(4);
    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = ECHO_UART_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    int intr_alloc_flags = 0;

#if CONFIG_UART_ISR_IN_IRAM
    intr_alloc_flags = ESP_INTR_FLAG_IRAM;
#endif

    ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, intr_alloc_flags));
    ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, ECHO_TEST_TXD, ECHO_TEST_RXD, ECHO_TEST_RTS, ECHO_TEST_CTS));

    // Configure a temporary buffer for the incoming data
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);

char s[12];
    while (1) {
    	 dht=DHT11_read();
    	    if(dht.status==0){
    	    last_dht=dht;
    	    tem=last_dht.temperature;
    	    hum=last_dht.humidity;}

    	if(a==an1&&tem>=30){sprintf(s,"t = %.1f ", tem);uart_write_bytes(ECHO_UART_PORT_NUM, s, 17),a=an0;nhayled5();a=an0;};
    	if(a==an2&&hum>30){sprintf(s,"h = %.1f ", hum);uart_write_bytes(ECHO_UART_PORT_NUM, s, 17),a=an0;nhayled10();};
    	if(a==ang&&hum>30&&tem>30){sprintf(s,"tem hum cao");uart_write_bytes(ECHO_UART_PORT_NUM, s, 17),a=an0;};

        // Write data back to the UART
       //uart_write_bytes(ECHO_UART_PORT_NUM, b, 10);
    }
}

void Init_gpio_output()
{
	  gpio_config_t io_conf = {};
	  io_conf.intr_type = GPIO_INTR_DISABLE;
	  io_conf.mode = GPIO_MODE_OUTPUT;
	  io_conf.pin_bit_mask = GPIO_OUTPUT_PIN_SEL;
	  io_conf.pull_down_en = 0;
	  io_conf.pull_up_en = 0;
	  gpio_config(&io_conf);

}
void Init_gpio_input()
{
	gpio_config_t io_conf = {};
	io_conf.intr_type = GPIO_INTR_DISABLE;
	io_conf.pin_bit_mask = GPIO_INPUT_PIN_SEL;
	io_conf.mode = GPIO_MODE_INPUT;
	io_conf.pull_up_en = 1;
	gpio_config(&io_conf);
	gpio_install_isr_service(0);

}
void button_task(void *pvParameter)
{

    gpio_pad_select_gpio(LED_GPIO);
    gpio_pad_select_gpio(BTN_GPIO);
    gpio_pad_select_gpio(BTN_DHT);

    /* Set the GPIO as a push/pull output */
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);

    gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
    gpio_set_pull_mode(BTN_GPIO, GPIO_PULLUP_ONLY);
    gpio_set_direction(BTN_DHT, GPIO_MODE_INPUT);

    static uint64_t time_db = 0;
char s[10];
    while(1) {

    	handle_button();
    	if(a==an1) nhayled5();
    }

}
char a1[5]="aaab";
static void log_error_if_nonzero(const char *message, int error_code)
{
    if (error_code != 0) {
        ESP_LOGE(TAG, "Last error %s: 0x%x", message, error_code);
    }
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    cJSON *root;
        	root = cJSON_CreateObject();
        	esp_chip_info_t chip_info;
        	esp_chip_info(&chip_info);
        	cJSON_AddNumberToObject(root, "t", tem);
        	cJSON_AddNumberToObject(root, "h", hum);
        	char *my_json_string = cJSON_Print(root);
        		ESP_LOGI(TAG, "my_json_string\n%s",my_json_string);
        		cJSON_Delete(root);

        		ESP_LOGI(TAG, "Deserialize.....");
        		cJSON *root2 = cJSON_Parse(my_json_string);
        		ESP_LOGI(TAG1, "my_json_string\n%s",my_json_string);
        		JSON_Analyze(root2);
        		cJSON_Delete(root2);
        		cJSON_free(my_json_string);
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        msg_id = esp_mqtt_client_publish(client, "messages/b13a15c0-5e5b-47b0-8aac-0065f55e3f18", my_json_string, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "messages/b13a15c0-5e5b-47b0-8aac-0065f55e3f18", 0);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);

        msg_id = esp_mqtt_client_subscribe(client, "messages/b13a15c0-5e5b-47b0-8aac-0065f55e3f18", 1);
        ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");

        break;

    case MQTT_EVENT_SUBSCRIBED:
       // ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "messages/b13a15c0-5e5b-47b0-8aac-0065f55e3f18", my_json_string, 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);

        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);



        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
            log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
            log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
            log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
            ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

        }

        break;
    default:

        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
    if(a==an1){a=an0;esp_mqtt_client_publish(client, "messages/b13a15c0-5e5b-47b0-8aac-0065f55e3f18", my_json_string, 0, 0, 0);}
                            else if(a==an2){a=an0; esp_mqtt_client_publish(client, "messages/b13a15c0-5e5b-47b0-8aac-0065f55e3f18", my_json_string, 0, 0, 0);}
                            else if(a==ang){a=an0;esp_mqtt_client_publish(client, "messages/b13a15c0-5e5b-47b0-8aac-0065f55e3f18", my_json_string, 0, 1, 0);}
}


static void mqtt_app_start(void)
{
    static esp_mqtt_client_config_t mqtt_cfg = {
    		.uri = CONFIG_BROKER_URL,
    						.username = "nnnn",
    						.password = "jMHdkyudVjE941Qt0qdsIrVSy5uzdQKI",
    };
#if CONFIG_BROKER_URL_FROM_STDIN
    char line[128];

    if (strcmp(mqtt_cfg.uri, "FROM_STDIN") == 0) {
        int count = 0;
        printf("Please enter url of mqtt broker\n");
        while (count < 128) {
            int c = fgetc(stdin);
            if (c == '\n') {
                line[count] = '\0';
                break;
            } else if (c > 0 && c < 127) {
                line[count] = c;
                ++count;
            }
            vTaskDelay(10 / portTICK_PERIOD_MS);
        }
        mqtt_cfg.uri = line;
        printf("Broker url: %s\n", line);
    } else {
        ESP_LOGE(TAG, "Configuration mismatch: wrong broker url");
        abort();
    }
#endif /* CONFIG_BROKER_URL_FROM_STDIN */

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    /* The last argument may be used to pass data to the event handler, in this example mqtt_event_handler */
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);
}

void app_main()
{

	Init_gpio_input();
	Init_gpio_output();
    ESP_LOGI(TAG, "[APP] Startup..");
    ESP_LOGI(TAG, "[APP] Free memory: %d bytes", esp_get_free_heap_size());
    ESP_LOGI(TAG, "[APP] IDF version: %s", esp_get_idf_version());

    esp_log_level_set("*", ESP_LOG_INFO);
    esp_log_level_set("MQTT_CLIENT", ESP_LOG_VERBOSE);
    esp_log_level_set("MQTT_EXAMPLE", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT_BASE", ESP_LOG_VERBOSE);
    esp_log_level_set("esp-tls", ESP_LOG_VERBOSE);
    esp_log_level_set("TRANSPORT", ESP_LOG_VERBOSE);
    esp_log_level_set("OUTBOX", ESP_LOG_VERBOSE);

    ESP_ERROR_CHECK(nvs_flash_init());
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(example_connect());

    mqtt_app_start();
	DHT11_init(4);
    xTaskCreate(&button_task, "button_task", 512, NULL, 2, NULL);
    //
   xTaskCreate(&echo_task, "uart_echo_task", ECHO_TASK_STACK_SIZE, NULL, 3, NULL);
  // xTaskCreate(&blink_task, "blink_task", 512, NULL, 5, NULL);
    mqtt_app_start();
    while(1){




nhayled5();


    }
}


