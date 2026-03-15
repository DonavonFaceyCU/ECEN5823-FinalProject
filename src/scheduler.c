/*
 * scheduler.c
 *
 *  Created on: Feb 1, 2026
 *      Author: donav
 */

#include "scheduler.h"

///////////////////////////// State Machines are Below ///////////////////////////////////

typedef enum discovery_state_t {
  S0_STARTUP = 0,
  S1_DISCOVERING,
  S2_CONNECTING,
  S3_CHECK_HTM_SERVICES,
  S4_CHECK_BUTTONSTATE_SERVICES,
  S5_CHECK_HTM_CHARACTERISTICS,
  S6_CHECK_BUTTONSTATE_CHARACTERISTICS,
  S7_ENABLE_HTM_INDICATIONS,
  S8_ENABLE_BUTTONSTATE_INDICATIONS,
  S9_RX_INDICATIONS,
  DISCOVERY_NUM_STATES
} discovery_state_t;

static uint8_t discoveryHandle;

static uint8_t HTM_service_UUID[] = {0x09, 0x18};
static uint32_t HTM_service_handle;

static uint8_t TempMeas_characteristic_UUID[] = {0x1C, 0x2A};
static uint32_t TempMeas_characteristic_handle;

static uint8_t buttonState_service_UUID[] = {0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x01, 0x00, 0x00, 0x00};
static uint32_t buttonState_service_handle;

static uint8_t buttonState_characteristic_UUID[] = {0x89, 0x62, 0x13, 0x2d, 0x2a, 0x65, 0xec, 0x87, 0x3e, 0x43, 0xc8, 0x38, 0x02, 0x00, 0x00, 0x00};
static uint32_t buttonState_characteristic_handle;

