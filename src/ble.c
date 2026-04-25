/*
 * ble.c
 *
 *  Created on: Feb 18, 2026
 *      Author: donav
 */

#include "ble.h"

#include "math.h"

// Include logging specifically for this .c file
#define INCLUDE_LOG_DEBUG 1
#include "src/log.h"

//value = desired period in milliseconds * 1.6 (set to 250ms)
#define ADVERTISING_INTERVAL_MIN 200
#define ADVERTISING_INTERVAL_MAX 200

//value = desired period in milliseconds * 0.8 (set to 75ms)
#define CONNECTION_INTERVAL_MIN 60
#define CONNECTION_INTERVAL_MAX 60

//value = "off the air" time (milliseconds) / connection interval (milliseconds) (set to 300ms) - 1
#define CONNECTION_SLAVE_LATENCY 2

//value = (1+slave_latency)*(connection_interval*2)+1
#define CONNECTION_SUPERVISION_TIMEOUT ((1+CONNECTION_SLAVE_LATENCY)*(75*2)+1)

#define CONNECTION_EVENT_LENGTH_MIN 0x0000
#define CONNECTION_EVENT_LENGTH_MAX 0x0004

static void sendIndication(uint16_t characteristic, size_t value_len, uint8_t* value);

static uint32_t nextPtr(uint32_t ptr);
static bool isEmpty();
static bool isFull();

static ble_data_struct_t ble_data;

ble_data_struct_t* get_ble_data(){
  return &ble_data;
}

uint8_t assignmentNumber = ASSIGNMENT_NUMBER;

bool connection_established(){
  return ble_data.connected;
}

// Connection handle.
static uint8_t connection_handle = 0;

#if DEVICE_IS_BLE_SERVER
// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;
#endif

//This function is heavily adapted and based on the bt-soc-thermometer example project
void handle_ble_event(sl_bt_msg_t *evt){
  sl_status_t sc;

  switch (SL_BT_MSG_ID(evt->header)) {

//Events shared by Server and Client
    case sl_bt_evt_system_boot_id:

      //Handle Display aspects first
      displayInit();
      sensor_init();
      displayPrintf(DISPLAY_ROW_TITLE, "Capacitive Keypad");
      displayPrintf(DISPLAY_ROW_STUDENT, "Donavon Facey");
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, "Final Project");
      displayPrintf(DISPLAY_ROW_NAME, "Server");
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
      //sl_bt_sm_delete_bondings();
      sl_bt_sm_set_bondable_mode(1);
      sl_bt_sm_configure(SL_BT_SM_CONFIGURATION_MITM_REQUIRED | SL_BT_SM_CONFIGURATION_BONDING_REQUEST_REQUIRED | SL_BT_SM_CONFIGURATION_PREFER_MITM, sm_io_capability_displayyesno);

      sc = sl_bt_system_get_identity_address(&ble_data.myAddress, sl_bt_gap_public_address);
      if(sc != SL_STATUS_OK){
          //LOG_ERROR("ADDRESS GET");
      }
      displayPrintf(DISPLAY_ROW_BTADDR, "%x:%x:%x:%x:%x:%x",
                    ble_data.myAddress.addr[0],
                    ble_data.myAddress.addr[1],
                    ble_data.myAddress.addr[2],
                    ble_data.myAddress.addr[3],
                    ble_data.myAddress.addr[4],
                    ble_data.myAddress.addr[5]);

      //Handle BLE advertising / discovery initialization here
      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&ble_data.myAddress, sl_bt_gap_public_address);

      // Create an advertising set.
      sc = sl_bt_advertiser_create_set(&advertising_set_handle);

      // Generate data for advertising
      sc = sl_bt_legacy_advertiser_generate_data(advertising_set_handle, sl_bt_advertiser_general_discoverable);

      // Set advertising interval to 250ms.
      sc = sl_bt_advertiser_set_timing(
        advertising_set_handle, // advertising set handle
        ADVERTISING_INTERVAL_MIN, // min. adv. interval (milliseconds * 1.6)
        ADVERTISING_INTERVAL_MAX, // max. adv. interval (milliseconds * 1.6)
        0,   // adv. duration
        0);  // max. num. adv. events

      // Start advertising and enable connections.
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);

      if(sc != SL_STATUS_OK){
          //LOG_ERROR("BT STACK BOOTUP");
      }
      break;

    case sl_bt_evt_connection_opened_id:
      connection_handle = evt->data.evt_connection_opened.connection;
      sc = sl_bt_advertiser_stop(advertising_set_handle);
      sc = sl_bt_connection_set_parameters(connection_handle,
                                      CONNECTION_INTERVAL_MIN,
                                      CONNECTION_INTERVAL_MAX,
                                      CONNECTION_SLAVE_LATENCY,
                                      CONNECTION_SUPERVISION_TIMEOUT,
                                      CONNECTION_EVENT_LENGTH_MIN,
                                      CONNECTION_EVENT_LENGTH_MAX);

      if(sc != SL_STATUS_OK){
          //LOG_ERROR("BT STACK CONNECTION OPENED");
      }

      ble_data.connected = true;

      if(evt->data.evt_connection_opened.bonding){
          displayPrintf(DISPLAY_ROW_CONNECTION, "Bonded");
      } else {
          displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
      }
      sl_bt_sm_increase_security(connection_handle);
      break;

    case sl_bt_evt_connection_closed_id:
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
      displayPrintf(DISPLAY_ROW_TOUCH_VALUE, "");
      displayPrintf(DISPLAY_ROW_PROXIMITY_VALUE, "");

      if(sc != SL_STATUS_OK){
          //LOG_ERROR("BT STACK CONNECTION CLOSED");
      }

      ble_data.connected = false;
      ble_data.touch_indication_enabled = false;
      ble_data.proximity_indication_enabled = false;
      gpioLed0SetOff();
      gpioLed1SetOff();
      //sl_bt_sm_delete_bondings();
      break;

    case sl_bt_evt_connection_parameters_id:
      if(evt->data.evt_connection_parameters.security_mode > sl_bt_connection_mode1_level1){
          displayPrintf(DISPLAY_ROW_CONNECTION, "Bonded");
          ble_data.bonded = true;
      }
      break;

    case sl_bt_evt_system_external_signal_id:
      //On Server, handle PB0 presses
      if(Scheduler_Active_PB0_pressed(evt)){
          if(ble_data.connected && !ble_data.bonded){
              sl_bt_sm_passkey_confirm(connection_handle,1);
              displayPrintf(DISPLAY_ROW_ACTION, "");
              displayPrintf(DISPLAY_ROW_PASSKEY, "");
          }
      }
      if(Scheduler_Active_PB0_released(evt)){
          //Do nothing
      }
      break;

    case sl_bt_evt_system_soft_timer_id:
      if(evt->data.evt_system_soft_timer.handle == SOFT_TIMER_HANDLE_LCD){
          displayUpdate();
      }
      break;

