extern const char *progname;
extern const char *usage_msg;

unsigned int parse_port(const char *port);
void get_rfc822_date(char *date);
void get_status(int status, char *description);
size_t dyn_cat(char **buf, const char *str2, size_t buf_size);
void create_header(const char *header, char **request);
void create_req_header(const char *methode, const char *filePath, const char *prot, char **request);
void add_header(char *header, char **request);
void add_opt_header(char *key, char* value, char **request);
void end_header(char **request);
void print_status(int status);
void error_exit(const char *msg);
void usage(void);
void usage_exit(void);