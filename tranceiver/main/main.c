#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_wifi.h"
//~ #include "esp_wifi_internal.h"
#include "esp_event_loop.h"

//~ #include "driver/gpio.h"
//~ #include "sdkconfig.h"
//~ #include "lwip/err.h"

#include "tranceiver.h"


typedef struct {
	float x;
	float y;
} control_packet;

#define CONTROL_PACKET_ID 0x1

uint8_t tranceiver_id[6] = {
	0x01, 0x02, 0x03, 0x04, 0x05, 0x06
};


esp_err_t event_handler(void *ctx, system_event_t *event) {
	return ESP_OK;
}




void send_task(void *pvParameter){
	printf("Arrgh\n");
	control_packet packet;
	packet.x = 0.0;
	packet.y = 0.0;

	tranceiver_init();
	tranceiver_set_id(tranceiver_id);
	tranceiver_set_channel(1);
	tranceiver_set_power(44); // ~10mw


	while(1){
		vTaskDelay(100 / portTICK_PERIOD_MS);
		packet.x += 0.1;
		tranceiver_send_packet(
			CONTROL_PACKET_ID,
			(uint8_t*)&packet,
			sizeof(packet)
		);
		printf("Send: x=%f, y=%f\n", packet.x, packet.y);
	}
}

void receive_task(void *pvParameter){
	QueueHandle_t rx_packet_queue = xQueueCreate(2, TRANCEIVER_MIN_QUEUE_SIZE + sizeof(control_packet));
	tranceiver_set_queue(CONTROL_PACKET_ID, rx_packet_queue);


	uint8_t latest_packet_raw[TRANCEIVER_MIN_QUEUE_SIZE + sizeof(control_packet)] = {0};
	tranceiver_packet_stats* latest_packet = (tranceiver_packet_stats*)&latest_packet_raw;
	control_packet* parsed_packet = (control_packet*)&latest_packet->data;
	while(1){
		xQueueReceive(rx_packet_queue, &latest_packet_raw, portMAX_DELAY);
		printf("RSSI: %d\nNoiseFloor: %d\nSize %d\n",
			latest_packet->rssi,
			latest_packet->noise_floor,
			latest_packet->data_length
		);

		if (sizeof(control_packet) == latest_packet->data_length){
			printf("Got: x=%f, y=%f\n\n", parsed_packet->x, parsed_packet->y);
		}
	}
}


void app_main()
{
	nvs_flash_init();
	tcpip_adapter_init();

	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

    xTaskCreate(&send_task, "send_test", configMINIMAL_STACK_SIZE+1000, NULL, 5, NULL);
    xTaskCreate(&receive_task, "receive_test", configMINIMAL_STACK_SIZE+1000, NULL, 5, NULL);
}
