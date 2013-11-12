/*
 * Tool to control the extra features of the Logitech MX 5500 bluetooth keyboard in hci mod based on Hidraw
 *
 * Copyright (c) 2013 Julian Sparber <julian@sparber.net>
 * Copyright (c) 2010 Signal 11 Software
 */

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>
/*
 * Ugly hack to work around failing compilation on systems that don't
 * yet populate new version of hidraw.h to userspace.
 */
#ifndef HIDIOCSFEATURE
#warning Please have your distro update the userspace kernel headers
#define HIDIOCSFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x06, len)
#define HIDIOCGFEATURE(len)    _IOC(_IOC_WRITE|_IOC_READ, 'H', 0x07, len)
#endif

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <time.h>
int open_device(int fd);
const char *bus_str(int bus);
int set_time(int fd);
int beep(int fd);
int set_temp_unit(int fd, char unit);
int no_arg();
int check(int res);

int main(int argc, char *argv[]){
	int fd;
	/* Open the Device. In real life,
	don't use a hard coded path; use libudev instead. */
	fd = open("/dev/hidraw1", O_RDWR);
	/*if (!open_device(fd)){*/
	if (!open_device(fd)){
		if ( argc > 1 ){
			switch (*(argv[1]+1)){
				case 't':
						printf("Set time...\n");
						set_time(fd);
						printf("Done.\n");
						break;
				case 'b':
						printf("Send beep cmd...\n");
						beep(fd);
						printf("Done.\n");
						break;
				case 'u':
						printf("Set temp_unit...\n");
						if ( argc > 2 && (*argv[2] == 'c' || *argv[2] == 'f' )){
							set_temp_unit(fd, *(argv[2]));
							printf("Done.\n");
						}
						else{
							no_arg();
						}	
						break;
				default:
						no_arg();
						break;
			}
		}
		else {
			no_arg();
		}
		close(fd);
	}
	else
		no_arg();
	return 0;
}

/* Send a Report to the Device */
/* Report Number is always the first element*/

/*Sends beep comand to the keyboard*/
int beep(int fd){
	int res;
	/*const char beep1[] = { 0x10, 0x01, 0x81, 0x50, 0x00, 0x00, 0x00 };*/
	const char beep2[] = { 0x10, 0x01, 0x80, 0x50, 0x02, 0x00, 0x00 };
	
	res = write(fd, beep2, 7);
	if (res < 0) {
		printf("Error: %d\n", errno);
		   	
		perror("write");
	}
	return res;
}


/*changes temp unit*/
int set_temp_unit(int fd, char unit){
	int res;
	/*char tempunit1[] = { 0x10, 0x01, 0x81, 0x30, 0x00, 0x00, 0x00 };*/
	char tempunit2[] = { 0x10, 0x01, 0x80, 0x30, 0xDC, 0x00, 0x00 };

	/* 0x00 for °C and 0x01 for °F*/
	if ( unit != 'f' )
		tempunit2[6] = 0x00;
	else
		tempunit2[6] = 0x01;
		
	res = write(fd, tempunit2, 7);
	if (res < 0) {
		printf("Error: %d\n", errno);
		perror("write");
	}
	return res;
}
/*set time to system time*/
int set_time(int fd){
	int res;
														/*mday  mon*/
	char daymonth[] = 	{ 0x10, 0x01, 0x80, 0x32, 0x00, 0x00, 0x00 };
														/*min hour*/
	char hourminute[] = { 0x10, 0x01, 0x80, 0x31, 0x19, 0x00, 0x00 };
												 /*year 0x00 is 2000*/
	char year[] = 		{ 0x10, 0x01, 0x80, 0x33, 0x00, 0x00, 0x00 };

	time_t mytime = 0;
	struct tm mytm;
	mytime = time(NULL);
	localtime_r(&mytime, &mytm);

	 /*Day*/
	daymonth[5] = mytm.tm_mday;
	/*Month*/
	daymonth[6] = mytm.tm_mon;
	/*year*/
	year[4] = mytm.tm_year - 100; /*y3k bug*/
	/*min*/
	hourminute[5] = mytm.tm_min;;
	/*hours*/
	hourminute[6] = mytm.tm_hour;
	/*Write to keyboard*/
	res = write(fd, daymonth, 7);
	res += write(fd, hourminute, 7);
	res += write(fd, year, 7);
  	
  	return res;
}

      /* Get a report from the device */
      /*  
        res = read(fd, buf, 16);
        if (res < 0) {
                perror("read");
        } else {
                printf("read() read %d bytes:\n\t", res);
                for (i = 0; i < res; i++)
                        printf("%hhx ", buf[i]);
                puts("\n");
        }*/


int check(int res){
	
}

/*Open the device*/
int open_device(int fd){
	int res;
	char buf[256];
	struct hidraw_report_descriptor rpt_desc;
	struct hidraw_devinfo info;

	if (fd < 0) {
		perror("Unable to open device");
		res = 1;
	}
	else {
		memset(&rpt_desc, 0x0, sizeof(rpt_desc));
		memset(&info, 0x0, sizeof(info));
		memset(buf, 0x0, sizeof(buf));

		/* Get Raw Name */
		res = ioctl(fd, HIDIOCGRAWNAME(256), buf);
		if (res < 0)
			perror("HIDIOCGRAWNAME");
		else
			printf("Raw Name: %s\n", buf);

		/* Get Physical Location */
		res = ioctl(fd, HIDIOCGRAWPHYS(256), buf);
		if (res < 0)
			perror("HIDIOCGRAWPHYS");
		else
			printf("Raw Phys: %s\n", buf);

		/* Get Raw Info */
		res = ioctl(fd, HIDIOCGRAWINFO, &info);
		if (res < 0) {
			perror("HIDIOCGRAWINFO");
		}
		else {
			printf("Raw Info:\n");
			printf("\tbustype: %d (%s)\n",
			        info.bustype, bus_str(info.bustype));
			printf("\tvendor: 0x%04hx\n", info.vendor);
			printf("\tproduct: 0x%04hx\n", info.product);
		}
		res = 0;
	}
	return res;
}

const char *bus_str(int bus){
	switch (bus) {
	case BUS_USB:
		    return "USB";
		    break;
	case BUS_HIL:
		    return "HIL";
		    break;
	case BUS_BLUETOOTH:
		    return "Bluetooth";
		    break;
	case BUS_VIRTUAL:
		    return "Virtual";
		    break;
	default:
		    return "Other";
		    break;
	}
}

int no_arg(){
	printf("No Operation argument\nArguments:\n\t-b -> for beep\n\t-t -> sets time to localtime\n\t-u -> set temp unit -u < c | f >\n");
	return 0;
}
