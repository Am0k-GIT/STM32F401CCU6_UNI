/*

  modbus.c - a lightweight ModBus implementation

  Part of grblHAL

  Copyright (c) 2020-2022 Terje Io
  except modbus_CRC16x which is Copyright (c) 2006 Christian Walter <wolti@sil.at>
  Lifted from his FreeModbus Libary

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

#ifdef ARDUINO
#include "../driver.h"
#else
#include "driver.h"
#endif

#if MODBUS_ENABLE

#include <string.h>

#ifdef ARDUINO
#include "../grbl/protocol.h"
#include "../grbl/settings.h"
#include "../grbl/nvs_buffer.h"
#include "../grbl/state_machine.h"
#else
#include "grbl/protocol.h"
#include "grbl/settings.h"
#include "grbl/nvs_buffer.h"
#include "grbl/state_machine.h"
#endif

#include "modbus.h"

#ifndef MODBUS_BAUDRATE
#define MODBUS_BAUDRATE 3 // 19200
#endif
#ifndef MODBUS_SERIAL_PORT
#define MODBUS_SERIAL_PORT -1
#endif
#ifndef MODBUS_DIR_AUX
#define MODBUS_DIR_AUX    -1
#endif

typedef struct queue_entry {
    bool async;
    bool sent;
    modbus_message_t msg;
    modbus_callbacks_t callbacks;
    struct queue_entry *next;
} queue_entry_t;

static const uint32_t baud[] = { 2400, 4800, 9600, 19200, 38400, 115200 };
static const modbus_silence_timeout_t dflt_timeout =
{
    .b2400   = 16,
    .b4800   = 8,
    .b9600   = 4,
    .b19200  = 2,
    .b38400  = 2,
    .b115200 = 2
};

static modbus_stream_t stream;
static uint32_t rx_timeout = 0, silence_until = 0, silence_timeout;
static int16_t exception_code = 0;
static modbus_silence_timeout_t silence;
static queue_entry_t queue[MODBUS_QUEUE_LENGTH];
static modbus_settings_t modbus;
static volatile bool spin_lock = false, is_up = false;
static volatile queue_entry_t *tail, *head, *packet = NULL;
static volatile modbus_state_t state = ModBus_Idle;
#if MODBUS_ENABLE == 2
static uint8_t dir_port;
#endif

static driver_reset_ptr driver_reset;
static on_execute_realtime_ptr on_execute_realtime, on_execute_delay;
static on_report_options_ptr on_report_options;
static nvs_address_t nvs_address;

static status_code_t modbus_set_baud (setting_id_t id, uint_fast16_t value);
static uint32_t modbus_get_baud (setting_id_t setting);
static void modbus_settings_restore (void);
static void modbus_settings_load (void);

// Compute the MODBUS RTU CRC
static uint16_t modbus_CRC16x (const char *buf, uint_fast16_t len)
{
    uint16_t crc = 0xFFFF;
    uint_fast8_t pos, i;

    for (pos = 0; pos < len; pos++) {
        crc ^= (uint16_t)buf[pos];          // XOR byte into least sig. byte of crc
        for (i = 8; i != 0; i--) {          // Loop over each bit
            if ((crc & 0x0001) != 0) {      // If the LSB is set
                crc >>= 1;                  // Shift right and XOR 0xA001
                crc ^= 0xA001;
            } else                          // Else LSB is not set
                crc >>= 1;                  // Just shift right
        }
    }

    // Note, this number has low and high bytes swapped, so use it accordingly (or swap bytes)
    return crc;
}
/*
static bool valid_crc (const char *buf, uint_fast16_t len)
{
    uint16_t crc = modbus_CRC16x(buf, len - 2);

    return buf[len - 1] == (crc >> 8) && buf[len - 2] == (crc & 0xFF);
}
*/

static inline void add_message (queue_entry_t *packet, modbus_message_t *msg, const modbus_callbacks_t *callbacks)
{
    packet->sent = false;
    memcpy(&packet->msg, msg, sizeof(modbus_message_t));
    if(callbacks)
        memcpy(&packet->callbacks, callbacks, sizeof(modbus_callbacks_t));
    else {
        packet->callbacks.on_rx_packet = NULL;
        packet->callbacks.on_rx_exception = NULL;
    }
}