//Events exclusive to Server
    case sl_bt_evt_gatt_server_characteristic_status_id:
      //client config changed
      if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config) {
          uint16_t client_config_flags = evt->data.evt_gatt_server_characteristic_status.client_config_flags;

          bool indication_enabled = false;

          if(client_config_flags == sl_bt_gatt_server_disable)                           {indication_enabled = false;}
          else if(client_config_flags == sl_bt_gatt_server_indication)                   {indication_enabled = true;}
          else if(client_config_flags == sl_bt_gatt_server_notification)                 {indication_enabled = false;}
          else if(client_config_flags == sl_bt_gatt_server_notification_and_indication)  {indication_enabled = true;}

          if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_touch_state){
              ble_data.touch_indication_enabled = indication_enabled;
              if(ble_data.touch_indication_enabled){
                  gpioLed0SetOn();
              } else {
                  gpioLed0SetOff();
              }
          } else if(evt->data.evt_gatt_server_characteristic_status.characteristic == gattdb_proximity_state){
              ble_data.proximity_indication_enabled = indication_enabled;
              if(ble_data.proximity_indication_enabled){
                  gpioLed1SetOn();
              } else {
                  gpioLed1SetOff();
              }
          }

      //confirmation received
      } else if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation) {
          ble_data.indication_inflight = false;
          sendIndication(0, 0, 0); // send new Indication without queueing anything
      }
      break;

    case sl_bt_evt_gatt_server_indication_timeout_id:
      ble_data.indication_inflight = false;
      //LOG_ERROR("Indication Timed Out");
      break;

    case sl_bt_evt_sm_bonding_failed_id:
      sl_bt_sm_delete_bondings();
      break;
    case sl_bt_evt_sm_confirm_bonding_id:
      sl_bt_sm_bonding_confirm(connection_handle, 1);
      break;
    case sl_bt_evt_sm_confirm_passkey_id:
      displayPrintf(DISPLAY_ROW_ACTION, "Confirm with PB0");
      displayPrintf(DISPLAY_ROW_PASSKEY, "Passkey: %06u", evt->data.evt_sm_confirm_passkey.passkey);
      break;
    case sl_bt_evt_sm_bonded_id:
      break;

    default: //do nothing
      break;
    }

  (void) sc;
}

