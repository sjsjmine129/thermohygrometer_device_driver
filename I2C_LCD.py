#!/usr/bin/env python3
import LCD1602
import time

def setup():
    LCD1602.init(0x3f, 1)
    LCD1602.write(0, 0, 'Hello World!!')
    LCD1602.write(5, 1, '- RPi 400 -')
    time.sleep(2)

def destroy():
    pass

if __name__ == "__main__":
    try:
        setup()
        while True:
            pass
    except KeyboardInterrupt:
        destroy()