static void modbus_poll (void)
{
    static uint32_t last_ms;

    uint32_t ms = hal.get_elapsed_ticks();

    if(ms == last_ms || spin_lock) // check once every ms
        return;

    spin_lock = true;

    switch(state) {

        case ModBus_Idle:
            if(tail != head && !packet) {

                packet = tail;
                tail = tail->next;
                state = ModBus_TX;
                rx_timeout = modbus.rx_timeout;

                if(stream.set_direction)
                    stream.set_direction(true);

                packet->sent = true;
                stream.flush_rx_buffer();
                stream.write(((queue_entry_t *)packet)->msg.adu, ((queue_entry_t *)packet)->msg.tx_length);
            }
            break;

        case ModBus_Silent:
            if(ms >= silence_until) {
                silence_until = 0;
                state = ModBus_Idle;
            }
            break;

        case ModBus_TX:
            if(!stream.get_tx_buffer_count()) {

                // When an auto-direction sense circuit supports higher baudrates is used at slower rates, it can switch during the off time (TXD is high) of some bit sequences.
                // In some cases (teensy4.1) this can result in garbage characters in the RX buffer after a message is transmitted.
                // Flushing the buffer prevents these characters from appearing as an RX message.
                // Since Modbus is half-duplex, there should never be valid data recived during a message transmit.
                stream.flush_rx_buffer();
                
                state = ModBus_AwaitReply;

                if(stream.set_direction)
                    stream.set_direction(false);
            }
            break;

        case ModBus_AwaitReply:
            if(rx_timeout && --rx_timeout == 0) {
                if(packet->async) {
                    state = ModBus_Silent;
                    packet = NULL;
                } else if(stream.read() == 1 && (stream.read() & 0x80)) {
                    exception_code = stream.read();
                    state = ModBus_Exception;
                } else
                    state = ModBus_Timeout;
                spin_lock = false;
                if(state != ModBus_AwaitReply)
                    silence_until = ms + silence_timeout;
                return;
            }

            if(stream.get_rx_buffer_count() >= packet->msg.rx_length) {

                char *buf = ((queue_entry_t *)packet)->msg.adu;

                uint16_t rx_len = packet->msg.rx_length;        // store original length for CRC check

                do {
                    *buf++ = stream.read();
                } while(--packet->msg.rx_length);

                if (packet->msg.crc_check) {
                    uint_fast16_t crc = modbus_CRC16x(((queue_entry_t *)packet)->msg.adu, rx_len - 2);

                    if (packet->msg.adu[rx_len-2] != (crc & 0xFF) || packet->msg.adu[rx_len-1] != (crc >>8)) {
                        // CRC check error
                        if((state = packet->async ? ModBus_Silent : ModBus_Exception) == ModBus_Silent) {
                            if(packet->callbacks.on_rx_exception)
                                packet->callbacks.on_rx_exception(0, packet->msg.context);
                            packet = NULL;
                        }
                        silence_until = ms + silence_timeout;
                        break;
                    }

                }

                if((state = packet->async ? ModBus_Silent : ModBus_GotReply) == ModBus_Silent) {
                    if(packet->callbacks.on_rx_packet)
                        packet->callbacks.on_rx_packet(&((queue_entry_t *)packet)->msg);
                    packet = NULL;
                }

                silence_until = ms + silence_timeout;
            }
            break;

        case ModBus_Timeout:
            state = ModBus_Silent;
            silence_until = ms + silence_timeout;
            break;

        default:
            break;
    }

    last_ms = ms;
    spin_lock = false;
}

static void modbus_poll_realtime (sys_state_t grbl_state)
{
    on_execute_realtime(grbl_state);

    modbus_poll();
}

static void modbus_poll_delay (sys_state_t grbl_state)
{
    on_execute_delay(grbl_state);

    modbus_poll();
}

