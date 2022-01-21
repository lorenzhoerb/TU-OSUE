/**
 * @file server.c
 * @date 16.01.2022
 *
 * @brief Validates program options and initializes options struct. 
 * Allowed options: [-p PORT] [-i INDEX] DOC_ROOT
 * -p PORT:     Can be used to specify the port on which the server shall listen 
 *              for incoming connections.
 *              If this option is not used the port defaults to 8080 
 * -i INDEX:    Is used to specify the index filename, i.e. the file 
 *              which the server shall attempt to
 *              transmit if the request path is a directory. 
 *              The default index filename is index.html 
 * 	  DOC_ROOT:	Is the path of the document root directory, 
 *              which contains the files that can be
 *              requested from the server. 
 * USAGE: [-p PORT] [-i INDEX] DOC_ROOT 
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
#include <signal.h>
#include "common.h"

typedef struct {
    unsigned int port;
    char *index;
    char *doc_root;
} options;

typedef struct {
    int status;
    int c_length;
    FILE *f;
} resp_t;

options opts = {8080, NULL, NULL};
resp_t response = {200, 0, NULL};

volatile sig_atomic_t quit = 0;

/**
 * @brief Validates program options and initializes options struct. 
 * Allowed options: [-p PORT] [-i INDEX] DOC_ROOT
 * -p PORT:     Can be used to specify the port on which the server shall listen 
 *              for incoming connections.
 *              If this option is not used the port defaults to 8080 
 * -i INDEX:    Is used to specify the index filename, i.e. the file 
 *              which the server shall attempt to
 *              transmit if the request path is a directory. 
 *              The default index filename is index.html 
 * 	  DOC_ROOT:	Is the path of the document root directory, 
 *              which contains the files that can be
 *              requested from the server. 
 * @param argc argument counter 
 * @param argv argument vector
 */
static void handle_options(int argc, char **argv) {
    char c;
    int pFlag = 0, iFlag = 0;
    char *port = NULL, *index = NULL;

    while((c = getopt(argc, argv, "p:i:")) != -1) {
        switch (c) {
            case 'p':
                pFlag++;
                port = optarg;
                break;
            case 'i':
                iFlag++;
                index = optarg;
                break;
            default:
                usage();
                exit(EXIT_FAILURE);
        }
    }

    // handle option p
    if(pFlag == 0) {
        ;
    } else if (pFlag == 1) {
        if(port != NULL) {
            opts.port = parse_port(port);
        } else {
            usage();
        error_exit("-p option needs an argument");
        }
    } else {
        usage();
        error_exit("-p option can only be set once");
    }

    // handle option i
    if(iFlag == 0) {
        opts.index = "index.html";
    } else if (iFlag == 1) {
        if(index != NULL) {
            opts.index = index;
        } else {
            usage();
            error_exit("-i options needs an argument");
        }
    } else {
        usage();
        error_exit("-p option can only be set once");
    }

    // handle doc_root
    if(argv[optind] != NULL && (argc - optind) == 1 ) {
		opts.doc_root = argv[optind];
	} else {
		usage();
        error_exit("DOC_ROOT must be set");
	}
    printf("PORT: %d, INDEX: %s, DOC_ROOT: %s\n", opts.port, opts.index, opts.doc_root);
}

/**
 * @brief Set the up the server socket with the specified 
 *        parameters in opts.
 * 
 * @return int retruns the socked fiel descriptor 
 */
static int setup_socket(void) {
    struct addrinfo hints, *ai;
    memset(&hints, 0 ,sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    char port[5];
    sprintf(port, "%d", opts.port);
    int res = getaddrinfo(NULL, port, &hints, &ai);
    if(res != 0)
		error_exit("getaddrinfo failed");

    int sockfd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);

    if(sockfd < 0) {
        error_exit("couldn't create socket");
    }

    int optval = 1;
    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof optval);

    if(bind(sockfd, ai->ai_addr, ai->ai_addrlen) < 0) {
        error_exit("couldn't bind socket");
    }

    if(listen(sockfd, 1) < 0)
        error_exit("socket: listen failed");
    freeaddrinfo(ai);

    return sockfd;
}

/**
 * @brief Gets the absolute file path to the requested file. 
 *        Allocates memory for absolutePath and stores the
 *        absolute file path into it.
 * @param filePath filePath of requested file. examples: /, /test/test.html
 * @param doc_root document root of the server
 * @param absolutePath variable where the absolute filePath will be stored
 */
