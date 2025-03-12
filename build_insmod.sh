cd device_driver

make
sudo insmod sensor_driver.ko
sudo insmod lcd_driver.ko

cd ..

gcc user_main.c thermohygrometer.c -o user_main.exe