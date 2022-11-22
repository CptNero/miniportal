set -e
make && cd examples/board_olimex_avr_mt128 && ./obj-x86_64-pc-linux-gnu/mt128.elf ./atmega128_portal.axf
