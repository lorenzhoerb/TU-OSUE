/**
 * @file client.c
 * @date 16.01.2022
 *
 * @brief Sends a HTTP request to a server and receives the response. 
 * If the option -o or -d is not set the response content will be printed to stdout.
 * If the status code of the response is not 200, the server will print the status code to stdout.
 * -p PORT: if -p is not set the default port is 80
 * -o FILE: The response will be saved to FILE
 * -d DIR:	The response file will be saved to the directory DIR 
 * 	  URL: URL to webserver
 *			valid url: http://www.HOSTNAME/[FILE_PATH]
 * USAGE: [-p PORT] [ -o FILE | -d DIR ] URL
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include "common.h"

#define PROT "HTTP/1.1"

typedef struct {
	char *url;
	unsigned int port;
	FILE *f;
} options_t;

options_t opts = {NULL, 80, NULL};

static void handle_options(int argc, char **argv);
static FILE *open_file(const char *file_path);
static FILE *connect_to_server(void);
static void url_get_host(char *url, char *host);
static void url_get_file_path(char *url, char *filePath);
static bool url_is_valid(const char *url);
static char *url_get_file_name(char *url);
static int parse_response_status(char *h_line);
static bool header_is_valid(char *header);

/**
 * @brief Validates program options and initializes options_t struct. 
 * Allowed options: [-p PORT] [ -o FILE | -d DIR ] URL
 * -p PORT:	Must be an integer
 * -o FILE:	Filename where response is going to be saved, standard is stdout
 * -d DIR:	Directory where response file is going to be saved
 * 	  URL:	URL to webserver
 * 
 * @param argc argument counter 
 * @param argv argument vector
 */
static void handle_options(int argc, char **argv) {
	char c;
	int pFlag = 0, dFlag = 0, oFlag = 0;
	char *port = NULL, *url = NULL, *outputFilePath = NULL;

	while((c = getopt(argc, argv, "p:o:d:")) != -1) {
		switch (c) {
			case 'p':
				pFlag++;
				port = optarg;
				break;
			case 'o':
				oFlag++;
				outputFilePath = optarg;
				break;
			case 'd':
				dFlag++;
				outputFilePath = optarg;
				break;
			default:
				usage();
				exit(EXIT_FAILURE);
		}
	}

	// handle url
	if(argv[optind] != NULL && (argc - optind) == 1 ) {
		url = argv[optind];

		if(!url_is_valid(url)) 
			error_exit("invalid url");

		opts.url = url;
	} else {
		usage_exit();
	}

	// handle port
	if(pFlag > 1)
		usage_exit();
	else if (pFlag == 1)
		opts.port = parse_port(port);
	else
		;

	// handle output file
	if((dFlag == 1 || oFlag == 1) && outputFilePath == NULL) {
		usage_exit();
	}

	if((dFlag == 1 && oFlag == 0)) {
		char *fileName = NULL;
		fileName = url_get_file_name(url);
		chdir(outputFilePath);
		opts.f = open_file(fileName);
	} else if(dFlag == 0 && oFlag == 1) {
		opts.f = open_file(outputFilePath);
	} else if(dFlag == 0 && oFlag == 0) {
		opts.f = stdout;
	} else {
		usage_exit();
	}

	printf("URL: %s, PORT: %d\n", opts.url, opts.port);
}

/**
 * @brief Checks if a url is valid. A url is valid if it starts with http://www.HOSTNAME/[filepath]
 * EXAMPLES: 
 * 	http://www.google.at/  -> valid
 * 	http://www.localhost/test.html  -> valid
 * 	http://www.localhost/test.html  -> valid
 * 	http://localhost/test.html  	-> invalid
 * 
 * @param url
 * @return true if url is valid
 */
static bool url_is_valid(const char *url) {
	char *p = strstr(url, "http://www.");
	if(p != url || p == NULL) {
		return false;
	}

	if(strlen(url) >= 13 && p++ != '\0')	{
		p = strchr(p, '/');
		if(p == NULL)
			return false;
		else
			return true;
	} else {
		return false;
	}
}

