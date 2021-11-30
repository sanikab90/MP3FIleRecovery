#!/bin/bash

printf "Copying mp3 file to usb...\n"

cp brainless.mp3 /media/usb-drive/

printf "Deleting mp3...\n"

rm /media/usb-drive/brainless.mp3

printf "Unmounting USB drive...\n"

umount /dev/sdc1

printf "Running main.c:\n\n"

make

printf "\nMounting usb\n"

mount /dev/sdc1 /media/usb-drive

printf "All done! Check to see if your mp3 is recovered!\n"