void discovery_stateMachine(sl_bt_msg_t *evt){
  static discovery_state_t current_state;
  discovery_state_t next_state = current_state;

  uint32_t event = SL_BT_MSG_ID(evt->header);

  if(event == sl_bt_evt_connection_closed_id){
      sl_bt_connection_close(discoveryHandle);
      sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m, sl_bt_scanner_discover_observation);
      displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
      displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
      displayPrintf(DISPLAY_ROW_BTADDR2, "");
      next_state = S1_DISCOVERING;
      return;
  }

  if(event == sl_bt_evt_gatt_characteristic_value_id){
      sl_bt_gatt_send_characteristic_confirmation(discoveryHandle);
  }

  sl_bt_evt_scanner_scan_report_t* report = NULL;
  bd_addr server_address = SERVER_BT_ADDRESS;

  displayPrintf(DISPLAY_ROW_10, "S%u", current_state);

  switch(current_state){

    case S0_STARTUP:
      if(event == sl_bt_evt_system_boot_id){
          displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
          next_state = S1_DISCOVERING;
      }
      break;

    case S1_DISCOVERING:
      //if scanner_scan_report_id matches our server, move state
      //call open command
      if(event == sl_bt_evt_scanner_scan_report_id){
          report = &evt->data.evt_scanner_scan_report;
          if(memcmp(&(report->address.addr), &server_address, sizeof(bd_addr)) == 0){
              sl_bt_scanner_stop();
              sl_bt_connection_open(report->address, report->address_type, sl_bt_gap_phy_1m, &discoveryHandle);
              //timerWaitUs(1000000); //1 second timeout on connecting
              next_state = S2_CONNECTING;
          }
      }
      break;

    case S2_CONNECTING:
      //if connection opened, call discover primary services, move state
      if(event == sl_bt_evt_connection_opened_id){
          sl_bt_gatt_discover_primary_services_by_uuid(discoveryHandle, sizeof(HTM_service_UUID), HTM_service_UUID);

          bd_addr* server_address = &evt->data.evt_connection_opened.address;

          displayPrintf(DISPLAY_ROW_CONNECTION, "Connected");
          displayPrintf(DISPLAY_ROW_BTADDR2, "%x:%x:%x:%x:%x:%x",
                    server_address->addr[0],
                    server_address->addr[1],
                    server_address->addr[2],
                    server_address->addr[3],
                    server_address->addr[4],
                    server_address->addr[5]);

          next_state = S3_CHECK_HTM_SERVICES;
      }
      //timeout while connecting
      if(event == sl_bt_evt_system_external_signal_id && Scheduler_Active_COMP1(evt)){
          sl_bt_connection_close(discoveryHandle);
          sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m, sl_bt_scanner_discover_observation);
          displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
          next_state = S1_DISCOVERING;
      }
      break;

    case S3_CHECK_HTM_SERVICES:
      //Wait for procedure to complete
      //if procedure complete, call discover characteristics, move state
      if(event == sl_bt_evt_gatt_procedure_completed_id){
          sl_bt_gatt_discover_primary_services_by_uuid(discoveryHandle, sizeof(buttonState_service_UUID), buttonState_service_UUID);
          next_state = S4_CHECK_BUTTONSTATE_SERVICES;
      }
      //service data received
      if(event == sl_bt_evt_gatt_service_id){
          HTM_service_handle = evt->data.evt_gatt_service.service;
      }
      break;

    case S4_CHECK_BUTTONSTATE_SERVICES:
      //Wait for procedure to complete
      //if procedure complete, call discover characteristics, move state
      if(event == sl_bt_evt_gatt_procedure_completed_id){
          sl_bt_gatt_discover_characteristics_by_uuid(discoveryHandle, HTM_service_handle, sizeof(TempMeas_characteristic_UUID), TempMeas_characteristic_UUID);
          next_state = S5_CHECK_HTM_CHARACTERISTICS;
      }
      //service data received
      if(event == sl_bt_evt_gatt_service_id){
          buttonState_service_handle = evt->data.evt_gatt_service.service;
      }
      break;

    case S5_CHECK_HTM_CHARACTERISTICS:
      //Wait for procedure to complete
      //if procedure complete, enable indications, move state
      //Update Display to say Handling Indications
      if(event == sl_bt_evt_gatt_procedure_completed_id){
          sl_bt_gatt_discover_characteristics_by_uuid(discoveryHandle, buttonState_service_handle, sizeof(buttonState_characteristic_UUID), buttonState_characteristic_UUID);
          next_state = S6_CHECK_BUTTONSTATE_CHARACTERISTICS;
      }
      //characteristic data received
      if(event == sl_bt_evt_gatt_characteristic_id){
          TempMeas_characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
      }
      break;

    case S6_CHECK_BUTTONSTATE_CHARACTERISTICS:
      //Wait for procedure to complete
      //if procedure complete, enable indications, move state
      //Update Display to say Handling Indications
      if(event == sl_bt_evt_gatt_procedure_completed_id){
          sl_bt_gatt_set_characteristic_notification(discoveryHandle, TempMeas_characteristic_handle, sl_bt_gatt_indication);
          //timerWaitUs(8000000); //8 second timeout to start receiving indications
          next_state = S7_ENABLE_HTM_INDICATIONS;
      }
      //characteristic data received
      if(event == sl_bt_evt_gatt_characteristic_id){
          buttonState_characteristic_handle = evt->data.evt_gatt_characteristic.characteristic;
      }
      break;

    case S7_ENABLE_HTM_INDICATIONS:
      //Wait for procedure to complete
      if(event == sl_bt_evt_gatt_procedure_completed_id){
          sl_bt_gatt_set_characteristic_notification(discoveryHandle, buttonState_characteristic_handle, sl_bt_gatt_indication);
          next_state = S8_ENABLE_BUTTONSTATE_INDICATIONS;
      }
    break;

    case S8_ENABLE_BUTTONSTATE_INDICATIONS:
      //Wait for procedure to complete
      if(event == sl_bt_evt_gatt_procedure_completed_id){
          displayPrintf(DISPLAY_ROW_CONNECTION, "Handling Indications");
          next_state = S9_RX_INDICATIONS;
      }
    break;

    case S9_RX_INDICATIONS:
      //Handle Indications received
      //Check indications are for HTM service / temperature characteristic
      //Update Display
      if(event == sl_bt_evt_gatt_characteristic_value_id){
          if(evt->data.evt_gatt_characteristic_value.characteristic != TempMeas_characteristic_handle){
              return;
          }

          uint8_t* value = evt->data.evt_gatt_characteristic_value.value.data;

          int32_t temperature = FLOAT_TO_INT32(value);
          uint16_t temp_wholeDigits = (temperature & 0xFFFF0000) >> 16;
          uint16_t temp_tenthDigits = (temperature & 0x0000FFFF) >> 0;

          displayPrintf(DISPLAY_ROW_TEMPVALUE, "Temp = %i.%i Celsius", temp_wholeDigits, temp_tenthDigits);
          timerWaitUs(4000000); //4 second timeout reset on receiving indications
      }
      //timeout while receiving indications
      if(event == sl_bt_evt_system_external_signal_id && Scheduler_Active_COMP1(evt)){
          sl_bt_connection_close(discoveryHandle);
          sl_bt_scanner_start(sl_bt_scanner_scan_phy_1m, sl_bt_scanner_discover_observation);
          displayPrintf(DISPLAY_ROW_CONNECTION, "Discovering");
          displayPrintf(DISPLAY_ROW_TEMPVALUE, "");
          displayPrintf(DISPLAY_ROW_BTADDR2, "");
          next_state = S1_DISCOVERING;
      }
      break;
    default:
      break;
  }

  //Not yet handled
  //sl_bt_evt_gatt_service_id:
  //sl_bt_evt_gatt_characteristic_id:

  current_state = next_state;
}

typedef enum i2c_transfer_state_t {
  S0_IDLE = 0,
  S1_STARTUP,
  S2_COMMAND,
  S3_MEASUREMENT,
  S4_PUBLISH,
  I2C_TRANSFER_NUM_STATES
} i2c_transfer_state_t;