/**
 * @brief Extracts the filename of an url.
 * If the URL ends with '/' the default filname: index.html is returned
 * The program terminates if the URL is invalid. 
 * @param url 
 * @return Pointer to filename 
 */
static char *url_get_file_name(char *url) {
	char *p = url;
	char *outURL = NULL;
	p += strlen(url)-1;
	if(*p == '/') {
		outURL = "index.html";
	} else {
		while(*p != '/') {
			if(p == url) {
				error_exit("invalid URL");
			}
			p--;
		}
		p++;
		outURL = p;
	}

	return outURL;
}

/**
 * @brief Extracts the host of a valid url and saves it into the parameter host.
 * The program terminates if the URL is invalid
 * EXAMPLE: http://www.test.com/ returns www.test.com
 * 
 * @param url input url 
 * @param host extracted host
 */
static void url_get_host(char *url, char *host) {
	char *hp_begin = strpbrk(url, "www.");
	if(hp_begin == NULL) 
		error_exit("invalid url");
	char *hp_end = strchr(hp_begin, '/');
	if(hp_end == NULL) 
		error_exit("invalid url");
	strncpy(host, hp_begin, hp_end - hp_begin);
 	host[hp_end - hp_begin] = '\0';
}

/**
 * @brief Extracts the filepath of a valid url.
 * The program terminates if the URL is invalid. 
 * EXAMPLE: 
 *		http://www.test.com/test/index.html returns /test/index.html
 *		http://www.test.com/ returns /
 * @param url input url 
 * @param filePath extracted filepath
 */
static void url_get_file_path(char *url, char *filePath) {
	char *fp_begin = strpbrk(url, "www.");
	if(fp_begin == NULL) 
		error_exit("invalid url");
	fp_begin = strchr(fp_begin, '/');
	if(fp_begin == NULL) 
		error_exit("invalid url");
	strcpy(filePath, fp_begin);
}

/**
 * @brief Opens a file in write mode and returns the pointer to the fail
 * Throws an error if the file couldn't be opened
 * 
 * @param file_path path to file 
 * @return FILE* pointer to the opened file
 */
static FILE *open_file(const char *file_path) {
	FILE *returnFile = fopen(file_path, "w");

    if (returnFile == NULL)
    {
        fprintf(stderr, "%s: fopen failed: %s\n", progname, strerror(errno));
        exit(EXIT_FAILURE);
    }
    return returnFile;
}

/**
 * @brief Connects to the server specified in the global opts struct.
 * Throws an error if connection fails
 * 
 * @return FILE* socketfile
 */
static FILE *connect_to_server(void) {
	struct addrinfo hints, *ai;
	memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;

	char port[5];
	char host[strlen(opts.url)];

	sprintf(port, "%d", opts.port);
	url_get_host(opts.url, host);
	char *p = host + 4; //skips www.
	int res = getaddrinfo(p, port, &hints, &ai);

	if(res != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(res));
		error_exit("getaddrinfo failed");
	}

	int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

	if(sockfd < 0) {
		fprintf(stderr, "Failed to create socket: %s\n", strerror(errno));
		exit(EXIT_FAILURE);
	}

	FILE *server = fdopen(sockfd, "r+");

	if(server == NULL)
		error_exit("Couldn't open serverfd");

	if (connect(sockfd, ai->ai_addr, ai->ai_addrlen) < 0)
		error_exit("Failed to connect to server");

	return server;
}

/**
 * @brief Sends a message to a server via socket connection
 * 
 * @param serverStream socket file of server
 * @param request message to be sent to serverStream
 */
static void send_request(FILE *serverStream, const char *request) {
	if(fputs(request, serverStream) < 0)
		error_exit("failed to send request");
	fflush(serverStream);
}

