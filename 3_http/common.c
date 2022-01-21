/**
 * @file common.c
 * @date 16.01.2022
 *
 * @brief Common function for server.c and client.c
 */
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include "common.h"

const char *progname = NULL;
const char *usage_msg = NULL;
int req_sz = 0;

/**
 * @brief Prints status with description to stdout
 * 
 * @param status response status
 */
void print_status(int status) {
	char description[100];
	get_status(status, description);
	printf("STATUS: %d %s\n",status, description);
}

/**
 * @brief Saves to current date-time to date in rfc822 format
 * 
 * @param date current date-time
 */
void get_rfc822_date(char *date) {
    char outstr[200];
    time_t t;
    struct tm *tmp;
    t = time(NULL);
    tmp = localtime(&t);
    if(tmp == NULL) {
        error_exit("localtime");
    }
    if(strftime(outstr, sizeof(outstr), "%a, %d %b %y %T %Z", tmp) == 0) {
        fprintf(stderr, "strftime returned 0\n");
        exit(EXIT_FAILURE);
    }

    strcpy(date, outstr);
}

/**
 * @brief Stores the description of the response status in the parameter descripteion
 * If status isn't known "no description" set
 * 
 * @param status response status
 * @param description resolved status
 */
void get_status(int status, char *description) {
	char *status_msg;
	switch(status) {
		case 200:
			status_msg = "OK";
			break;
		case 308:
			status_msg = "Permanent Redirect";
			break;
		case 400:
			status_msg = "Bad Request";
			break;
		case 501:
			status_msg = "Not implemented";
			break;
		case 404:
			status_msg = "Not Found";
			break;
		default:
			status_msg = "no description";
	}
	strcpy(description, status_msg);
}

/**
 * @brief Tries to parse string to a valid port.
 * A port must be between 0 and 65535 to be able to be parsed.
 * If port couldn't be parsed the program terminates with an error message.
 * 
 * @param port 
 * @return unsigned int parsed port
 */
unsigned int parse_port(const char *port) {
    int o_port;
    if((o_port = strtol(port, NULL, 10)) == 0 && (errno == EINVAL || errno == ERANGE)) {
        fprintf(stderr, "%s ERROR: port couldn't be parsed: %s\n", progname, strerror(errno));
        exit(EXIT_FAILURE);
    }

    if(o_port < 0) {
        error_exit("Port could not be parsed: negative port error");
    }

	if(o_port > 65535) {
        error_exit("Port could not be parsed: port must be smaller then 65536");
	}

	return o_port;
}

/**
 * @brief Dynamically concatinates two strings.
 * str2 will be added to buf
 * Returns new size of buffer.
 * 
 * @param buf pointer to a buffer where str2 is added
 * @param str2 string to add to buff
 * @param buf_size current buffer size
 * @return size_t buffer size after concatenation
 */
size_t dyn_cat(char **buf, const char *str2, size_t buf_size) {
	if (*buf == NULL) {
		*buf = malloc(strlen(str2)+1);
		if(*buf == NULL) {
			error_exit("allocationg buf failed");
		} else {
			buf_size = 0;
			strcpy(*buf, str2);
		}
	} else {
		*buf = realloc(*buf, strlen(str2) + buf_size + 1);
		if(*buf == NULL) {
			error_exit("allocationg buf failed");
		} else {
			strcat(*buf, str2);
		}
	}
	return buf_size + strlen(str2);
}

/**
 * @brief Dynamically creats the first line of a request / response header
 * Memory for **request will be allocated. **request must be freed
 * 
 * @param header header line to add to the reqest
 * @param request request buffer
 */
void create_header(const char *header, char **request) {
	req_sz = dyn_cat(request, header, req_sz);
	req_sz = dyn_cat(request, "\r\n", req_sz);
}

/**
 * @brief Dynamically creates the first line of a request header
 * Memory for **request will be allocated. **request must be freed
 * 
 * EXAMPLE: GET / HTTP/1.1
 * 
 * @param methode request methode
 * @param filePath request file path
 * @param prot request protocol 
 * @param request request buffer
 */
void create_req_header(const char *methode, const char *filePath, const char *prot, char **request) {
	size_t sz = strlen(methode) + strlen(filePath) + strlen(prot) + 5;
	char header[sz];
	sprintf(header, "%s %s %s", methode, filePath, prot);
	create_header(header, request);
}

/**
 * @brief Dynamically adds a header line to a request / response header
 * Memory for **request will be allocated. **request must be freed
 * 
 * @param header header line to add to the reqest
 * @param request request buffer
 */
void add_header(char *header, char **request) {
	req_sz = dyn_cat(request, header, req_sz);
	req_sz = dyn_cat(request, "\r\n", req_sz);
}

/**
 * @brief Dynamically adds a header line as key value pair.
 * EXAMPLE: key...Connection	value... close	-> header line: Connection: close
 * 
 * @param key request header line key
 * @param value request header line value
 * @param request request buffer
 */
void add_opt_header(char *key, char *value, char **request) {
	char header[strlen(key) + strlen(value) + 2];
	sprintf(header, "%s: %s", key, value);
	add_header(header, request);
}

/**
 * @brief End a request header by appendin "\r\n"
 * 
 * @param request request buffer
 */
void end_header(char **request) {
	req_sz = dyn_cat(request, "\r\n", req_sz);
}

/**
 * @brief Prints msg to stderr and exits with EXIT_FAILURE
 * The global variable progname must be set bevore calling this methode.
 *
 * @param msg error message
 */
void error_exit(const char *msg) {
    if(progname == NULL)
        fprintf(stderr, "ERROR: progname not set");
	fprintf(stderr, "%s ERROR: %s\n", progname, msg);
	exit(EXIT_FAILURE);
}

/**
 * @brief Prints the usage of the current program to stderr.
 * The global variables progname and usage_msg must be set bevore calling this methode.
 */
void usage(void) {
    if(progname == NULL) {
        fprintf(stderr, "ERROR: progname not set\n");
        exit(EXIT_FAILURE);
    }
    if(usage_msg == NULL) {
        fprintf(stderr, "ERROR: usage_msg not set\n");
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "%s %s", progname, usage_msg);
}

/**
 * @brief Prints the usage of the current program to stderr and exits with EXIT_FAILURE.
 * The global variables progname and usage_msg must be set bevore calling this methode.
 */
void usage_exit(void) {
	usage();
	exit(EXIT_FAILURE);
}


