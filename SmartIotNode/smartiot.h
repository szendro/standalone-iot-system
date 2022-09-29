/*
*
* Smartiot.h 	15.09.2022		Christopher Collins
*
*/



// Packet Types Gateway/Node
#define ACK 0
#define NACK 1
#define DATA 2
#define LED_CMND 3
#define REG_CMND 4
#define CMND_WINDOW_OPEN 5
#define REG_VALUE 6
#define MAX_FIELD_LEN 4

// Commands Gateway/Website server
// If debug LED's are not used ACK_TIMEOUT can be set to 100 ms.
#define GREEN_LED_ON 1
#define GREEN_LED_OFF 0
#define MAX_PACKET_SIZE 24
#define ACK_TIMEOUT 300 
#define NO_RSSI 1000

// LED DIGITAL PINS
#define GREEN_LED 6
#define RED_LED 5