/*

  lb_clusters.c - plugin for unpacking LightBurn cluster encoded engraving moves.

  *** EXPERIMENTAL ***

  Part of grblHAL

  Copyright (c) 2022 Terje Io

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

#include "driver.h"

#if LB_CLUSTERS_ENABLE

#include "grbl/hal.h"
#include "grbl/gcode.h"
#include "grbl/protocol.h"

#include <string.h>

#ifndef LB_CLUSTER_SIZE
#define LB_CLUSTER_SIZE 16
#endif

#ifndef LB_SVALUE_SCALING
#define LB_SVALUE_SCALING 0 // Change to 1 if S-values is to be multiplied by $30 value (max RPM).
#endif

static struct {
    char block[LINE_BUFFER_SIZE];
    char *s;
    char *cmd;
    char eol;
    uint_fast16_t length;
} input = {0};

static struct {
    char block[24];
    char sval[LB_CLUSTER_SIZE][10];
    char *s;
    char *cmd;
    uint_fast16_t count;
    uint_fast16_t next;
} cluster;

static stream_read_ptr file_read = NULL, stream_read = NULL;
static on_stream_changed_ptr on_stream_changed;
static on_report_handlers_init_ptr on_report_handlers_init;
static status_message_ptr status_message = NULL;
static on_report_options_ptr on_report_options;
static on_reset_ptr on_reset;

static inline char *get_value (char *v, uint_fast16_t scale)
{
    float val;
    uint_fast8_t cc = 0;

    read_float(v, &cc, &val);

    return ftoa(val / (float)scale, 8);
}

#if LB_SVALUE_SCALING

static inline char *get_s_value (char *v)
{
    char *s;
    float val;
    uint_fast8_t cc = 0;

    read_float(v, &cc, &val);

    s = ftoa(val * settings.spindle.rpm_max, 0);

    *strchr(s, '.') = input.eol;

    return s;
}

#endif

// File stream decoder

static inline void file_fill_buffer (void)
{
    int16_t c;

    if(cluster.count == 0) {

        char *s = input.block;

        input.s = s;
        input.length = 0;

        while((c = file_read()) != SERIAL_NO_DATA) {
            *s++ = (char)c;
            input.length++;
            if((char)c == '\n' || (char)c == '\r') {
                if(input.length == 1 && input.eol && input.eol != (char)c) {
                    input.eol = '\0';
                    input.s = s;
                    input.length = 0;
                }
                input.eol = c;
                break;
            }
        }

        *s = '\0';

        if(input.length > 5 && !strncasecmp(input.block, "G1", 2) && strchr(input.block, ':')) {

            char *s2 = input.block, *s3;

            s = cluster.block;

            while((c = *s2++)) {
                if(c != ' ')
                    *s++ = c;
                if(c == 'S')
                    break;
            }
            *s = '\0';

            cluster.s = s;
            cluster.cmd = cluster.block;
            cluster.count = cluster.next = 0;

            s3 = s2;
            while((c = *s2)) {
                if(c == ':') {
#if LB_SVALUE_SCALING
                    *s2++ = '\0';
                    strcpy(cluster.sval[cluster.count++], get_s_value(s3));
#else
                    *s2++ = input.eol;
                    c = *s2;
                    *s2 = '\0';
                    strcpy(cluster.sval[cluster.count++], s3);
                    *s2 = (char)c;
#endif
                    if(cluster.count == LB_CLUSTER_SIZE) {
                        cluster.count = input.length = 0;
                        return;
                    }
                    s3 = s2;
                }
                s2++;
            }
#if LB_SVALUE_SCALING
            strcpy(cluster.sval[cluster.count++], get_s_value(s3));
#else
            *s2 = '\0';
            strcpy(cluster.sval[cluster.count++], s3);
#endif
            s = cluster.cmd + 3;
            strcpy(s, get_value(s, cluster.count));
            cluster.s = strchr(s, '\0');
            while(*(cluster.s - 1) == '0')
                *(--cluster.s) = '\0';
            strcat(cluster.s++, "S");
        }
    }

    if(cluster.count) {
        strcpy(cluster.s, cluster.sval[cluster.next++]);
//        if(cluster.next == 2) !! oddly this slows down the parser
//            cluster.cmd += 2;
        if(cluster.next == cluster.count)
            cluster.count = 0;
        input.s = cluster.cmd;
        input.length = strlen(cluster.cmd);
    }
}

static int16_t file_decoder (void)
{
    int16_t c;

    if(input.length == 0)
        file_fill_buffer();

    if(input.length) {
        c = *input.s++;
        input.length--;
    } else
        c = SERIAL_NO_DATA;

    return c;
}

// "Normal" stream decoder

static int16_t stream_fill_buffer (void)
{
    static char *s = NULL;

    int16_t c;

    if(s == NULL || input.s == NULL) {
        input.s = s = input.block;
        cluster.count = input.length = 0;
    }

    if(cluster.count == 0) {

        c = stream_read();

        if(c == SERIAL_NO_DATA || c == ASCII_CAN) {

            if(ABORTED) {
                cluster.count = input.length = 0;
                s = NULL;
            }

            return c;
        }

        if(++input.length >= LINE_BUFFER_SIZE - 1) {
            s = NULL;
            return SERIAL_NO_DATA;
        }

        *s++ = (char)c;

        if((char)c == '\n' || (char)c == '\r') {
            if(input.length == 1 && input.eol && input.eol != (char)c) {
                input.eol = '\0';
                input.length = 0;
                s--;
                return SERIAL_NO_DATA;
            }
            input.eol = (char)c;
        } else
            return SERIAL_NO_DATA;

        *s = '\0';

        if(input.length > 5 && !strncasecmp(input.block, "G1", 2) && strchr(input.block, ':')) {

            char *s2 = input.block, *s3;

            s = cluster.block;

            while((c = *s2++)) {
                if(c != ' ')
                    *s++ = c;
                if(c == 'S')
                    break;
            }
            *s = '\0';

            cluster.s = s;
            cluster.cmd = cluster.block;
            cluster.count = cluster.next = 0;

            s3 = s2;
            while((c = *s2)) {
                if(c == ':') {
#if LB_SVALUE_SCALING
                    *s2++ = '\0';
                    strcpy(cluster.sval[cluster.count++], get_s_value(s3));
#else
                    *s2++ = input.eol;
                    c = *s2;
                    *s2 = '\0';
                    strcpy(cluster.sval[cluster.count++], s3);
                    *s2 = (char)c;
#endif
                    if(cluster.count == LB_CLUSTER_SIZE) {
                        s = NULL;
                        return SERIAL_NO_DATA;
                    }
                    s3 = s2;
                }
                s2++;
            }
#if LB_SVALUE_SCALING
            strcpy(cluster.sval[cluster.count++], get_s_value(s3));
#else
            *s2 = '\0';
            strcpy(cluster.sval[cluster.count++], s3);
#endif
            s = cluster.cmd + 3;
            strcpy(s, get_value(s, cluster.count));
            cluster.s = strchr(s, '\0');
            while(*(cluster.s - 1) == '0')
                *(--cluster.s) = '\0';
            strcat(cluster.s++, "S");
        } else
            s = NULL;
    }

    if(cluster.count) {
        strcpy(cluster.s, cluster.sval[cluster.next++]);
//        if(cluster.next == 2) !! oddly this slows down the parser
//            cluster.cmd += 2;
        if(cluster.next == cluster.count) {
            s = NULL;
            cluster.count = 0;
        }
        input.s = cluster.cmd;
        input.length = strlen(cluster.cmd);
    }

    return 0;
}

static int16_t stream_decoder (void)
{
    static bool buffering = true;

    int16_t c;

    if(buffering) {
        while((c = stream_fill_buffer()) != 0) {
            if(ABORTED)
                break;
            return c;
        }
        buffering = false;
    }

    if(input.length) {
        c = *input.s++;
        input.length--;
    } else {
        buffering = true;
        c = SERIAL_NO_DATA;
    }

    return c;
}

// Only respond with a single "ok" message for each cluster
// or terminate cluster unpacking if error status reported.
static status_code_t cluster_status_message (status_code_t status_code)
{
    if(status_code != Status_OK) {
        status_message(status_code);
        if(cluster.next) {
            input.s = NULL;
            cluster.count = cluster.next = input.length = 0;
        }
    } else if(cluster.count == 0)
        status_message(status_code);

    return status_code;
}

static void stream_changed (stream_type_t type)
{
    if(on_stream_changed)
        on_stream_changed(type);

    if(type == StreamType_File) {
        file_read = hal.stream.read;
        hal.stream.read = file_decoder;
    } else if(hal.stream.read != stream_decoder) {
        stream_read = hal.stream.read;
        hal.stream.read = stream_decoder;
    }

    cluster.count = cluster.next = input.length = 0;
}

static void cluster_reset (void)
{
    if(on_reset)
        on_reset();

    cluster.count = cluster.next = input.length = 0;
}

static void cluster_report (void)
{
    if(on_report_handlers_init)
        on_report_handlers_init();

    status_message = grbl.report.status_message;
    grbl.report.status_message = cluster_status_message;
}

static void report_options (bool newopt)
{
    if(!newopt) {
        hal.stream.write("[CLUSTER:");
        hal.stream.write(uitoa(LB_CLUSTER_SIZE));
        hal.stream.write("]" ASCII_EOL);
        hal.stream.write("[PLUGIN:LightBurn clusters v0.04]" ASCII_EOL);
    }

    on_report_options(newopt);
}

void lb_clusters_init (void)
{
    on_stream_changed = grbl.on_stream_changed;
    grbl.on_stream_changed = stream_changed;

    on_report_options = grbl.on_report_options;
    grbl.on_report_options = report_options;

    on_reset = grbl.on_reset;
    grbl.on_reset = cluster_reset;

    on_report_handlers_init = grbl.on_report_handlers_init;
    grbl.on_report_handlers_init = cluster_report;

    stream_changed(hal.stream.type);
}

#endif
