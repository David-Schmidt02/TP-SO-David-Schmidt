#!/usr/bin/bash

cd kernel
make

cd ../cpu
make

cd ../memoria
make

cd ../filesystem
make

konsole --noclose -e "bash -c 'cd /home/ozi/scripts/tp-2024-2c-SAFED/kernel && ./bin/kernel; exec bash'" & konsole --noclose -e "bash -c 'cd /home/ozi/scripts/tp-2024-2c-SAFED/cpu && ./bin/cpu; exec bash'" & konsole --noclose -e "bash -c 'cd /home/ozi/scripts/tp-2024-2c-SAFED/filesystem && ./bin/filesystem; exec bash'" & konsole --noclose -e "bash -c 'cd /home/ozi/scripts/tp-2024-2c-SAFED/memoria && ./bin/memoria; exec bash'"
