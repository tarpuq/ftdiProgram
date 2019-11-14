/* LIBFTDI EEPROM access example
    author: Galo Guzman

*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
//#include <ctype.h>
#include <unistd.h>
//#include <getopt.h>
#include <ftdi.h>

int read_decode_eeprom(struct ftdi_context *ftdi)
{
    int i, j, f;
    int value;
    int size;
    unsigned char buf[256];
    unsigned char serialNumber[64];

    f = ftdi_read_eeprom(ftdi);
    if (f < 0)
    {
        fprintf(stderr, "ftdi_read_eeprom: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
        return -1;
    }

    ftdi_get_eeprom_value(ftdi, CHIP_SIZE, &value);
    if (value < 0)
    {
        fprintf(stderr, "No EEPROM found or EEPROM empty\n");
        fprintf(stderr, "On empty EEPROM, use -w option to write default values\n");
        return -1;
    }
    fprintf(stderr, "Chip type %d ftdi_eeprom_size: %d\n", ftdi->type, value);

    if (ftdi->type == TYPE_R)
        size = 0xa0;
    else
        size = value;

    ftdi_get_eeprom_buf(ftdi, buf, size);
    for (i = 0; i < size; i += 16)
    {
        fprintf(stdout, "0x%03x:", i);

        for (j = 0; j < 8; j++)
            fprintf(stdout, " %02x", buf[i + j]);
        fprintf(stdout, " ");
        for (; j < 16; j++)
            fprintf(stdout, " %02x", buf[i + j]);
        fprintf(stdout, " ");
        for (j = 0; j < 8; j++)
            fprintf(stdout, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        fprintf(stdout, " ");
        for (; j < 16; j++)
            fprintf(stdout, "%c", isprint(buf[i + j]) ? buf[i + j] : '.');
        fprintf(stdout, "\n");
    }

    f = ftdi_eeprom_decode(ftdi, 1);
    if (f < 0)
    {
        fprintf(stderr, "ftdi_eeprom_decode: %d (%s)\n", f, ftdi_get_error_string(ftdi));
        return -1;
    }
    return 0;
}

int main(int argc, char **argv)
{
    struct ftdi_context *ftdi;
    int f, i;
    int vid = 0;
    int pid = 0;
    char const *product = 0;
    char const *serial = 0;
    char truncatedSerial[10];
    int erase = 0;
    int use_defaults = 0;
    int large_chip = 0;
    int do_write = 0;
    int retval = 0;
    int value;
    unsigned char manufacturer[64];

    while ((i = getopt(argc, argv, "d::ev:p:l:P:S:w")) != -1)
    {
        switch (i)
        {
        case 'd':
            use_defaults = 1;
            if (optarg)
                large_chip = 0x66;
            break;
        case 'e':
            erase = 1;
            break;
        case 'v':
            vid = strtoul(optarg, NULL, 0);
            break;
        case 'p':
            pid = strtoul(optarg, NULL, 0);
            break;
        case 'P':
            product = optarg;
            break;
        case 'S':
            serial = optarg;
            strncpy(truncatedSerial,serial + strlen(serial) - 5,5);
            break;
        case 'w':
            do_write = 1;
            break;
        default:
            fprintf(stderr, "usage: %s [options]\n", *argv);
            fprintf(stderr, "\t-d[num] Work with default valuesfor 128 Byte "
                            "EEPROM or for 256 Byte EEPROM if some [num] is given\n");
            fprintf(stderr, "\t-w write\n");
            fprintf(stderr, "\t-e erase\n");
            fprintf(stderr, "\t-v verbose decoding\n");
            fprintf(stderr, "\t-p <number> Search for device with PID == number\n");
            fprintf(stderr, "\t-v <number> Search for device with VID == number\n");
            fprintf(stderr, "\t-P <string? Search for device with given "
                            "product description\n");
            fprintf(stderr, "\t-S <string? Search for device with given "
                            "serial number\n");
            retval = -1;
            goto done;
        }
    }

    if ((ftdi = ftdi_new()) == 0)
    {
        fprintf(stderr, "Failed to allocate ftdi structure :%s \n",
                ftdi_get_error_string(ftdi));
        return EXIT_FAILURE;
    }
    else
    {
        fprintf(stdout, "Allocate ftdi structure OK \n");
    }

    // Select first interface
    ftdi_set_interface(ftdi, INTERFACE_ANY);
    struct ftdi_device_list *devlist, *curdev;
    int res;
    if ((res = ftdi_usb_find_all(ftdi, &devlist, 0, 0)) < 0)
    {
        fprintf(stderr, "No FTDI with default VID/PID found\n");
        retval = EXIT_FAILURE;
        goto do_deinit;
    }
    if (res > 1)
    {
        int i = 1;
        fprintf(stderr, "%d FTDI devices found: Only Readout on EEPROM done. ", res);
        fprintf(stderr, "Use VID/PID/desc/serial to select device\n");
        for (curdev = devlist; curdev != NULL; curdev = curdev->next, i++)
        {
            f = ftdi_usb_open_dev(ftdi, curdev->dev);
            if (f < 0)
            {
                fprintf(stderr, "Unable to open device %d: (%s)",
                        i, ftdi_get_error_string(ftdi));
                continue;
            }
            fprintf(stderr, "Decoded values of device %d:\n", i);
            read_decode_eeprom(ftdi);
            ftdi_usb_close(ftdi);
        }
        ftdi_list_free(&devlist);
        retval = EXIT_SUCCESS;
        goto do_deinit;
    }
    else if (res == 1)
    {
        fprintf(stdout, "Found one device\n");
        f = ftdi_usb_open_dev(ftdi, devlist[0].dev);
        if (f < 0)
        {
            fprintf(stderr, "Unable to open device %d: (%s)",
                    i, ftdi_get_error_string(ftdi));
        }
    }
    else
    {
        fprintf(stderr, "No devices found\n");
        f = 0;
    }
    ftdi_list_free(&devlist);

    retval = read_decode_eeprom(ftdi);

    strcpy(manufacturer, "Tarpuq Cia. Ltda.");

    fprintf(stdout, "\nProgramming Device\n");
    fprintf(stdout, "Product: %s\n",product);
    fprintf(stdout, "Serial: %s\n",serial);
    fprintf(stdout, "Truncated serial: %s\n",truncatedSerial);

    ftdi_eeprom_initdefaults(ftdi, manufacturer, product, truncatedSerial);

    f = ftdi_erase_eeprom(ftdi);
    if (ftdi_set_eeprom_value(ftdi, SELF_POWERED, 1) < 0)
    {
        fprintf(stderr, "ftdi_set_eeprom_value: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
    }

    f = ftdi_erase_eeprom(ftdi);
    if (ftdi_set_eeprom_value(ftdi, SUSPEND_PULL_DOWNS, 1) < 0)
    {
        fprintf(stderr, "ftdi_set_eeprom_value: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
    }

    f = ftdi_erase_eeprom(ftdi);
    if (ftdi_set_eeprom_value(ftdi, MAX_POWER , 0) < 0)
    {
        fprintf(stderr, "ftdi_set_eeprom_value: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
    }

    f = ftdi_erase_eeprom(ftdi);
    if (ftdi_set_eeprom_value(ftdi, CBUS_FUNCTION_0, CBUSX_RXLED) < 0)
    {
        fprintf(stderr, "ftdi_set_eeprom_value: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
    }

    f = ftdi_erase_eeprom(ftdi);
    if (ftdi_set_eeprom_value(ftdi, CBUS_FUNCTION_1, CBUSX_TXLED) < 0)
    {
        fprintf(stderr, "ftdi_set_eeprom_value: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
    }

    f = ftdi_erase_eeprom(ftdi);
    if (ftdi_set_eeprom_value(ftdi, CBUS_FUNCTION_2, CBUSX_TRISTATE) < 0)
    {
        fprintf(stderr, "ftdi_set_eeprom_value: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
    }

    f = ftdi_erase_eeprom(ftdi);
    if (ftdi_set_eeprom_value(ftdi, CBUS_FUNCTION_3, CBUSX_VBUS_SENSE) < 0)
    {
        fprintf(stderr, "ftdi_set_eeprom_value: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
    }

    f = ftdi_erase_eeprom(ftdi);
    if (ftdi_get_eeprom_value(ftdi, CHIP_TYPE, &value) < 0)
    {
        fprintf(stderr, "ftdi_get_eeprom_value: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
    }

    if (value == -1)
        fprintf(stderr, "No EEPROM\n");
    else if (value == 0)
        fprintf(stderr, "Internal EEPROM\n");
    else
        fprintf(stderr, "Found 93x%02x\n", value);

    f = ftdi_eeprom_build(ftdi);
    if (f < 0)
    {
        fprintf(stderr, "Erase failed: %s",
                ftdi_get_error_string(ftdi));
        retval = -2;
        goto done;
    }

    f = ftdi_write_eeprom(ftdi);
    if (f < 0)
    {
        fprintf(stderr, "ftdi_eeprom_decode: %d (%s)\n",
                f, ftdi_get_error_string(ftdi));
        retval = 1;
        goto done;
    }

    retval = read_decode_eeprom(ftdi);

done:
    ftdi_usb_close(ftdi);
do_deinit:
    ftdi_free(ftdi);
    fprintf(stdout, "retCode %d\n", retval);
    return retval;
}
