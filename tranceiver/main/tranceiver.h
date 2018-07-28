#ifndef __tranciever_h__
#define __tranciever_h__


#define TRANCEIVER_MAX_PACKET_BYTES 64


/* In order to support multiple packet types, the tranceiver associates each
 * packet type with a uint8_t. This allows up to 256 different packet types.
 *
 * Technically, this is sent as the first byte of the packet.
*/
typedef uint8_t tranciever_packet_type;

/* Callbacks are passed in data about the received packet
 */
typedef struct {
	tranciever_packet_type packet_type;
	int8_t rssi; //dbm
	int8_t noise_floor; //in multiples of 0.25 dbm
	uint8_t data_length;
	uint8_t data[];
} tranceiver_packet_stats;

#define TRANCEIVER_MIN_QUEUE_SIZE sizeof(tranceiver_packet_stats)-1


/* When receiving a packet, a callback function is called. This is expected
 * to be a function that returns void that takes three arguments:
 *  - Metadata about the received packet
 *  - The data buffer
 *  - The length of the data
 *  -
*/
typedef void (*tranciever_callback_t(tranceiver_packet_stats packet_type, uint8_t data[], uint16_t buffer_len));


/*
 * Send a packet, claiming it to be one of a specified type
 * Returns nonzero if not sent
*/
uint8_t tranceiver_send_packet(tranciever_packet_type packet_type, uint8_t data[], uint16_t data_len);


/* Register a queue for a particular packet type. To de register a queue it,
 * pass in NULL (ideally before destroying the queue)
 * Note that the queue should be sizeof(data)+TRANCEIVER_MIN_QUEUE_SIZE
*/
void tranceiver_set_queue(tranciever_packet_type packet_type, QueueHandle_t* queue);


/* Sets the transmit power. Lower numbers are more power. It appears that
 * the number provided here is about equal to the power in dbm/4. Eg:
 *  - set_transmit_power(78) will transmit at about 19.5dbm (90mW)
 *  - set_transmit_power(44) will transmit at about 11dbm (12.6mW)
 *
 * There is some granularity, so not all values are unique. For more info,
 * see the ESP32 docs on esp_wifi_set_max_tx_power
 *
 * Check your local regulations for the maximum permissible.
 */
void tranceiver_set_power(int8_t transmit_power);


/* Sets the transmission channel. Check your countries regulations.
 * (most countries allow use of 1 - 11. Some countries restrict the use of
 * 12-14)
 */
void tranceiver_set_channel(uint8_t channel);


/*
 * To differentiate the transceiver packets from normal wifi traffic, these
 * bytes are sent as an identifier.
 *
 * This value must be the same between multiple devices that wish to communicate
 */
void tranceiver_set_id(uint8_t id_bytes[6]);


void tranceiver_init();

#endif
