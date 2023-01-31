/*

  modbus.h - a lightweight ModBus implementation

  Part of grblHAL

  Copyright (c) 2020-2021 Terje Io

  Grbl is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  Grbl is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with Grbl.  If not, see <http://www.gnu.org/licenses/>.

*/

#ifndef _MODBUS_H_
#define _MODBUS_H_

#ifndef MODBUS_MAX_ADU_SIZE
#define MODBUS_MAX_ADU_SIZE 10
#endif
#ifndef MODBUS_QUEUE_LENGTH
#define MODBUS_QUEUE_LENGTH 8
#endif

#include <stdint.h>

typedef enum {
    ModBus_Idle,
    ModBus_Silent,
    ModBus_TX,
    ModBus_AwaitReply,
    ModBus_Timeout,
    ModBus_GotReply,
    ModBus_Exception
} modbus_state_t;

typedef enum {
    ModBus_ReadCoils = 1,
    ModBus_ReadDiscreteInputs = 2,
    ModBus_ReadHoldingRegisters = 3,
    ModBus_ReadInputRegisters = 4,
    ModBus_WriteCoil = 5,
    ModBus_WriteRegister = 6,
    ModBus_ReadExceptionStatus = 7,
    ModBus_Diagnostics = 8,
    ModBus_WriteCoils = 15,
    ModBus_WriteRegisters = 16

} modbus_function_t;

typedef struct {
    void *context;
    bool crc_check;
    uint8_t tx_length;
    uint8_t rx_length;
    char adu[MODBUS_MAX_ADU_SIZE];
} modbus_message_t;

typedef struct {
    void (*on_rx_packet)(modbus_message_t *msg);
    void (*on_rx_exception)(uint8_t code, void *context);
} modbus_callbacks_t;

typedef union {
    uint16_t timeout[6];
    struct {
        uint16_t b2400;
        uint16_t b4800;
        uint16_t b9600;
        uint16_t b19200;
        uint16_t b38400;
        uint16_t b115200;
    };
} modbus_silence_timeout_t;

typedef void (*stream_set_direction_ptr)(bool tx);

typedef struct {
    set_baud_rate_ptr set_baud_rate;
    stream_set_direction_ptr set_direction; // NULL if auto direction
    get_stream_buffer_count_ptr get_tx_buffer_count;
    get_stream_buffer_count_ptr get_rx_buffer_count;
    stream_write_n_ptr write;
    stream_read_ptr read;
    flush_stream_buffer_ptr flush_tx_buffer;
    flush_stream_buffer_ptr flush_rx_buffer;
} modbus_stream_t;

void modbus_init (void);
bool modbus_isup (void);
bool modbus_enabled (void);
void modbus_set_silence (const modbus_silence_timeout_t *timeout);
bool modbus_send (modbus_message_t *msg, const modbus_callbacks_t *callbacks, bool block);
modbus_state_t modbus_get_state (void);

#endif
