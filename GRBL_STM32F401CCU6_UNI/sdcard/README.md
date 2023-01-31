## SD Card driver plugin

This plugin adds a few `$` commands for listing files and running G-code from a SD Card:

`$FM`

Mount card.

`$F`

List files on the card recursively.

`$F=<filename>`

Run g-code in file. If the file ends with `M2` or rewind mode is active then it will be "rewound" and a cycle start command will start it again.

`$FR`

Enable rewind mode for next file to be run.

`$F<=<filename>`

Dump file content to output stream.

`$FD=<filename>`

Delete file.

Dependencies:

[FatFS library](http://www.elm-chan.org/fsw/ff/00index_e.html)

__NOTE:__ some drivers uses ports of FatFS provided by the MCU supplier.


Experimental ymodem upload support has been added as an option from build 20210422. Enable by setting `SDCARD_ENABLE` to `2` in _my_machine.h_.  
This requires a compatible sender as the initial `C` used to start the transfer will not be sent by the controller. Transfer is initiated by the sender by sending the first packet.

---
2021-04-23
