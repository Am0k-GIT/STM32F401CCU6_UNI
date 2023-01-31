/*

  select.c - spindle select plugin

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

#include <math.h>
#include <string.h>

#ifdef ARDUINO
#include "../grbl/hal.h"
#else
#include "grbl/hal.h"
#endif

static user_mcode_ptrs_t user_mcode;
static on_report_options_ptr on_report_options;

static user_mcode_t check (user_mcode_t mcode)
{
    return mcode == Spindle_Select ? mcode : (user_mcode.check ? user_mcode.check(mcode) : UserMCode_Ignore);
}

static status_code_t validate (parser_block_t *gc_block, parameter_words_t *deprecated)
{
    status_code_t state = Status_OK;

    if(gc_block->user_mcode == Spindle_Select) {

        if(gc_block->words.p) {
            if(isnanf(gc_block->values.p))
                state = Status_GcodeValueWordMissing;
            else if(!(isintf(gc_block->values.p) && gc_block->values.p >= 0.0f && gc_block->values.p <= 1.0f))
                state = Status_GcodeValueOutOfRange;
        } else if(gc_block->words.q) {
            if(isnanf(gc_block->values.q))
                state = Status_GcodeValueWordMissing;
            else if(!(isintf(gc_block->values.q) && gc_block->values.q >= 0.0f && gc_block->values.q <= (float)spindle_get_count()))
                state = Status_GcodeValueOutOfRange;
        } else
            state = Status_GcodeValueWordMissing;

        if(state == Status_OK && gc_block->words.p != gc_block->words.q) {
            gc_block->words.p = gc_block->words.q = Off;
            gc_block->user_mcode_sync = On;
        } else
            state = Status_GcodeValueOutOfRange;

    } else
        state = Status_Unhandled;

    return state == Status_Unhandled && user_mcode.validate ? user_mcode.validate(gc_block, deprecated) : state;
}

static void execute (sys_state_t state, parser_block_t *gc_block)
{
    if(gc_block->user_mcode == Spindle_Select) {
        if(gc_block->words.p)
            spindle_select((spindle_id_t)(gc_block->values.p == 0.0f ? 0 : settings.spindle.flags.type));
        else
            spindle_select((spindle_id_t)(gc_block->values.q));
    } else if(user_mcode.execute)
        user_mcode.execute(state, gc_block);
}

static void report_options (bool newopt)
{
    on_report_options(newopt);

    if(!newopt && hal.stream.write_n) {

        const setting_detail_t *spindles;

        if((spindles = setting_get_details(Setting_SpindleType, NULL))) {

            uint16_t len = 1;
            const char *s = spindles->format;
            spindle_id_t spindle = spindle_get_current();

            while(spindle-- && s) {
                s = strchr(s, ',');
                if(s)
                    s++;
            }

            if(s)
                len = (strchr(s, ',') ? strchr(s, ',') : strchr(s, '\0')) - s;

            hal.stream.write("[SPINDLE:");
            hal.stream.write_n(s, len);
            hal.stream.write("]" ASCII_EOL);
        }
    }
}

void spindle_select_init (void)
{
    if(spindle_get_count() > 1) {

        memcpy(&user_mcode, &hal.user_mcode, sizeof(user_mcode_ptrs_t));

        hal.user_mcode.check = check;
        hal.user_mcode.validate = validate;
        hal.user_mcode.execute = execute;

        on_report_options = grbl.on_report_options;
        grbl.on_report_options = report_options;
    }
}