void temperature_stateMachine(sl_bt_msg_t *evt){
  static i2c_transfer_state_t current_state;

  //ignore all events aside from external signals
  if(SL_BT_MSG_ID(evt->header) != sl_bt_evt_system_external_signal_id){
      return;
  }

    i2c_transfer_state_t next_state = current_state;

    switch(current_state){
      case S0_IDLE:
        //if UF event is triggered, turn on sensor, wait 82ms
        if(Scheduler_Active_UF(evt) && connection_established() && HTM_indication_allowed()){
          next_state = S1_STARTUP;

          i2cEnableSensor();
          timerWaitUs(80000);
        }
        break;
      case S1_STARTUP:
        //if COMP1 is triggered after 80ms, command a temperature reading to happen
        if(Scheduler_Active_COMP1(evt)){
          next_state = S2_COMMAND;

          i2cCommandTemperatureReading();
          NVIC_EnableIRQ(I2C0_IRQn);
        }
        break;
      case S2_COMMAND:
        //if TXC is triggered, wait 11ms for temperature to be read
        if(Scheduler_Active_TXC()){
          next_state = S3_MEASUREMENT;

          NVIC_DisableIRQ(I2C0_IRQn);
          timerWaitUs(10800);
        }
        break;
      case S3_MEASUREMENT:
        //if COMP1 is triggered, start temperature reading
        if(Scheduler_Active_COMP1(evt)){
          next_state = S4_PUBLISH;

          i2cReadTemperature_nonblocking_start();
          NVIC_EnableIRQ(I2C0_IRQn);
        }
        break;
      case S4_PUBLISH:
        //if TXC is triggered, shutdown sensor, go back to idle
        if(Scheduler_Active_TXC(evt)){
          next_state = S0_IDLE;

          NVIC_DisableIRQ(I2C0_IRQn);
          uint32_t temp_11073 = i2cReadTemperature_nonblocking_finish();

          uint8_t htmTempBuffer[5];

          uint8_t *p = htmTempBuffer;

          uint8_t flags = 0x00;
          UINT8_TO_BITSTREAM(p, flags);
          UINT32_TO_BITSTREAM(p, temp_11073);

          update_temperature_reading(5, htmTempBuffer);

          //i2cDisableSensor();
        }
        break;
      default:
        next_state = S0_IDLE;
        break;
    }

    current_state = next_state;
}

///////////////////////////// Event Generic Wrappers are Below ///////////////////////////////////

static uint32_t Scheduler_Active(sl_bt_msg_t *evt, uint32_t mask);
static uint32_t Scheduler_Active(sl_bt_msg_t *evt, uint32_t mask){
  return evt->data.evt_system_external_signal.extsignals & mask;
}

static void Scheduler_Set(uint32_t mask);
static void Scheduler_Set(uint32_t mask){
  CORE_ATOMIC_SECTION
  (
      sl_bt_external_signal(mask);
  )
}

///////////////////////////// Event Specific Wrappers are Below ///////////////////////////////////

#define EVENT_UF (0x1 << 0)
uint32_t Scheduler_Active_UF(sl_bt_msg_t *evt)      { return Scheduler_Active(evt, EVENT_UF);     }
void Scheduler_Set_UF()                             { Scheduler_Set(EVENT_UF);                    }

#define EVENT_COMP1 (0x1 << 1)
uint32_t Scheduler_Active_COMP1(sl_bt_msg_t *evt)   { return Scheduler_Active(evt, EVENT_COMP1);  }
void Scheduler_Set_COMP1()                          { Scheduler_Set(EVENT_COMP1);                 }

#define EVENT_TXC (0x1 << 2)
uint32_t Scheduler_Active_TXC(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_TXC);    }
void Scheduler_Set_TXC()                            { Scheduler_Set(EVENT_TXC);                   }

#define EVENT_PB0_pressed (0x1 << 3)
uint32_t Scheduler_Active_PB0_pressed(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_PB0_pressed);    }
void Scheduler_Set_PB0_pressed()                            { Scheduler_Set(EVENT_PB0_pressed);                   }

#define EVENT_PB0_released (0x1 << 4)
uint32_t Scheduler_Active_PB0_released(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_PB0_released);    }
void Scheduler_Set_PB0_released()                            { Scheduler_Set(EVENT_PB0_released);                   }

#define EVENT_ButtonState_ToggleIndication (0x1 << 5)
uint32_t Scheduler_Active_ButtonState_ToggleIndication(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_ButtonState_ToggleIndication);    }
void Scheduler_Set_ButtonState_ToggleIndication()                            { Scheduler_Set(EVENT_ButtonState_ToggleIndication);                   }

#define EVENT_PB0_ButtonState_Read (0x1 << 6)
uint32_t Scheduler_Active_ButtonState_Read(sl_bt_msg_t *evt)     { return Scheduler_Active(evt, EVENT_PB0_ButtonState_Read);    }
void Scheduler_Set_ButtonState_Read()                            { Scheduler_Set(EVENT_PB0_ButtonState_Read);                   }
