/*
 * userspace applicaton to control and comunicate with associated kernel module
 * can be accessed through command line arguments "count", "reset" and "time"
 * or a simple text menu when no arguments are provided
 */

#include<stdio.h>
#include<stdlib.h>
#include<fcntl.h>
#include<sys/ioctl.h>
#include<time.h>
#include<string.h>

#include"keyboard_interrupt_counter.h"

// prototypes
int retrieve_count(int);
/*
 * prints out current interrupt count provided by kernel,
 * returns 0 on success or error code on failure
 */

int reset_count(int);
/*
 * tasks the cernel with reseting interrupt counter,
 * returns 0 on success or error code on failure
 */

int retrieve_time(int);
/*
 * prints out date and time of last counter reset,
 * returns 0 on success or error code on failure
 */

void menu(int);
/*
 * prints out options provided to the user and returns requested data by
 * invoking other fuctions necessary
 */


int main(int argc, char *argv[])
{
	char choice[10];
	int file_descriptor, ret = 0;
	// accesing the device file
	file_descriptor = open(KIC_DEVICE_PATH, O_RDWR);
	if (file_descriptor < 0) {
		printf("Device:%s inaccesible or missing. Error Code: %d\n", KIC_DEVICE_PATH, file_descriptor);
		exit(EXIT_FAILURE);
	}

	if(argc == 1)
		menu(file_descriptor);
	else
		if (!strcmp(argv[1], "count")) {
			retrieve_count(file_descriptor);
		} else if (!strcmp(argv[1], "reset")) {
			reset_count(file_descriptor);
		} else if (!strcmp(argv[1], "time")) {
			retrieve_time(file_descriptor);
		} else
			printf("Operation unsupported. Check spelling.\n");
	return 0;
}


int retrieve_count(int file_descriptor)
{
	int result = 0;
	unsigned int data_carry;
	result = ioctl(file_descriptor, KIC_GET_COUNT_QUERY, &data_carry);

	if (result < 0)
		printf("Retrieval Operation failed. Error Code: %d\n", result);
	else
		printf("Current keyboard interrupt count: %u\n", data_carry);

	return result;
}

int reset_count(int file_descriptor)
{
	int result = 0;
	result = ioctl(file_descriptor, KIC_RESET_QUERY);
	if (result < 0)
		printf("Reset Operation failed. Error Code: %d\n", result);
	else
		printf("Keyboard interrupt count has been reset.\n");

	return result;
}

int retrieve_time(int file_descriptor)
{
	int result = 0;
	long int data_carry;
	struct tm *time_structured;

	result = ioctl(file_descriptor, KIC_GET_TIME_QUERY, &data_carry);
	if (result < 0)
		printf("Retrieval Operation failed. Error Code: %d\n", result);
	else if (data_carry == -1)
		printf("Timer has not yet been reset.\n");
	else {
		// stripping time data off of nanoseconds
		data_carry = data_carry/1000000000;
		// conversion to local time (as per system settings)
		time_structured = localtime(&data_carry);
		// no '\n' since asctime provides it's own
		printf("Last reset time: %s", asctime(time_structured));
	}

	return result;
}

void menu(int desc)
{
	int flag = 1;
	printf("This is the control panel for keyboard interrupt counter module.\n\
Your options are:\n\
        > 1: retrieve current interrupt count.\n\
        > 2: reset interrupt count.\n\
        > 3: retrieve time and date of last reset.\n\
        > 0: leave.\n");
	do {
		printf("Choose option: ");
		scanf("%d", &flag);
		switch(flag) {
		case 1:
			retrieve_count(desc);
			break;
		case 2:
			reset_count(desc);
			break;
		case 3:
			retrieve_time(desc);
			break;
		case 0:
			printf("Goodbye.\n");
			break;
		default:
			printf("Invalid choice.\n");
			break;
		}
		printf("\n");
	} while (flag != 0);
}