static void get_absolute_file_path(const char *filePath, char *doc_root, char **absolutePath) {
    char *symlinkpath = doc_root;
    char t_absolutepath[300];
    size_t sz = 0;

    realpath(symlinkpath, t_absolutepath);
    
    sz = dyn_cat(absolutePath, t_absolutepath, sz);
    sz = dyn_cat(absolutePath, filePath, sz);

    if(filePath[strlen(filePath)-1] == '/') {
        sz = dyn_cat(absolutePath, opts.index, sz);
    } 
}

/**
 * @brief Opends the requested file
 * 
 * @param req_file request file path example: /, /test/abc.html
 * @return FILE* a pointer to the requested file
 */
static FILE *get_req_file(const char *req_file) {
    char *abs_filePath = NULL;
    get_absolute_file_path(req_file, opts.doc_root, &abs_filePath);
    printf("%s\n", abs_filePath);
    FILE *f = fopen(abs_filePath, "r");

    if(abs_filePath != NULL)
        free(abs_filePath);

    return f;
}

/**
 * @brief Tries to open the request file and sets it to the response struct. 
 * If the file does not exist on the server. The response status code 404 (file not found) is set
 * 
 * @param filePath request file path
 */
static void on_GET(const char *filePath) {
        FILE *f = get_req_file(filePath);
        if (f == NULL) {
            response.status = 404; // file not found
        } else {
            response.f = f;
        }
}

/**
 * @brief Reads a file to a buffer. Allocates memory for buf. buf must be freed
 * 
 * @param f file to read from
 * @param buf buffer where the file is saved
 */
static void readFileToBuf(FILE *f, char **buf) {
    char line[1024];
    size_t sz = 0;
	while (fgets(line, sizeof(line), f) != NULL) {
        sz = dyn_cat(buf, line, sz);
	}
}

/**
 * @brief Handles all implemented request methodes. If a methdoeis not implemented the 
 *        response code 501 (Not implemented) is set. 
 * 
 * @param type request methode type exampes: GET, POST, UPDATE, ...
 * @param filePath 
 */
static void handle_req_methode(char *type, const char *filePath) {
    if(strcmp(type, "GET") == 0) {
        on_GET(filePath);
    } else {
        response.status = 501; // Not implemented
    }
}

/**
 * @brief Validates if the request header is valid. 
 *        The request header is valid if the first line 
 *        ends with HTTP/1.1\r\n
 * 
 * @param header request header
 * @return true if the request header is valid
 * @return false  if the request header is invalid
 */
static bool header_is_valid(const char *header) {
    char tmp[strlen(header)];
    strcpy(tmp, header);
	strtok(tmp, " ");
	strtok(NULL, " ");
	char *prot = strtok(NULL, " ");

    if(strcmp(prot, "HTTP/1.1\r\n") == 0)
        return true;
    else
        return false;
}

/**
 * @brief Tries to send a response message to the client.
 * The message contains the response header. If a response file 
 * is set in the response struct, the content of it will be sent.
 * 
 * @param connfile client socket file
 */
static void send_response(FILE *connfile) {
    char date[200];
    char status_desc[100];
    int content_length = 0;
    get_rfc822_date(date);
    get_status(response.status, status_desc);

    // get content length
    if(response.f != NULL) {
        fseek(response.f, 0L, SEEK_END);
        content_length = ftell(response.f);
        fseek(response.f, 0L, SEEK_SET);
    }

    // send header
    fprintf(connfile, "HTTP/1.1 %d %s\r\n", response.status, status_desc);
    fprintf(connfile, "Date: %s\r\n", date);
    fprintf(connfile, "Content-Length: %d\r\n", content_length);
    fprintf(connfile, "Connection: close\r\n\r\n");

    // send file content
    if (response.f != NULL) {
        char *buf = NULL;
        readFileToBuf(response.f, &buf);
        fputs(buf, connfile);
        fflush(connfile);
        fclose(response.f);
        free(buf);
    }
}

/**
 * @brief Server tries to accept a client and returns the client socket file.
 * Resets response struct with default values.
 * 
 * @param sockfd server socket
 * @return FILE* Returns the client socket fail if the connection succeeded. 
 *         Returns null if accept fails.
 */