/**
 * @brief Tries to receive a response from the server.
 * Response header is going to be saved to **header
 * Response content is going to be saved to **content
 * Memory for **header and **content will be allocated. Need to be freed.
 * Throws an Error: "Protocol error" if the response is not valid
 * 
 * @param serverStream server socket file
 * @param header response header will be saved here
 * @param content response content will be saved here
 */
static void recv_response(FILE *serverStream, char **header, char **content) {
	char buf[1024];
	bool firstLine = true, l_header = true;
	size_t sz_header = 0, sz_content = 0;
	while (fgets(buf, sizeof(buf), serverStream) != NULL) {
		if (firstLine) {
			if (!header_is_valid(buf))
				error_exit("Protocol error");
			firstLine = false;
		}

		if(l_header) {
			if(strcmp(buf, "\r\n") == 0) // end of header
				l_header = false; 
			else
				sz_header = dyn_cat(header, buf, sz_header);
		} else {
			sz_content = dyn_cat(content, buf, sz_content);
		}
	}
}

/**
 * @brief Takes a valid response header and returns the status code of the response
 * 
 * @param header valid response header
 * @return int status code of response
 */
static int parse_response_status(char *header) {
	int status = 0;
	char *tmp = strdup(header);
	char *str_status = strtok(tmp, " ");
	str_status = strtok(NULL, " ");

    if((status = strtol(str_status, NULL, 10)) == 0 && (errno == EINVAL || errno == ERANGE)) {
        fprintf(stderr, "%s ERROR: status couldn't be parsed: %s\n", progname, strerror(errno));
        exit(EXIT_FAILURE);
    }

	return status;
}

/**
 * @brief Checks if a response header is valid
 * The header is invalid if it doesn't start with PROT (HTTP/1.1)
 * or the status couldn't be parsed
 * 
 * @param header response header
 * @return true 	response header is valid
 * @return false 	response header is invalid
 */
static bool header_is_valid(char *header) {
	char *tmp = NULL;
	tmp = strdup(header);

	if(strncmp(tmp, PROT, strlen(PROT)) != 0) 
		return false;

	char *str_status = strtok(tmp, " ");
	str_status = strtok(NULL, " ");

	// checks if status is int
    if(strtol(str_status, NULL, 10) == 0 && (errno == EINVAL || errno == ERANGE))
        return false;

	return true;
}

/**
 * @brief Sends a HTTP request to a server and receives the response. 
 * If the option -o or -d is not set the response content will be printed to stdout.
 * If the status code of the response is not 200, the server will print the status code to stdout.
 * -p PORT: if -p is not set the default port is 80
 * -o FILE: The response will be saved to FILE
 * -d DIR:	The response file will be saved to the directory DIR 
 * 	  URL: URL to webserver
 *			valid url: http://www.HOSTNAME/[FILE_PATH]
 * USAGE: [-p PORT] [ -o FILE | -d DIR ] URL
 * 
 * @param argc argument count
 * @param argv argument vector
 * @return int return status 
 */
int main(int argc, char **argv) {
	progname = argv[0];
	usage_msg = "[-p PORT] [ -o FILE | -d DIR ] URL\n";
	handle_options(argc, argv);

	FILE *server = connect_to_server();

	char *req = NULL;
	char host[strlen(opts.url)];
	char filePath[strlen(opts.url)];

	url_get_host(opts.url, host);
	url_get_file_path(opts.url, filePath);

	create_req_header("GET", filePath, PROT, &req);
	add_opt_header("Host", host, &req);
	add_opt_header("Connection", "close", &req);
	end_header(&req);

	// request 
	send_request(server, req);
	free(req);

	// response
	char *header = NULL, *response = NULL;
	recv_response(server, &header, &response);

	int status = parse_response_status(header);

	if(status == 200) {
		fputs(response, opts.f);
	} else {
		print_status(status);
	}

	free(header);
	free(response);

	if(opts.f != stdout && opts.f != NULL)
		fclose(opts.f);

	if(server != NULL)
		fclose(server);

	return 0;
}