bool modbus_send (modbus_message_t *msg, const modbus_callbacks_t *callbacks, bool block)
{
    static queue_entry_t sync_msg = {0};

    uint_fast16_t crc = modbus_CRC16x(msg->adu, msg->tx_length - 2);

    msg->adu[msg->tx_length - 1] = crc >> 8;
    msg->adu[msg->tx_length - 2] = crc & 0xFF;

    while(spin_lock);

    if(block) {

        bool poll = true;

        do {
            grbl.on_execute_realtime(state_get());
        } while(state != ModBus_Idle);

        if(stream.set_direction)
            stream.set_direction(true);

        state = ModBus_TX;
        rx_timeout = modbus.rx_timeout;

        add_message(&sync_msg, msg, callbacks);

        sync_msg.async = false;
        stream.flush_rx_buffer();
        stream.write(sync_msg.msg.adu, sync_msg.msg.tx_length);

        packet = &sync_msg;

        while(poll) {

            grbl.on_execute_realtime(state_get());

            switch(state) {

                case ModBus_Timeout:
                    if(packet->callbacks.on_rx_exception)
                        packet->callbacks.on_rx_exception(0, packet->msg.context);
                    poll = false;
                    break;

                case ModBus_Exception:
                    if(packet->callbacks.on_rx_exception)
                        packet->callbacks.on_rx_exception(exception_code == -1 ? 0 : (uint8_t)(exception_code & 0xFF), packet->msg.context);
                    poll = false;
                    break;

                case ModBus_GotReply:
                    if(packet->callbacks.on_rx_packet)
                        packet->callbacks.on_rx_packet(&((queue_entry_t *)packet)->msg);
                    poll = block = false;
                    break;

                default:
                    break;
            }
        }
    
        packet = NULL;
        state = silence_until > 0 ? ModBus_Silent : ModBus_Idle;

    } else if(packet != &sync_msg) {
        if(head->next != tail) {
            add_message((queue_entry_t *)head, msg, callbacks);
            head->async = true;
            head = head->next;
        }
    }

    return !block;
}

modbus_state_t modbus_get_state (void)
{
    modbus_poll();

    return state;
}

static void modbus_reset (void)
{
    while(spin_lock);

    if(sys.abort) {

        packet = NULL;
        tail = head;

        silence_until = 0;
        state = ModBus_Idle;

        stream.flush_tx_buffer();
        stream.flush_rx_buffer();

    }

    while(state != ModBus_Idle)
        modbus_poll();

    driver_reset();
}

static uint32_t get_baudrate (uint32_t rate)
{
    uint32_t idx = sizeof(baud) / sizeof(uint32_t);

    do {
        if(baud[--idx] == rate)
            return idx;
    } while(idx);

    return MODBUS_BAUDRATE;
}

static const setting_group_detail_t modbus_groups [] = {
    { Group_Root, Group_ModBus, "ModBus"}
};

static const setting_detail_t modbus_settings[] = {
    { Settings_ModBus_BaudRate, Group_ModBus, "ModBus baud rate", NULL, Format_RadioButtons, "2400,4800,9600,19200,38400,115200", NULL, NULL, Setting_NonCoreFn, modbus_set_baud, modbus_get_baud, NULL },
    { Settings_ModBus_RXTimeout, Group_ModBus, "ModBus RX timeout", "milliseconds", Format_Integer, "####0", "50", "250", Setting_NonCore, &modbus.rx_timeout, NULL, NULL }
};

static void modbus_settings_save (void)
{
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&modbus, sizeof(modbus_settings_t), true);
}

static setting_details_t setting_details = {
    .groups = modbus_groups,
    .n_groups = sizeof(modbus_groups) / sizeof(setting_group_detail_t),
    .settings = modbus_settings,
    .n_settings = sizeof(modbus_settings) / sizeof(setting_detail_t),
    .save = modbus_settings_save,
    .load = modbus_settings_load,
    .restore = modbus_settings_restore
};

static status_code_t modbus_set_baud (setting_id_t id, uint_fast16_t value)
{
    modbus.baud_rate = baud[(uint32_t)value];
    silence_timeout = silence.timeout[(uint32_t)value];
    stream.set_baud_rate(modbus.baud_rate);

    return Status_OK;
}

static uint32_t modbus_get_baud (setting_id_t setting)
{
    return get_baudrate(modbus.baud_rate);
}

static void modbus_settings_restore (void)
{
    modbus.rx_timeout = 50;
    modbus.baud_rate = baud[MODBUS_BAUDRATE];

    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&modbus, sizeof(modbus_settings_t), true);
}

static void modbus_settings_load (void)
{
    if(hal.nvs.memcpy_from_nvs((uint8_t *)&modbus, nvs_address, sizeof(modbus_settings_t), true) != NVS_TransferResult_OK)
        modbus_settings_restore();

    is_up = true;
    silence_timeout = silence.timeout[get_baudrate(modbus.baud_rate)];

    stream.set_baud_rate(modbus.baud_rate);
}

static void onReportOptions (bool newopt)
{
    on_report_options(newopt);

    if(!newopt)
        hal.stream.write("[PLUGIN:MODBUS v0.14]" ASCII_EOL);
}

bool modbus_isup (void)
{
    return is_up;
}

