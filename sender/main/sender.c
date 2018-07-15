#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_internal.h"
#include "esp_event_loop.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "lwip/err.h"


/* Parameters for the transmitter */
#define WIFI_CHANNEL 1
// The BSSID is normally the MAC address of the router being talked through.
// In this case, I am misusing it as a means to identify that this is a
// control packet. A router with the "wrong" BSSID may cause issues, but there
// are 16 million addresses, so the chance of a collision is small
// This must match the one in reciever.c
#define BSSID 0x01, 0x02, 0x03, 0x04, 0x05, 0x06

// The data that gets sent between transmitter and reciever. This must match
// the one in reciever.c
typedef struct {
	float x;
	float y;
} control_packet;




const uint8_t packet_header[] = {
	0x08, 0x00, // Data packet (normal subtype)
	0x00, 0x00, // Duration and ID
	0xff, 0xff, 0xff, 0xff, 0xff, 0xff, // Recipient Address
	0x30, 0xae, 0xa4, 0x7c, 0x9b, 0x88, // Transmitter Address
	BSSID, // BSSID Address
	0x00, 0x00, // Sequence Control
	0x00, 0x00, // QOS control

	/* Data gets substituted here */
	/* Then the ESP automatically adds the checksum for us */
};


esp_err_t event_handler(void *ctx, system_event_t *event) {
	return ESP_OK;
}


void setup_wifi(void){
	nvs_flash_init();
	tcpip_adapter_init();

	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK( esp_wifi_start() );
	esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);
}


void send_control_packet(const control_packet* data){

	uint8_t raw_packet[sizeof(packet_header) + sizeof(control_packet)] = {0};

	memcpy(&raw_packet, &packet_header, sizeof(packet_header));

	//memcpy(&raw_packet+sizeof(packet_header), (uint8_t*)data, sizeof(control_packet));
	// For some reason the memcpy isn't working right (always all zeros?!).
	// I'm not sure why, but anyway, here's my own little version
	for (int i=0; i<sizeof(control_packet); i++){
		raw_packet[i+sizeof(packet_header)] = ((uint8_t*)data)[i];
	}

	ESP_ERROR_CHECK(esp_wifi_80211_tx(
		ESP_IF_WIFI_STA,
		(void*)&raw_packet, sizeof(raw_packet),
		false
	));
}


void send_task(void *pvParameter){
	control_packet packet;
	packet.x = 0.0;
	packet.y = 0.0;


	while(1){
		vTaskDelay(100 / portTICK_PERIOD_MS);
		packet.x += 0.1;
		send_control_packet(&packet);
		printf("Send: x=%f, y=%f\n", packet.x, packet.y);
	}
}


void app_main()
{
	setup_wifi();
    xTaskCreate(&send_task, "send_test", configMINIMAL_STACK_SIZE+1000, NULL, 5, NULL);
}