//If possible, sends indication from front of queue. Optionally queues a new indication to send if characteristic != -1.
static void sendIndication(uint16_t characteristic, size_t value_len, uint8_t* value){
  sl_status_t sc = SL_STATUS_OK;

  //Queue indication if wanting to queue
  if(characteristic != 0){
      write_queue(characteristic, value_len, value);
  }

  //Reset Queue if no connection is established
  if(ble_data.connected == false){
      reset_queue();
      return;
  }

  //if indication is already inflight, return
  if(ble_data.indication_inflight){
      return;
  }

  uint16_t charHandle;
  uint32_t bufLength;
  uint8_t buffer[MAX_BUFFER_LENGTH];

  if(read_queue(&charHandle, &bufLength, buffer)){
      //buffer either empty or error.
      return;
  }

  //Check conditions for sending indications
  if( (charHandle == gattdb_touch_state && ble_data.touch_indication_enabled && ble_data.bonded) ||
     (charHandle == gattdb_proximity_state && ble_data.proximity_indication_enabled && ble_data.bonded)){
      sc = sl_bt_gatt_server_send_indication(connection_handle, charHandle, bufLength, buffer);
      static uint32_t indications_sent;
      indications_sent++;
      ble_data.indication_inflight = true;
  }

  if(sc != SL_STATUS_OK){
      //LOG_ERROR("Sending Indication Error");
  }
}

void update_sensor_reading(uint16_t touch_value, uint8_t proximity_value){
  displayPrintf(DISPLAY_ROW_TOUCH_VALUE, "Touch = 0x%03X\n", touch_value);
  displayPrintf(DISPLAY_ROW_PROXIMITY_VALUE, "Proximity = 0x%01X\n", proximity_value);

  touch_value = (touch_value << 8) | (touch_value >> 8);
  sl_status_t sc;
  sc = sl_bt_gatt_server_write_attribute_value(gattdb_touch_state, 0, 2, (uint8_t*) &touch_value);
  sc = sl_bt_gatt_server_write_attribute_value(gattdb_proximity_state, 0, 1, &proximity_value);

  sendIndication(gattdb_touch_state, 2, (uint8_t*) &touch_value);
  sendIndication(gattdb_proximity_state, 1, &proximity_value);

  if(sc != SL_STATUS_OK){
      //LOG_ERROR("BT STACK INDICATION SEND");
  }
}

// -----------------------------------------------
// Private function, original from Dan Walkes. I fixed a sign extension bug.
// We'll need this for Client A7 assignment to convert health thermometer
// indications back to an integer. Convert IEEE-11073 32-bit float to signed integer.
//
// 2/26: Modified to include fractional digits. Upper 16 bits are in integer, lower 16 are in tenths.
// -----------------------------------------------
int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr)
{
 uint8_t signByte = 0;
 int32_t mantissa;
 // input data format is:
 // [0] = flags byte, bit[0] = 0 -> Celsius; =1 -> Fahrenheit
 // [3][2][1] = mantissa (2's complement)
 // [4] = exponent (2's complement)
 // BT buffer_ptr[0] has the flags byte
 int8_t exponent = (int8_t)buffer_ptr[4];
 // sign extend the mantissa value if the mantissa is negative
 if (buffer_ptr[3] & 0x80) { // msb of [3] is the sign of the mantissa
 signByte = 0xFF;
 }
 mantissa = (int32_t) (buffer_ptr[1] << 0) |
 (buffer_ptr[2] << 8) |
 (buffer_ptr[3] << 16) |
 (signByte << 24) ;
 // value = 10^exponent * mantissa, pow() returns a double type
 int16_t wholeDigits = (int16_t) (pow(10, exponent) * mantissa);
 int16_t tenthDigits = ((int16_t) (pow(10, exponent+1) * mantissa)) %10;

 return ((wholeDigits << 16) | tenthDigits);

} // FLOAT_TO_INT32

//////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                            ///
///       Circular Buffer Code below (taken from assignment 0.5)                               ///
///                                                                                            ///
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <string.h> // for memcpy()

// ---------------------------------------------------------------------
// Private function used only by this .c file.
// Compute the next ptr value. Given a valid ptr value, compute the next valid
// value of the ptr and return it.
// Isolation of functionality: This defines "how" a pointer advances.
// ---------------------------------------------------------------------
static uint32_t nextPtr(uint32_t ptr)
{
  return ((ptr + 1) % QUEUE_DEPTH);

} // nextPtr()

// ---------------------------------------------------------------------
// Private function used only by this .c file.
// Returns if buffer is full
// ---------------------------------------------------------------------
static bool isFull()
{
  return ((ble_data.wptr + 1) % QUEUE_DEPTH) == ble_data.rptr;

} // isFull()

// ---------------------------------------------------------------------
// Private function used only by this .c file.
// Returns if buffer is full
// ---------------------------------------------------------------------
static bool isEmpty()
{
  return ble_data.wptr == ble_data.rptr;

} // isEmpty()



