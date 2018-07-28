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

#include "tranceiver.h"


/* Parameters for the transmitter */
#define DEFAULT_WIFI_CHANNEL 1
#define DEFAULT_TRANSMIT_POWER 8 //2dbm = 1.5mW

// The BSSID is normally the MAC address of the router being talked through.
// In this case, I am misusing it as a means to identify that this is a
// control packet. A router with the "wrong" BSSID may cause issues, but there
// are 16 million addresses, so the chance of a collision is small
// This must match the one in reciever.c
#define DEFAULT_ID 0x01, 0x02, 0x03, 0x04, 0x05, 0x06

// The data that gets sent between transmitter and reciever. This must match
// the one in reciever.c



QueueHandle_t* tranceiver_queues[256] = {NULL};


uint8_t packet_header[] = {
	0x08, 0x00, // Data packet (normal subtype)
	0x00, 0x00, // Duration and ID
	DEFAULT_ID, // Recipient Address
	DEFAULT_ID, // Transmitter Address
	DEFAULT_ID, // BSSID Address
	0x00, 0x00, // Sequence Control
	0x00, 0x00, // QOS control

	/* Data gets substituted here */
	/* Then the ESP automatically adds the checksum for us */
};

#define ID_OFFSET 4
#define ID_LENGTH 6


uint8_t tx_packet_buffer[sizeof(packet_header) + TRANCEIVER_MAX_PACKET_BYTES] = {0};
uint8_t rx_packet_buffer[sizeof(tranceiver_packet_stats) + TRANCEIVER_MAX_PACKET_BYTES] = {0};


void tranceiver_set_channel(uint8_t channel){
	esp_wifi_set_channel(channel, WIFI_SECOND_CHAN_NONE);
}


void tranceiver_set_power(int8_t power){
	esp_wifi_set_max_tx_power(power);
}


void tranceiver_set_id(uint8_t id_bytes[6]){
	for (uint8_t i=0; i<3; i++){
		for (uint8_t j=0; j<ID_LENGTH; j++){
			packet_header[ID_OFFSET+i*ID_LENGTH + j] = id_bytes[j];
		}
	}
}

static void _handle_data_packet(void* buff, wifi_promiscuous_pkt_type_t type) {
	/* Runs whenever there is an incoming packet */
	// Convert the packet into the correct packet type
	const wifi_promiscuous_pkt_t *ppkt = (wifi_promiscuous_pkt_t *)buff;

	// Display some useful stats on the received packet


	for (int i=0; i<3; i++){
		if (memcmp(packet_header+ID_OFFSET, ((ppkt->payload)+ID_OFFSET + i*ID_LENGTH), ID_LENGTH) != 0) {
			return;
		}
	}

	//~ for (uint16_t i=0; i<ppkt->rx_ctrl.sig_len; i++){
		//~ printf(" 0x%x", ppkt->payload[i]);
	//~ }
	//~ printf("\n");
	uint8_t packet_type = ppkt->payload[sizeof(packet_header)];
	//printf("Got Packet %d\n", packet_type);
	if (tranceiver_queues[packet_type] == NULL){
		return;
	}

	uint16_t data_len = ppkt->rx_ctrl.sig_len - sizeof(packet_header) - 1 - 4;  //magic numbers are 1=packet type 4=crc bytes
	if (data_len > TRANCEIVER_MAX_PACKET_BYTES){
		return;
	}

	// Make metadata and data continuous in memory
	((tranceiver_packet_stats*)rx_packet_buffer)->rssi = ppkt->rx_ctrl.rssi;
	((tranceiver_packet_stats*)rx_packet_buffer)->noise_floor = ppkt->rx_ctrl.noise_floor;
	((tranceiver_packet_stats*)rx_packet_buffer)->data_length = data_len;
	memcpy(
		((tranceiver_packet_stats*)rx_packet_buffer)->data,
		(ppkt->payload)+sizeof(packet_header) + 1,
		data_len
	);

	xQueueSend(
		tranceiver_queues[packet_type],
		rx_packet_buffer,
		0
	);
}


void tranceiver_init(void){
	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
	ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
	ESP_ERROR_CHECK( esp_wifi_start() );

	//Set up a callback to be called whenever packets arrive.
	esp_wifi_set_promiscuous(true);
	wifi_promiscuous_filter_t filter;
	filter.filter_mask = WIFI_PROMIS_FILTER_MASK_DATA;  // Only data packets
	esp_wifi_set_promiscuous_filter(&filter);
	esp_wifi_set_promiscuous_rx_cb(&_handle_data_packet);


	tranceiver_set_channel(DEFAULT_WIFI_CHANNEL);
	tranceiver_set_power(DEFAULT_TRANSMIT_POWER);
}


void tranceiver_set_queue(tranciever_packet_type packet_type, QueueHandle_t* queue){
	tranceiver_queues[packet_type] = queue;
}


uint8_t tranceiver_send_packet(tranciever_packet_type packet_type, uint8_t data[], uint16_t data_len){
	if (data_len > TRANCEIVER_MAX_PACKET_BYTES){
		return 1;
	}

	memcpy(&tx_packet_buffer, &packet_header, sizeof(packet_header));

	tx_packet_buffer[sizeof(packet_header)] = packet_type;  //Remember, zero indexed

	//memcpy(&raw_packet+sizeof(packet_header), (uint8_t*)data, sizeof(control_packet));
	// For some reason the memcpy isn't working right (always all zeros?!).
	// I'm not sure why, but anyway, here's my own little version
	for (int i=0; i<data_len; i++){
		tx_packet_buffer[sizeof(packet_header) + 1 + i] = data[i];
	}

	ESP_ERROR_CHECK(esp_wifi_80211_tx(
		ESP_IF_WIFI_STA,
		(void*)&tx_packet_buffer, sizeof(packet_header) + data_len + 1,
		false
	));

	return 0;
}





