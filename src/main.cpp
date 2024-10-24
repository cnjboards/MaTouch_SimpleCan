#include <Arduino.h>
#include <ESP32-TWAI-CAN.hpp>

/* ESP32 Arduino CAN RTOS Example
 *     This example will send and receive messages on the CAN bus using RTOS tasks
 *     
 *     When controlling a device over CAN bus that device may expect a CAN message to be received at a specific frequency.
 *     Using RTOS tasks simplifies sending dataframes and makes the timing more consistent.
 *
 *     An external transceiver is required and should be connected 
 *     to the CAN_tx and CAN_rx gpio pins specified by CANInit. Be sure
 *     to use a 3.3V compatable transceiver such as the SN65HVD23x
 *
 */

// Uncomment for Station A, comment for Station B
//#define STATION_A // Station A is MaTouch 2.1" display

// Station A is MaTouch 2.1" display
// Wiki:https://wiki.makerfabs.com/MaTouch_ESP32_S3_2.1_Rotary_TFT_with_Touch.html

// Station B is MaTouch 1.28" display (GC9A01)
// Wiki:https://wiki.makerfabs.com/MaTouch_ESP32_S3_Rotary_IPS_Display_1.28_GC9A01.html

// Both using CNJBoards Can bus adapter card:
// 
#ifdef STATION_A
    #define CAN_TX GPIO_NUM_43 // on MaTouch this is on J2, pin 4
    #define CAN_RX GPIO_NUM_44 // on MaTouch this is on J2, pin 3
#else
    #define CAN_TX GPIO_NUM_4 // on MaTouch this is on J3, pin 4
    #define CAN_RX GPIO_NUM_5 // on MaTouch this is on J3, pin 3
#endif

/* RTOS priorities, higher number is more important */
#define CAN_TX_PRIORITY     3
#define CAN_RX_PRIORITY     1

#define CAN_TX_RATE_ms      1500 // how fast to send frames
#define CAN_RX_RATE_ms      500 // how fast to poll rx queue

/* can frame structures for Tx and Rx */
CanFrame txFrame;
CanFrame rxFrame;

/* CAN RTOS callback functions */
void canSend(void *pvParameters);
void canReceive(void *pvParameters);

/* CAN RTOS task handles */
static TaskHandle_t canTxTask = NULL;
static TaskHandle_t canRxTask = NULL;

// setup serial, tasks, can bus etc
void setup() {
  Serial.begin(115200);
  delay(2500);
  Serial.println("ESP32-Arduino-CAN RTOS Example");
  #ifdef STATION_A
    Serial.println("Station A");
  #else
    Serial.println("Station B");
  #endif
  
// Set pins for Can bus xciever, defines above
ESP32Can.setPins(CAN_TX, CAN_RX);

// setup some tx and rx queues
ESP32Can.setRxQueueSize(10);
ESP32Can.setTxQueueSize(10);

// .setSpeed() and .begin() functions require to use TwaiSpeed enum,
// but you can easily convert it from numerical value using .convertSpeed()
// 500kBps is typical automotive can bus speed
ESP32Can.setSpeed(ESP32Can.convertSpeed(500));


  /* setup can send RTOS task */
  xTaskCreatePinnedToCore(canSend,         /* callback function */
                          "CAN TX",        /* name of task */
                          1024,            /* stack size (bytes in ESP32, words in FreeRTOS */
                          NULL,            /* parameter to pass to function */
                          CAN_TX_PRIORITY, /* task priority (0 to configMAX_PRIORITES - 1 */
                          &canTxTask,      /* task handle */
                          1);              /* CPU core, Arduino runs on 1 */

  /* setup can receive RTOS task */
	xTaskCreatePinnedToCore(canReceive,      /* callback function */
                          "CAN RX",        /* name of task */
                          2048,            /* stack size (bytes in ESP32, words in FreeRTOS */
                          NULL,            /* parameter to pass to function */
                          CAN_RX_PRIORITY, /* task priority (0 to configMAX_PRIORITES - 1 */
                          &canRxTask,      /* task handle */
                          1);              /* CPU core, Arduino runs on 1 */

    // prebuild tx frame
    txFrame.extd = 0;
    #if defined(STATION_A)
    txFrame.identifier = 0x123; // just pick random id
    #else
    txFrame.identifier = 0x122;
    #endif

    // build fixed tx frame
    txFrame.data_length_code = 8;
    for (int8_t i= 0; i<8; i++) {
        txFrame.data[i] = 0xAA;     /* pad frame with 0xFF */
    }

    // It is also safe to use .begin() without .end() as it calls it internally
    if(ESP32Can.begin(ESP32Can.convertSpeed(500), CAN_TX, CAN_RX, 10, 10)) {
        Serial.println("CAN bus started!");
    } else {
        Serial.println("CAN bus failed!");
    } // end if
} // end setup


// main processing loop, tx and rx are handled by tasks
void loop() {
  /* place time in seconds into first two bytes of dataframe */
  uint16_t time = millis()/1000;
  txFrame.data[0] = highByte(time);
  txFrame.data[1] = lowByte(time);
  delay(100);
} // end loop

/* function to send a can frame */
void canSend(void *pvParameters) {
	TickType_t xLastWakeTime; /* keep track of last time can message was sent */
	TickType_t xFrequency = CAN_TX_RATE_ms / portTICK_PERIOD_MS; /* set the transmit frequency */

  /* this task will run forever at frequency set above 
   * to stop this task from running call vTaskSuspend(canTxTask) in the main loop */
	for (;;) {
		ESP32Can.writeFrame(&txFrame);           /* send dataframe */

    // debug tx frame
    Serial.print("Sending Can Frame timestamp:0x");
    Serial.print(txFrame.data[0]);
    Serial.print(txFrame.data[1]);
    Serial.println("");

		vTaskDelayUntil(&xLastWakeTime, xFrequency); /* do something else until it is time to send again */
        /* the above delay function was used since it specifies an absolute wake time. 
         * Make sure the code in the forever for loop can run faster then desired send frequency or this task will take all of the CPU time available */
	}
} // end canSend

// Fuction to receive and print a valid can frame
void canReceive(void *pvParameters) {
	const TickType_t xDelay = CAN_RX_RATE_ms / portTICK_PERIOD_MS;
  
  // RX Task startup message
  Serial.println("Can RX Waiting");

  // wait forever for rx data
	for (;;) {
    if (ESP32Can.readFrame(&rxFrame)) {  /* only print when CAN message is received*/
      Serial.print("RX Frame: id=");
      Serial.print(rxFrame.identifier, HEX);               /* print the CAN ID*/
      Serial.print(" Length=");
      Serial.print(rxFrame.data_length_code);              /* print number of bytes in data frame*/
      Serial.print(" Payload=");
      for (int i=0; i<rxFrame.data_length_code; i++) {     /* print the data frame*/
        Serial.print(rxFrame.data[i], HEX);
      } // end for

      Serial.println();
    } // end if
		vTaskDelay(xDelay); /* do something else until it is time to receive again. This is a simple delay task. */
	}// end for
} // end canRecieve
