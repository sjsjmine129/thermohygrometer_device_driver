https://github.com/DavidAntliff/esp32-i2c-lcd1602.git


$ git clone --recursive https://github.com/DavidAntliff/esp32-i2c-lcd1602-example.git

$ cd esp32-i2c-lcd1602-example.git
$ idf.py menuconfig    # set your serial configuration and the I2C GPIO - see below
$ idf.py build
$ idf.py -p (PORT) flash monitor
