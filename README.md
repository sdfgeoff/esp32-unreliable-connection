# Esp32 Unreliable Connection
The ESP32 is a great little chip. You can use it for many IOT applications,
but it's low cost makes it interesting for other applications as well. in some
of these other applications, a developer may want to use the radio hardware in
an unreliable mode - where packets can get lost. This is advantageous over
a normal WIFI connection, or even the ESP_NOW protocol because it allows lower
latency communications.

For example, my planned use is for remote control, where a transmitter
broadcasts packets every few milliseconds, and the receiver listens for them.
In this case, I only care about the latest packet, and a couple milliseconds
delay due to a lost packet is not critical. In high noise situations where the
negotiation of a normal wifi connection may not be possible, some packets may
still get through, so they may as well be data packets eh?

Essentially, the two programs in this repository are a packet sniffer and a
packet injector. The intended use is that you inject packets with one, and
sniff them with the other. Because of this, there is potential for malicious
use as you can snoop on other data, and insert your own. The ESP32 does have
some internal safeguards preventing some packet types being sent, but it is
still your responsibility what you do with the code. If in doubt, check your
local laws and regulations.
