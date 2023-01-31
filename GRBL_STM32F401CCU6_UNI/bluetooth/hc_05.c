/*

  hc_05.c - HC-05 Bluetooth module interface plugin

  Part of grblHAL

  Copyright (c) 2021-2022 Terje Io

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

#include <string.h>

#ifdef ARDUINO
#include "../grbl/hal.h"
#include "../grbl/protocol.h"
#include "../grbl/state_machine.h"
#include "../grbl/nvs_buffer.h"
#else
#include "grbl/hal.h"
#include "grbl/protocol.h"
#include "grbl/state_machine.h"
#include "grbl/nvs_buffer.h"
#endif

typedef union {
    uint8_t value;
    struct {
        uint8_t enable : 1;
    };
} hc05_options_t;

typedef struct {
    hc05_options_t options;
    uint32_t baud_rate;
    uint8_t state_port;
    char device_name[33];
} hc05_settings_t;

static uint8_t state_port;
static uint32_t timeout;
static on_report_options_ptr on_report_options;
static nvs_address_t nvs_address;
static io_stream_t bt_stream;
static hc05_settings_t hc05_settings;

static void hc05_settings_restore (void);
static void hc05_settings_load (void);
static void hc05_settings_save (void);
static void select_stream (sys_state_t state);
static uint8_t n_ports;
static char max_port[4];

static void on_connect (uint8_t port, bool state)
{
    if((bt_stream.state.connected = state))
        select_stream(state_get());
    else if(hal.stream.type == StreamType_Bluetooth)
        stream_disconnect(&bt_stream);
}

static void connected (sys_state_t state)
{
    report_init_message();
}

void select_stream (sys_state_t state)
{
    if(hc05_settings.options.enable) {

        bt_stream.set_baud_rate(115200);

        if(bt_stream.write != hal.stream.write) {
            if(hal.stream.disable_rx)
                hal.stream.disable_rx(true);
            stream_connect(&bt_stream);
        }
    }

    protocol_enqueue_rt_command(connected);
}

static bool send_command (char *command)
{
    int16_t c;
    struct {
        uint_fast8_t idx;
        char buffer[50];
    } response;

    memset(&response, 0, sizeof(response));

    bt_stream.reset_read_buffer();
    bt_stream.write(command);

    timeout = hal.get_elapsed_ticks() + 1000;

    while(hal.get_elapsed_ticks() <= timeout) {

        if((c = bt_stream.read()) != SERIAL_NO_DATA) {

            if(c == ASCII_LF)
                continue;

            if(c == ASCII_CR || response.idx >= sizeof(response.buffer))
                break;

            response.buffer[response.idx++] = c;
        }
    }

    return !strcmp(response.buffer, "OK");
}

static void auto_config (sys_state_t state)
{
    bool ok = false;
    char devname[45];
    io_stream_t active_stream;
    enqueue_realtime_command_ptr prev_handler;

    if(hal.stream.write != bt_stream.write)
        hal.stream.write("Attempting to configure HC-05 module..." ASCII_EOL);

    memcpy(&active_stream, &hal.stream, sizeof(io_stream_t));               // Save current stream pointers,
    prev_handler = hal.stream.set_enqueue_rt_handler(stream_buffer_all);    // stop core real-time command handling.

    bt_stream.set_baud_rate(38400);
    bt_stream.set_enqueue_rt_handler(stream_buffer_all);

    if(send_command("AT" ASCII_EOL)) {
        strcpy(devname, "AT+NAME=");
        strcat(devname, hc05_settings.device_name);
        strcat(devname, ASCII_EOL);
        ok = send_command("AT+UART=115200,1,0" ASCII_EOL);
        ok = ok && send_command(devname);
        ok = ok && send_command("AT+POLAR=1,1" ASCII_EOL);
    }

    memcpy(&hal.stream, &active_stream, sizeof(io_stream_t));   // Restore current stream pointers.
    hal.stream.set_enqueue_rt_handler(prev_handler);

    bt_stream.set_baud_rate(115200);

    if(ok) {
        hc05_settings.options.enable = true;
        hc05_settings_save();
        if(hal.stream.write != bt_stream.write)
            hal.stream.write("HC-05 configuration successful!" ASCII_EOL);
    }
    else if(hal.stream.write != bt_stream.write)
        hal.stream.write("HC-05 configuration failed, is the module set to AT mode?" ASCII_EOL);
}

static void hc05_setup (sys_state_t state)
{
    bool is_connected = (hal.port.wait_on_input(true, state_port, WaitMode_Immediate, 0.0f) == 1);

    if(!hc05_settings.options.enable && !is_connected)
        protocol_enqueue_rt_command(auto_config);
    else {
        hal.port.register_interrupt_handler(state_port, IRQ_Mode_Change, on_connect);
        if(is_connected)
            protocol_enqueue_rt_command(select_stream);
    }
}

static status_code_t set_options (setting_id_t id, uint_fast16_t int_value)
{
    hc05_options_t opt;

    opt.value = int_value;

    if(hc05_settings.options.enable != opt.enable && opt.enable)
        hal.port.register_interrupt_handler(state_port, IRQ_Mode_Change, on_connect);

    hc05_settings.options.value = opt.value;

    return Status_OK;
}

static uint32_t get_options (setting_id_t id)
{
    return hc05_settings.options.value;
}

static bool is_setting_available (const setting_detail_t *setting)
{
    return setting->id == Setting_BlueToothStateInput && ioport_can_claim_explicit();
}

static const setting_group_detail_t bluetooth_groups [] = {
    { Group_Root, Group_Bluetooth, "Bluetooth"}
};

static const setting_detail_t bluetooth_settings[] = {
    { Setting_BlueToothInitOK, Group_Bluetooth, "HC-05 init ok", NULL, Format_Bool, NULL, NULL, NULL, Setting_NonCoreFn, set_options, get_options, NULL },
    { Setting_BlueToothDeviceName, Group_Bluetooth, "Bluetooth device name", NULL, Format_String, "x(32)", NULL, "32", Setting_NonCore, hc05_settings.device_name, NULL, NULL },
    { Setting_BlueToothStateInput, Group_AuxPorts, "Bluetooth state port", NULL, Format_Int8, "#0", "0", max_port, Setting_NonCore, &hc05_settings.state_port, NULL, is_setting_available, { .reboot_required = On } },
};

#ifndef NO_SETTINGS_DESCRIPTIONS

static const setting_descr_t bluetooth_settings_descr[] = {
    { Setting_BlueToothInitOK,     "Uncheck to enter autoconfig mode on startup when AT-mode button is pressed." },
    { Setting_BlueToothDeviceName, "Bluetooth device name." },
    { Setting_BlueToothStateInput, "Aux port number to use for the STATE pin input." },
};

#endif

static setting_details_t setting_details = {
    .groups = bluetooth_groups,
    .n_groups = sizeof(bluetooth_groups) / sizeof(setting_group_detail_t),
    .settings = bluetooth_settings,
    .n_settings = sizeof(bluetooth_settings) / sizeof(setting_detail_t),
#ifndef NO_SETTINGS_DESCRIPTIONS
    .descriptions = bluetooth_settings_descr,
    .n_descriptions = sizeof(bluetooth_settings_descr) / sizeof(setting_descr_t),
#endif
    .save = hc05_settings_save,
    .load = hc05_settings_load,
    .restore = hc05_settings_restore,
};

static void hc05_settings_save (void)
{
    hal.nvs.memcpy_to_nvs(nvs_address, (uint8_t *)&hc05_settings, sizeof(hc05_settings_t), true);
}

static void hc05_settings_restore (void)
{
    hc05_settings.options.enable = false;
    strcpy(hc05_settings.device_name, "grblHAL");

    if(ioport_can_claim_explicit()) {

        xbar_t *portinfo;
        uint8_t port = n_ports;

        hc05_settings.state_port = 0;

        // Find highest numbered port that supports change interrupt.
        if(port > 0) do {
            port--;
            if((portinfo = hal.port.get_pin_info(Port_Digital, Port_Input, port))) {
                if(!portinfo->cap.claimed && (portinfo->cap.irq_mode & IRQ_Mode_Change)) {
                    hc05_settings.state_port = port;
                    break;
                }
            }
        } while(port);
    }

    hc05_settings_save();
}

static void warning_pin (uint_fast16_t state)
{
    report_message("Bluetooth plugin failed to initialize, no pin for STATE signal!", Message_Warning);
}

static void warning_stream (uint_fast16_t state)
{
    report_message("Bluetooth plugin failed to initialize, no serial stream available!", Message_Warning);
}

static void hc05_settings_load (void)
{
    if(hal.nvs.memcpy_from_nvs((uint8_t *)&hc05_settings, nvs_address, sizeof(hc05_settings_t), true) != NVS_TransferResult_OK)
        hc05_settings_restore();

    if(hc05_settings.state_port >= n_ports)
        hc05_settings.state_port = n_ports - 1;

    if(*hc05_settings.device_name == '\0')
        strcpy(hc05_settings.device_name, "grblHAL");

    state_port = hc05_settings.state_port;

    xbar_t *portinfo = hal.port.get_pin_info(Port_Digital, Port_Input, state_port);

    if(portinfo && !portinfo->cap.claimed && (portinfo->cap.irq_mode & IRQ_Mode_Change) && ioport_claim(Port_Digital, Port_Input, &state_port, "HC-05 STATE"))
        protocol_enqueue_rt_command(hc05_setup);
    else
        protocol_enqueue_rt_command(warning_pin);
}

static void report_options (bool newopt)
{
    on_report_options(newopt);

    if(!newopt)
        hal.stream.write("[PLUGIN:Bluetooth HC-05 v0.07]" ASCII_EOL);
}

bool bluetooth_init (void)
{
    bool ok;
    io_stream_t const *stream = stream_open_instance(255, 115200, NULL); // open first free serial port

    if((ok = stream != NULL)) {

        memcpy(&bt_stream, stream, sizeof(io_stream_t));

        bt_stream.type = StreamType_Bluetooth;

        if(hal.periph_port.set_pin_description) {
            hal.periph_port.set_pin_description(Output_TX, (pin_group_t)(PinGroup_UART + bt_stream.instance), "Bluetooth");
            hal.periph_port.set_pin_description(Input_RX, (pin_group_t)(PinGroup_UART + bt_stream.instance), "Bluetooth");
        }
    }

    if(ok) {

        if(!ioport_can_claim_explicit()) {

            // Driver does not support explicit pin claiming, claim the highest numbered port instead.

            if((ok = (n_ports = hal.port.num_digital_in) > 0 && (nvs_address = nvs_alloc(sizeof(hc05_settings_t)))))
                hc05_settings.state_port = --hal.port.num_digital_in;

        } else if((ok = (n_ports = ioports_available(Port_Digital, Port_Input)) > 0 && (nvs_address = nvs_alloc(sizeof(hc05_settings_t)))))
            strcpy(max_port, uitoa(n_ports - 1));

        if(ok) {

            on_report_options = grbl.on_report_options;
            grbl.on_report_options = report_options;

            settings_register(&setting_details);

        } else
            protocol_enqueue_rt_command(warning_pin);

    } else
        protocol_enqueue_rt_command(warning_stream);

    return ok;
}
