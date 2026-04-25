/*
 * ble.h
 *
 *  Created on: Feb 18, 2026
 *      Author: donav
 */

#ifndef SRC_BLE_H_
#define SRC_BLE_H_

#include "sl_bluetooth.h"
#include "gatt_db.h"
#include "ble_device_type.h"

#define UINT8_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); }

#define UINT32_TO_BITSTREAM(p, n) { *(p)++ = (uint8_t)(n); *(p)++ = (uint8_t)((n) >> 8); \
 *(p)++ = (uint8_t)((n) >> 16); *(p)++ = (uint8_t)((n) >> 24); }

#define INT32_TO_FLOAT(m, e) ( (int32_t) (((uint32_t) m) & 0x00FFFFFFU) | (((uint32_t) e) << 24) )

int32_t FLOAT_TO_INT32(const uint8_t *buffer_ptr);

void handle_ble_event(sl_bt_msg_t *evt);

void update_sensor_reading(uint16_t touch_value, uint8_t proximity_value);

bool HTM_indication_allowed();
bool connection_established();

//////////////////////////////////////////////////////////////////////////////////////////////////
///                                                                                            ///
///       Circular Buffer Code below (taken from assignment 0.5)                               ///
///                                                                                            ///
//////////////////////////////////////////////////////////////////////////////////////////////////

#include <stdint.h>    // for uint8_t etc.
#include <stdbool.h>   // for bool

#define QUEUE_DEPTH      (16)
#define MAX_BUFFER_LENGTH  (5)
#define MIN_BUFFER_LENGTH  (1)

typedef struct {
  uint16_t       charHandle;                 // GATT DB handle from gatt_db.h
  uint32_t       bufLength;                  // Number of bytes written to field buffer[5]
  uint8_t        buffer[MAX_BUFFER_LENGTH];  // The actual data buffer for the indication
} queue_struct_t;

void     reset_queue      (void);
bool     write_queue      (uint16_t  charHandle, uint32_t  bufLength, uint8_t *buffer);
bool     read_queue       (uint16_t *charHandle, uint32_t *bufLength, uint8_t *buffer);
void     get_queue_status (uint32_t *wptr, uint32_t *rptr, bool *full, bool *empty);
uint32_t get_queue_depth  (void);

// BLE Data Structure, save all of our private BT data in here.
// Modern C (circa 2021 does it this way)
// typedef ble_data_struct_t is referred to as an anonymous struct definition
typedef struct {
 // values that are common to servers and clients
 bd_addr myAddress;
 // values unique for server
 // The advertising set handle allocated from Bluetooth stack.
 uint8_t advertisingSetHandle;
 // values unique for client
 uint8_t discoverySetHandle;

 bool connected;
 bool bonded;
 bool indication_inflight;

 bool HTM_indication_enabled;
 bool Button_indication_enabled;

 queue_struct_t   indication_queue[QUEUE_DEPTH]; // the queue
 uint32_t         wptr;              // write pointer
 uint32_t         rptr;              // read pointer
} ble_data_struct_t;

ble_data_struct_t* get_ble_data();

#endif /* SRC_BLE_H_ */