static FILE *accept_client(int sockfd) {
    int connfd = accept(sockfd, NULL, NULL);
    if(connfd < 0) {
        fprintf(stderr, "%s ERROR: socket: accept failed\n", progname);
        return NULL;
    }

    FILE *connfile = fdopen(connfd, "r+");

    if(connfile == NULL)
        error_exit("Couldn't open connfd file");

    response.status = 200;
    response.f = NULL;

    return connfile;
}

/**
 * @brief Tries to receives a request message and stores it into **request. 
 * Allocates memory for **request, which must be freed afertwards. 
 * 
 * @param clientStream client socket file 
 * @param request request buffer where the request message is stored
 */
static void recv_request(FILE *clientStream, char **request) {
    char buf[1024];
	bool firstLine = true;
	size_t sz_req = 0;
	while (fgets(buf, sizeof(buf), clientStream) != NULL) {
		if (firstLine) {
			if (!header_is_valid(buf))
                response.status = 400; // Bad Request
			firstLine = false;
		}
        sz_req = dyn_cat(request, buf, sz_req);

        if(strcmp(buf, "\r\n") == 0) // client didnt close send stream
            break;
	}
}

/**
 * @brief Parese the request header and sets request methode and request filePath.
 * Allocates memory for methode and filePath (must be freed afterwards).
 * 
 * @param header 
 * @param methode 
 * @param filePath 
 * @return int returns 0 success and -1 on failure
 */
static int parse_header(char *header, char **methode, char **filePath) {
    char tmp[strlen(header)];
    strcpy(tmp, header);
	char *t_methode = strtok(tmp, " ");
	char *t_filePath = strtok(NULL, " ");

    if(t_methode == NULL || t_filePath == NULL) {
        return -1;
    }

    int sz_m = 0, sz_f = 0;

    sz_m = dyn_cat(methode, t_methode, sz_m);
    dyn_cat(methode, "\0", sz_m);
    sz_f = dyn_cat(filePath, t_filePath, sz_f);
    dyn_cat(filePath, "\0", sz_f);
    return 0;
}

/**
 * @brief Handles signal and sets quit to 1
 * 
 * @param signal 
 */
static void handle_signal(int signal) {
    quit = 1;
}

/**
 * @brief Initializes sigaction on SIGINT and SIGTERM with hand_signal 
 * 
 * @param sa struct sigaction
 */
static void init_signal(struct sigaction *sa) {
    memset(sa, 0, sizeof(*sa));
    sa->sa_handler = handle_signal;
    sigaction(SIGINT, sa, NULL);
    sigaction(SIGTERM, sa, NULL);
}

/**
 * @brief Validates program options and initializes options struct. 
 * Allowed options: [-p PORT] [-i INDEX] DOC_ROOT
 * -p PORT:     Can be used to specify the port on which the server shall listen 
 *              for incoming connections.
 *              If this option is not used the port defaults to 8080 
 * -i INDEX:    Is used to specify the index filename, i.e. the file 
 *              which the server shall attempt to
 *              transmit if the request path is a directory. 
 *              The default index filename is index.html 
 * 	  DOC_ROOT:	Is the path of the document root directory, 
 *              which contains the files that can be
 *              requested from the server. 
 * @param argc argument counter 
 * @param argv argument vector
 */
int main(int argc, char **argv) {
    progname = argv[0];
    usage_msg = "[-p PORT] [-i INDEX] DOC_ROOT";
    handle_options(argc, argv);

    struct sigaction sa;
    init_signal(&sa);

    int sockfd = setup_socket();
    char *req_header;
    char *methode, *filePath;

    while (!quit) {
        FILE *client = accept_client(sockfd);
        if(client == NULL) break;

        req_header = NULL;
        methode = NULL;
        filePath = NULL;

        recv_request(client, &req_header);

        if (parse_header(req_header, &methode, &filePath) == 0) {
            char tmp_methode[5];
            strcpy(tmp_methode, methode);
            handle_req_methode(tmp_methode, filePath);
        } else {
            response.status = 400;
        }

        send_response(client);

        if (client != NULL)
            fclose(client);
            
        if(methode != NULL)
            free(methode);

        if(filePath != NULL)
            free(filePath);
    }

    close(sockfd);

    if(req_header != NULL)
        free(req_header);

    return 0;
}
