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
#define ADVERTISING_INTERVAL_MIN 400
#define ADVERTISING_INTERVAL_MAX 400

//value = desired period in milliseconds * 0.8 (set to 75ms)
#define CONNECTION_INTERVAL_MIN 60
#define CONNECTION_INTERVAL_MAX 60

//value = "off the air" time (milliseconds) / connection interval (milliseconds) (set to 300ms) - 1
#define CONNECTION_SLAVE_LATENCY 4

//value = (1+slave_latency)*(connection_interval*2)+1
#define CONNECTION_SUPERVISION_TIMEOUT ((1+CONNECTION_SLAVE_LATENCY)*(75*2)+1)

#define CONNECTION_EVENT_LENGTH_MIN 0x0000
#define CONNECTION_EVENT_LENGTH_MAX 0x0004

ble_data_struct_t ble_data;

ble_data_struct_t* get_ble_data(){
  return &ble_data;
}

static bool connected = false;
static bool indication_inflight = false;
static bool indication_enabled = false;

uint8_t assignmentNumber = ASSIGNMENT_NUMBER;

bool indication_allowed(){
  return indication_enabled;
}

bool connection_established(){
  return connected;
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
      displayPrintf(DISPLAY_ROW_ASSIGNMENT, "A%u", assignmentNumber);
#if DEVICE_IS_BLE_SERVER
      displayPrintf(DISPLAY_ROW_NAME, "Server");
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
#else //DEVICE_IS_BLE_CLIENT
      displayPrintf(DISPLAY_ROW_NAME, "Client");
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif

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
#if DEVICE_IS_BLE_SERVER
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
          LOG_ERROR("BT STACK BOOTUP");
      }
#else //DEVICE_IS_BLE_CLIENT
      sl_bt_scanner_set_parameters(sl_bt_scanner_scan_mode_passive, 0x0010, 0x0010);
      sl_bt_connection_set_default_parameters(CONNECTION_INTERVAL_MIN,
                                              CONNECTION_INTERVAL_MAX,
                                              CONNECTION_SLAVE_LATENCY,
                                              CONNECTION_SUPERVISION_TIMEOUT,
                                              CONNECTION_EVENT_LENGTH_MIN,
                                              CONNECTION_EVENT_LENGTH_MAX);
      sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m, sl_bt_scanner_discover_observation);
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
#endif
      break;

#if DEVICE_IS_BLE_SERVER
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
          LOG_ERROR("BT STACK CONNECTION OPENED");
      }
#else //DEVICE_IS_BLE_CLIENT
      displayPrintf(DISPLAY_ROW_BTADDR2, "%x:%x:%x:%x:%x:%x",
                    evt->data.evt_connection_opened.address.addr[0],
                    evt->data.evt_connection_opened.address.addr[1],
                    evt->data.evt_connection_opened.address.addr[2],
                    evt->data.evt_connection_opened.address.addr[3],
                    evt->data.evt_connection_opened.address.addr[4],
                    evt->data.evt_connection_opened.address.addr[5]);
#endif
      displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
      connected = true;
      break;

    case sl_bt_evt_connection_closed_id:
#if DEVICE_IS_BLE_SERVER
      sl_
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);
      displayPrintf(DISPLAY_ROW_CONNECTION, "Advertising");
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
#else //DEVICE_IS_BLE_CLIENT
#endif

      if(sc != SL_STATUS_OK){
          //LOG_ERROR("BT STACK CONNECTION CLOSED");
      }

      connected = false;
      indication_enabled = false;
      break;

    /*
    case sl_bt_evt_connection_parameters_id:
      LOG_INFO("Connection Parameters Updated:\n");
      LOG_INFO("  Interval: %.2f ms\n", evt->data.evt_connection_parameters.interval * 1.25f);
      LOG_INFO("  Latency : %u\n", evt->data.evt_connection_parameters.latency);
      LOG_INFO("  Timeout : %.0f ms\n", evt->data.evt_connection_parameters.timeout * 10.0f);
      break;
    */

    case sl_bt_evt_system_external_signal_id:
      //Do nothing.
      //On Server, this is handled by temperature state machine.
      //On Client, this doesn't need to run at all.
      break;

    case sl_bt_evt_system_soft_timer_id:
      displayUpdate();
      break;

//Events exclusive to Server
#if DEVICE_IS_BLE_SERVER
    case sl_bt_evt_gatt_server_characteristic_status_id:
      //client config changed
      if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config) {
          uint16_t client_config_flags = evt->data.evt_gatt_server_characteristic_status.client_config_flags;

          if(evt->data.evt_gatt_server_characteristic_status.characteristic != gattdb_temperature_measurement){
              break; //Do not care about any indications aside from the temperature measurement
          }

          if(client_config_flags == sl_bt_gatt_server_disable)                           {indication_enabled = false;}
          else if(client_config_flags == sl_bt_gatt_server_indication)                   {indication_enabled = true;}
          else if(client_config_flags == sl_bt_gatt_server_notification)                 {indication_enabled = false;}
          else if(client_config_flags == sl_bt_gatt_server_notification_and_indication)  {indication_enabled = true;}

          if(indication_enabled){
            LETIMER_IntSet(LETIMER0, LETIMER_IF_UF); //Trigger an UF event to guarantee a temperature measurement is taken after indications are enabled
          } else {
            displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
          }

      //confirmation received
      } else if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation) {
          indication_inflight = false;
      }
      break;

    case sl_bt_evt_gatt_server_indication_timeout_id:
      indication_inflight = false;
      //LOG_ERROR("Indication Timed Out");
      break;

//Events exclusive to Client
#else //DEVICE_IS_BLE_CLIENT
    case sl_bt_evt_scanner_scan_report_id:
      //Handled by Discovery State Machine
      break;
    case sl_bt_evt_gatt_procedure_completed_id:
      //Handled by Discovery State Machine
      break;
    case sl_bt_evt_gatt_service_id:
      //Handled by Discovery State Machine
      break;
    case sl_bt_evt_gatt_characteristic_id:
      //Handled by Discovery State Machine
      break;
    case sl_bt_evt_gatt_characteristic_value_id:
      //Handled by Discovery State Machine
      break;
#endif


    default: //do nothing
      break;
    }

  (void) sc;
}

void send_temperature_reading(size_t value_len, const uint8_t* value){
  indication_inflight = true;
  sl_status_t sc = sl_bt_gatt_server_send_indication(connection_handle, gattdb_temperature_measurement, value_len, value);

  if(sc != SL_STATUS_OK){
      //LOG_ERROR("BT STACK INDICATION SEND");
  }

  int32_t temperature = FLOAT_TO_INT32(value);
  uint16_t temp_wholeDigits = (temperature & 0xFFFF0000) >> 16;
  uint16_t temp_tenthDigits = (temperature & 0x0000FFFF) >> 0;

  displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp = %i.%i Celsius", temp_wholeDigits, temp_tenthDigits);
}

void update_temperature_reading(size_t value_len, const uint8_t* value){
  sl_status_t sc = sl_bt_gatt_server_write_attribute_value(gattdb_temperature_measurement, 0, value_len, value);

  if(sc != SL_STATUS_OK){
      //LOG_ERROR("BT STACK GATTDB UPDATE");
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
