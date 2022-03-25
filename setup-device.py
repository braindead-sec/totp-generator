#!/usr/bin/env python3

import sys
import math
import datetime
import time
import glob
import base64

import serial

### FUNCTIONS ###

def print_bar():
    print('-' * 40)

# Find available serial ports
def get_serial_ports():
    if sys.platform.startswith('win'):
        ports = ['COM%s' % (i + 1) for i in range(256)]
    elif sys.platform.startswith('linux') or sys.platform.startswith('cygwin'):
        # this excludes your current terminal "/dev/tty"
        ports = glob.glob('/dev/tty[A-Za-z]*')
    elif sys.platform.startswith('darwin'):
        ports = glob.glob('/dev/tty.*')
    else:
        print('Unsupported platform.')
        sys.exit(1)
    result = []
    for port in ports:
        try:
            s = serial.Serial(port)
            s.close()
            result.append(port)
        except (OSError, serial.SerialException):
            pass
    if len(result) == 0:
        print('No serial ports available.')
        sys.exit(1)
    return result

# Intialize serial connection
def serial_connect(usb_tty):
    try:
        return serial.Serial(usb_tty, 115200)
    except:
        time.sleep(2)
        serial_connect(usb_tty)

# Send one message via serial
def serial_send(msg, encode=True):
    if encode:
        tty.write('{}\n'.format(msg).encode('utf-8'))
    else:
        tty.write(msg)
        tty.write('\n'.encode('utf-8'))

# Receive one message via serial
def serial_receive():
    print_bar()
    print('[+] {}'.format(tty.readline().decode('utf-8')), end='')

# Receive one message via serial without printing
def serial_get():
    return tty.readline().decode('utf-8')

# Get current UTC timestamp
def get_now():
    now = datetime.datetime.now()
    ts = math.trunc(datetime.datetime.timestamp(now))
    return ts

# Sync time from computer to the device
def sync_time():
    serial_send('sync')
    now = get_now()
    print('Sending timestamp: {}'.format(now))
    serial_send(now)
    serial_receive()

# Read key from the device
def get_key():
    serial_send('get')
    result = int(serial_get())
    if result == 0:
        print_bar()
        print('[+] No keys set')
    else:
        while result > 0:
            serial_receive()
            result -= 1

# Write keys to the device
def set_key():
    # Get key name
    key_name = input('Enter a name for the key (3 chars): ')[0:3]
    # Pad to 3 chars if necessary
    while len(key_name) < 3:
        key_name += ' ' 
    print('What key do you want to add?')
    try:
        # Get key in base32 format
        b32_key = input('Enter in base32 format: ')
        # Add padding if necessary
        while len(b32_key) % 8 > 0:
            b32_key = b32_key + '='
        # Convert to hexadecimal
        hex_key = base64.b32decode(b32_key)
    except:
        print('[+] Invalid key')
        return
    serial_send('set')
    serial_send(key_name)
    serial_send(hex_key, False)
    serial_receive()

# Delete key from the device
def del_key():
    get_key()
    print_bar()
    key_id = int(input('Which key do you want to delete? ')) - 1
    serial_send('del')
    serial_send(key_id)
    serial_receive()

# Reset device settings
def reset():
    serial_send('wipe')
    serial_receive()

### MAIN ###

# Choose a serial port
menu = get_serial_ports()
print_bar()
print('Which port is your device connected to?')
print_bar()
for i in range(1, len(menu)+1):
    print('{}. {}'.format(i, menu[i-1]))
print_bar()
selection = input('> ')
try:
    usb_tty = menu[int(selection)-1]
except:
    print('Invalid selection. Bye!')
    sys.exit(1)

# Connect to serial terminal and sync time
print_bar()
print('Connecting to serial terminal at {}...'.format(usb_tty))
tty = serial_connect(usb_tty)
time.sleep(2)
sync_time()

# Show menu with other options
menu = [
    'Sync time',
    'List keys',
    'Add key',
    'Delete key',
    'Reset device',
    'Quit'
]
while True:
    print_bar()
    print('What else would you like to do?')
    print_bar()
    for i in range(1, len(menu)+1):
        print('{}. {}'.format(i, menu[i-1]))
    print_bar()
    selection = int(input('> '))
    if selection < len(menu)+1:
        if selection == 1:
            sync_time()
        elif selection == 2:
            get_key()
        elif selection == 3:
            set_key()
        elif selection == 4:
            del_key()
        elif selection == 5:
            reset()
        elif selection == 6:
            break
    else:
        print('Poor choice. Try again.')

# Clean up and quit
print('OK bye!')
tty.close()
