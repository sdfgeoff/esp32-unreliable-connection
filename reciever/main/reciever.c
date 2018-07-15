#include <stdio.h>
#include <string.h>
#include "nvs_flash.h"
#include "freertos/FreeRTOS.h"
#include "esp_wifi.h"
#include "esp_wifi_internal.h"
#include "esp_event_loop.h"
#include "freertos/task.h"
#include "sdkconfig.h"
#include "lwip/err.h"

/* Parameters for the transmitter */
#define WIFI_CHANNEL 1
// The BSSID is normally the MAC address of the router being talked through.
// In this case, I am misusing it as a means to identify that this is a
// control packet. A router with the "wrong" BSSID may cause issues, but there
// are 16 million addresses, so the chance of a collision is small.
// This must match the one in sender.c
#define BSSID 0x01, 0x02, 0x03, 0x04, 0x05, 0x06

// The data that gets sent between transmitter and reciever. This must match
// the one in sender.c
typedef struct {
	float x;
	float y;
} control_packet;


// Size of a 802.11 header (ie before the control_packet starts)
#define HEADER_SIZE 26

void handle_incoming(void* buff, wifi_promiscuous_pkt_type_t type) {
	/* Runs whenever there is an incoming packet */
	// Convert the packet into the correct packet type
	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

	const uint8_t bssid_tx[6] = {BSSID};
	if (memcmp(&bssid_tx, ppkt->payload+16, sizeof(bssid_tx)) != 0) {
		return;
	}

	// Display some useful stats on the received packet
	printf("CHAN=%02d, RSSI=%02d, LEN=%d\n",
		ppkt->rx_ctrl.channel,
		ppkt->rx_ctrl.rssi,
		ppkt->rx_ctrl.sig_len
	);

	// Display the contents of the received packet
	control_packet incoming_packet;
	for (int i=0; i<sizeof(control_packet); i++){
		((uint8_t*)(&incoming_packet))[i] = ppkt->payload[i+HEADER_SIZE];
	}
	printf("Got: x=%f, y=%f\n", incoming_packet.x, incoming_packet.y);
}


esp_err_t event_handler(void *ctx, system_event_t *event) {
	return ESP_OK;
}


void setup_listener(void){
	nvs_flash_init();
	tcpip_adapter_init();

	ESP_ERROR_CHECK( esp_event_loop_init(event_handler, NULL) );

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK( esp_wifi_start() );
	esp_wifi_set_channel(WIFI_CHANNEL, WIFI_SECOND_CHAN_NONE);

	//Set up our callback to be called whenever packets arrive.
	esp_wifi_set_promiscuous(true);
	wifi_promiscuous_filter_t filter;
	filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;
	esp_wifi_set_promiscuous_filter(&filter);
	esp_wifi_set_promiscuous_rx_cb(&handle_incoming);
}


void app_main()
{
	setup_listener();
}