// ---------------------------------------------------------------------
// Public function.
// This function resets the queue.
// ---------------------------------------------------------------------
void reset_queue (void)
{
  ble_data.wptr = 0;
  ble_data.rptr = 0;
} // reset_queue()



// ---------------------------------------------------------------------
// Public function.
// This function writes an entry to the queue if the the queue is not full.
// Input parameter "charHandle" should be written to queue_struct_t element "charHandle".
// Input parameter "bufLength" should be written to queue_struct_t element "bufLength"
// The bytes pointed at by input parameter "buffer" should be written to queue_struct_t element "buffer"
// Returns bool false if successful or true if writing to a full fifo.
// i.e. false means no error, true means an error occurred.
// ---------------------------------------------------------------------
bool write_queue (uint16_t charHandle, uint32_t bufLength, uint8_t *buffer)
{
  if(!buffer){
    return true;
  }

  if(bufLength > MAX_BUFFER_LENGTH || bufLength < MIN_BUFFER_LENGTH){
    return true;
  }

  if(isFull()){
    return true;
  }

  ble_data.indication_queue[ble_data.wptr].charHandle = charHandle;
  ble_data.indication_queue[ble_data.wptr].bufLength = bufLength;
  memcpy(&ble_data.indication_queue[ble_data.wptr].buffer, buffer, bufLength);

  ble_data.wptr = nextPtr(ble_data.wptr);

  return false;

} // write_queue()



// ---------------------------------------------------------------------
// Public function.
// This function reads an entry from the queue, and returns values to the
// caller. The values from the queue entry are returned by writing
// the values to variables declared by the caller, where the caller is passing
// in pointers to charHandle, bufLength and buffer. The caller's code will look like this:
//
//   uint16_t     charHandle;
//   uint32_t     bufLength;
//   uint8_t      buffer[5];
//
//   status = read_queue (&charHandle, &bufLength, &buffer[0]);
//
// *** If the code above doesn't make sense to you, you probably lack the
// necessary prerequisite knowledge to be successful in this course.
//
// Write the values of charHandle, bufLength, and buffer from my_queue[rptr] to
// the memory addresses pointed at by charHandle, bufLength and buffer, like this :
//      *charHandle = <something>;
//      *bufLength  = <something_else>;
//      *buffer     = <something_else_again>; // perhaps memcpy() would be useful?
//
// In this implementation, we do it this way because
// standard C does not provide a mechanism for a C function to return multiple
// values, as is common in perl or python.
// Returns bool false if successful or true if reading from an empty fifo.
// i.e. false means no error, true means an error occurred.
// ---------------------------------------------------------------------
bool read_queue (uint16_t *charHandle, uint32_t *bufLength, uint8_t *buffer)
{
  if(!buffer || !bufLength || !charHandle){
    return true;
  }

  if(isEmpty()){
    return true;
  }

  *charHandle = ble_data.indication_queue[ble_data.rptr].charHandle;
  *bufLength = ble_data.indication_queue[ble_data.rptr].bufLength;
  memcpy(buffer, &ble_data.indication_queue[ble_data.rptr].buffer, ble_data.indication_queue[ble_data.rptr].bufLength);

  ble_data.rptr = nextPtr(ble_data.rptr);

  return false;

} // read_queue()



// ---------------------------------------------------------------------
// Public function.
// This function returns the wptr, rptr, full and empty values, writing
// to memory using the pointer values passed in, same rationale as read_queue()
// The "_" characters are used to disambiguate the global variable names from
// the input parameter names, such that there is no room for the compiler to make a
// mistake in interpreting your intentions.
// ---------------------------------------------------------------------
void get_queue_status (uint32_t *_wptr, uint32_t *_rptr, bool *_full, bool *_empty)
{
  if(!_wptr || !_rptr || !_full || !_empty){
    return;
  }

  *_wptr = ble_data.wptr;
  *_rptr = ble_data.rptr;
  *_full = isFull();
  *_empty = isEmpty();

} // get_queue_status()



// ---------------------------------------------------------------------
// Public function.
// Function that computes the number of written entries currently in the queue. If there
// are 3 entries in the queue, it should return 3. If the queue is empty it should
// return 0. If the queue is full it should return either QUEUE_DEPTH if
// USE_ALL_ENTRIES==1 otherwise returns QUEUE_DEPTH-1.
// ---------------------------------------------------------------------
uint32_t get_queue_depth()
{
  if(isEmpty()){       //  EMPTY
    return 0;
  } else if (isFull()){   //  FULL
    return QUEUE_DEPTH-1;
  } else {                //  PARTIALLY FILLED
    return (QUEUE_DEPTH + ble_data.wptr - ble_data.rptr) % QUEUE_DEPTH;
  }
} // get_queue_depth()
