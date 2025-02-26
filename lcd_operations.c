#include "lcd_operations.h"

#define SEND_LENGTH 1
#define LCD_CHR  1
#define LCD_CMD  4 
#define LCD_BACKLIGHT 8 
#define ENABLE 4

uint8_t i2c_data_buffer[2] = {0};
struct i2c_client *client;

int init_client(struct i2c_client *init_client)
{
    client = init_client;
    return 0;
}

// announce send the data or command to lcd
int lcdToggleEnable(int bits) 
{
    char data = bits | ENABLE; // make enable bit high
    if(i2c_master_send(client, &data, SEND_LENGTH) != 1)
    {
        return -1;
    }
    usleep_range(500, 500);

    data = bits & ~ENABLE; // set enable bit low
    if(i2c_master_send(client, &data, SEND_LENGTH) != 1)
    {
        return -1;
    }
    usleep_range(500, 500);

    return 0;
}

int send_data_to_lcd(uint16_t data, uint16_t data_type)
{
    //set data
    i2c_data_buffer[0] = data_type | (data & 0xF0) | LCD_BACKLIGHT; 
    i2c_data_buffer[1] = data_type | ((data << 4) & 0xF0) | LCD_BACKLIGHT;
    
    //send bit data twice
    for(int i = 0; i < 2; i++)
    {
        if(i2c_master_send(client, &i2c_data_buffer[i], SEND_LENGTH) != 1)
        {
            return -1;
        }

        if(lcdToggleEnable(i2c_data_buffer[i]))
        {
            return -1;
        }
    }

    return 0;
}


int send_command_to_lcd(uint16_t command)
{
    if(send_data_to_lcd(command, LCD_CMD))
    {
        printk(KERN_ALERT "fail to send command: %x\n", command);
        return -1;
    }

    return 0;
}


int write_text_to_lcd(const char *text, int length) 
{
    int write_len = 0;

    for (int i = 0; i < length && *text; i++) 
    {
        if(send_data_to_lcd(*text++, LCD_CHR) != 0)
        {
            printk(KERN_ALERT "fail to write text: %s\n", text);
            return -1;
        }
        write_len++;
    }

    return write_len;
}


MODULE_AUTHOR("seungju.eum");
MODULE_LICENSE("Dual BSD/GPL");
MODULE_DESCRIPTION("Character device driver for SHT31 sensor with I2C support");