bool modbus_enabled (void)
{
    return nvs_address != 0;
}

void modbus_set_silence (const modbus_silence_timeout_t *timeout)
{
    if(timeout)
        memcpy(&silence, timeout, sizeof(modbus_silence_timeout_t));
    else
        memcpy(&silence, &dflt_timeout, sizeof(modbus_silence_timeout_t));

    silence_timeout = silence.timeout[get_baudrate(modbus.baud_rate)];
}

static bool stream_is_valid (const io_stream_t *stream)
{
    return stream &&
            !(stream->set_baud_rate == NULL ||
               stream->get_tx_buffer_count == NULL ||
                stream->get_rx_buffer_count == NULL ||
                 stream->write_n == NULL ||
                  stream->read == NULL ||
                   stream->reset_write_buffer == NULL ||
                    stream->reset_read_buffer == NULL ||
                     stream->set_enqueue_rt_handler == NULL);
}

static void pos_failed (uint_fast16_t state)
{
    report_message("Modbus failed to initialize!", Message_Warning);
}

#if MODBUS_ENABLE == 2
static void modbus_set_direction (bool tx)
{
    hal.port.digital_out(dir_port, tx);
}
#endif

static bool claim_stream (io_stream_properties_t const *sstream)
{
    io_stream_t const *claimed = NULL;

#if MODBUS_SERIAL_PORT >= 0
    if(sstream->type == StreamType_Serial && sstream->instance == MODBUS_SERIAL_PORT) {
#else
    if(sstream->type == StreamType_Serial && sstream->flags.modbus_ready && !sstream->flags.claimed) {
#endif
        if((claimed = sstream->claim(baud[MODBUS_BAUDRATE])) && stream_is_valid(claimed)) {

            claimed->set_enqueue_rt_handler(stream_buffer_all);

            stream.set_baud_rate = claimed->set_baud_rate;
            stream.get_tx_buffer_count = claimed->get_tx_buffer_count;
            stream.get_rx_buffer_count = claimed->get_rx_buffer_count;
            stream.write = claimed->write_n;
            stream.read = claimed->read;
            stream.flush_tx_buffer = claimed->reset_write_buffer;
            stream.flush_rx_buffer = claimed->reset_read_buffer;
#if MODBUS_ENABLE == 2
            stream.set_direction = modbus_set_direction;
#endif
            if(hal.periph_port.set_pin_description) {
                hal.periph_port.set_pin_description(Output_TX, (pin_group_t)(PinGroup_UART + claimed->instance), "Modbus");
                hal.periph_port.set_pin_description(Input_RX, (pin_group_t)(PinGroup_UART + claimed->instance), "Modbus");
            }
        } else
            claimed = NULL;
    }

    return claimed != NULL;
}

void modbus_init (void)
{

#if MODBUS_ENABLE == 2

    uint8_t n_out = ioports_available(Port_Digital, Port_Output);

  #if MODBUS_DIR_AUX >= 0
    dir_port = MODBUS_DIR_AUX;
  #else
    dir_port = n_out - 1;
  #endif

    if(!(n_out > dir_port && ioport_claim(Port_Digital, Port_Output, &dir_port, "Modbus RX/TX direction"))) {
        protocol_enqueue_rt_command(pos_failed);
        system_raise_alarm(Alarm_SelftestFailed);
        return;
    }

#endif

    if(stream_enumerate_streams(claim_stream) && (nvs_address = nvs_alloc(sizeof(modbus_settings_t)))) {

        driver_reset = hal.driver_reset;
        hal.driver_reset = modbus_reset;

        on_execute_realtime = grbl.on_execute_realtime;
        grbl.on_execute_realtime = modbus_poll_realtime;

        on_execute_delay = grbl.on_execute_delay;
        grbl.on_execute_delay = modbus_poll_delay;

        on_report_options = grbl.on_report_options;
        grbl.on_report_options = onReportOptions;

        //TODO: subscribe to grbl.on_reset event to terminate polling?

        settings_register(&setting_details);

        head = tail = &queue[0];

        uint_fast8_t idx;
        for(idx = 0; idx < MODBUS_QUEUE_LENGTH; idx++)
            queue[idx].next = idx == MODBUS_QUEUE_LENGTH - 1 ? &queue[0] : &queue[idx + 1];

        modbus_set_silence(NULL);

    } else {
        protocol_enqueue_rt_command(pos_failed);
        system_raise_alarm(Alarm_SelftestFailed);
    }
}

#endif
