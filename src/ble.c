/*
 * ble.c
 *
 *  Created on: Feb 18, 2026
 *      Author: donav
 */

#include "ble.h"

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
#define CONNECTION_SLAVE_LATENCY 3

//value = (1+slave_latency)*(connection_interval*2)+1
#define CONNECTION_SUPERVISION_TIMEOUT ((1+CONNECTION_SLAVE_LATENCY)*(75*2)+1)

#define CONNECTION_EVENT_LENGTH_MIN 0x0000
#define CONNECTION_EVENT_LENGTH_MAX 0xffff

ble_data_struct_t ble_data;

ble_data_struct_t* get_ble_data(){
  return &ble_data;
}

static bool connected = false;
static bool indication_inflight = false;
static bool indication_enabled = false;

bool indication_allowed(){
  return indication_enabled;
}

bool connection_established(){
  return connected;
}

// Connection handle.
static uint8_t connection_handle = 0;

// The advertising set handle allocated from Bluetooth stack.
static uint8_t advertising_set_handle = 0xff;

//This function is heavily adapted and based on the bt-soc-thermometer example project
void handle_ble_event(sl_bt_msg_t *evt){
  sl_status_t sc;
  bd_addr address;
  uint8_t address_type;

  switch (SL_BT_MSG_ID(evt->header)) {
    case sl_bt_evt_system_boot_id:

      // Extract unique ID from BT Address.
      sc = sl_bt_system_get_identity_address(&address, &address_type);

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
          LOG_ERROR("BT STACK CONNECTION OPENED");
      }

      connected = true;
      break;

    case sl_bt_evt_connection_closed_id:
      sc = sl_bt_legacy_advertiser_start(advertising_set_handle, sl_bt_advertiser_connectable_scannable);

      if(sc != SL_STATUS_OK){
          LOG_ERROR("BT STACK CONNECTION CLOSED");
      }

      connected = false;
      break;

    case sl_bt_evt_connection_parameters_id:
      /*
      LOG_INFO("Connection Parameters Updated:\n");
      LOG_INFO("  Interval: %.2f ms\n", evt->data.evt_connection_parameters.interval * 1.25f);
      LOG_INFO("  Latency : %u\n", evt->data.evt_connection_parameters.latency);
      LOG_INFO("  Timeout : %.0f ms\n", evt->data.evt_connection_parameters.timeout * 10.0f);
      */
    break;
      break;

    case sl_bt_evt_system_external_signal_id:
      //Do nothing. This is handled by temperature state machine
      break;

    case sl_bt_evt_gatt_server_characteristic_status_id:
      //client config changed
      if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_client_config) {
          //TODO Handle Indication Enable tracking
          uint16_t client_config_flags = evt->data.evt_gatt_server_characteristic_status.client_config_flags;

               if(client_config_flags == sl_bt_gatt_server_disable)                      {indication_enabled = false;}
          else if(client_config_flags == sl_bt_gatt_server_indication)                   {indication_enabled = true;}
          else if(client_config_flags == sl_bt_gatt_server_notification)                 {indication_enabled = false;}
          else if(client_config_flags == sl_bt_gatt_server_notification_and_indication)  {indication_enabled = true;}

      //confirmation received
      } else if (evt->data.evt_gatt_server_characteristic_status.status_flags == sl_bt_gatt_server_confirmation) {
          indication_inflight = false;
      }
      break;

    case sl_bt_evt_gatt_server_indication_timeout_id:
      indication_inflight = false;
      //LOG_ERROR("Indication Timed Out");
      break;

    default: //do nothing
      break;
    }

  (void) sc;
}

void send_temperature_reading(size_t value_len, const uint8_t* value){
  indication_inflight = true;
  sl_status_t sc = sl_bt_gatt_server_send_indication(connection_handle, gattdb_temperature_measurement, value_len, value);

  if(sc != SL_STATUS_OK){
      LOG_ERROR("BT STACK INDICATION SEND");
  }
}

void update_temperature_reading(size_t value_len, const uint8_t* value){
  sl_status_t sc = sl_bt_gatt_server_write_attribute_value(gattdb_temperature_measurement, 0, value_len, value);

  if(sc != SL_STATUS_OK){
      LOG_ERROR("BT STACK GATTDB UPDATE");
  }
}
