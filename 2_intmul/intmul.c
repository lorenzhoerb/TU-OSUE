#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/wait.h>

struct values {
    char *a;
    char *b;
} values_t;

char *prg_name;

void handle_stdin(struct values *values);
void parse_str_to_hex(char *valStr, int *hex);
void mult_values(struct values *v);
void mult_hex(char *a, char *b, int *result);
void init_pipes(int pipes[8][2], int pid);
void add_hex_numbers(char *x, char *y, int offset);
int hex_char_to_int(char character);
void remove_zeros(char *str);
char int_to_hex_char(int i);
void exit_error(char *msg);
void usage(void);

/**
 * @file intmul.c
 * @author Lorenz Hoerburger <e12024737@student.tuwien.ac.at>
 * @date 12.12.2021 
 *
 * @brief Main program module.
 * This program multiplies two hexnumbers from stdin and outputs the result to stdout.
 * Both hex numbers must have the same length and the length must be even.
 * USAGE: %s
 */
int main(int argc, char **argv) {
    prg_name = argv[0];
    struct values values;

    if(argc != 1) {
        usage();
    }

    handle_stdin(&values);
    mult_values(&values);

    free(values.a);
    free(values.b);
    return 0;
}

/**
 * @brief multiplies two equal length hex numers
 * if the length is 1 then the hex string is multiplied immediately
 * otherwise create sub procresses and calculate result
 * 
 * @param v struct in which the hex values are safed
 */
void mult_values(struct values *v) {
    const int len = strlen(v->a);
    if(len == 1) {
        int result;
        mult_hex(v->a, v->b, &result);
        printf("%X\n", result);
        fflush(stdout);
        free(v->a);
        free(v->b);
        exit(EXIT_SUCCESS);
    }

    int hlen = len / 2;
    char ah[hlen+2];
    char al[hlen+2];
    char bh[hlen+2];
    char bl[hlen+2];

    for(int i=0; i < hlen; i++) {
        al[i] = v->a[i+hlen];
        ah[i] = v->a[i];
        bl[i] = v->b[i+hlen];
        bh[i] = v->b[i];
    }

    ah[hlen] = '\n';
    bh[hlen] = '\n';
    al[hlen] = '\n';
    bl[hlen] = '\n';

    ah[hlen+1] = '\0';
    bh[hlen+1] = '\0';
    al[hlen+1] = '\0';
    bl[hlen+1] = '\0';

    int pid[4], pipes[8][2], i;

    // open pipes
    for(i = 0; i < 8; i++) {
        if(pipe(pipes[i]) < 0)
            exit_error("open pipe failed");
    }

    for(i = 0; i < 4; i++) {
        pid[i] = fork();

        if(pid[i] < 0) {
            exit_error("fork failed");
        } else if(pid[i] == 0) {
            init_pipes(pipes, i);
            if(execlp(prg_name, prg_name, NULL) == -1)
                exit_error("execlp failed");
        } else {
            // parent
        }
    }

    // close reading ends
    close(pipes[0][0]);
    close(pipes[2][0]);
    close(pipes[4][0]);
    close(pipes[6][0]);

    // write 
    write(pipes[0][1], ah, strlen(ah));
    write(pipes[0][1], bh, strlen(bh));
    close(pipes[0][1]);

    write(pipes[2][1], ah, strlen(ah));
    write(pipes[2][1], bl, strlen(bl));
    close(pipes[2][1]);

    write(pipes[4][1], al, strlen(al));
    write(pipes[4][1], bh, strlen(bh));
    close(pipes[4][1]);

    write(pipes[6][1], al, strlen(al));
    write(pipes[6][1], bl, strlen(bl));
    close(pipes[6][1]);

    wait(NULL);
    wait(NULL);
    wait(NULL);
    wait(NULL);

    char res_hh[2 * len +2];
    char res_hl[2 * len +2];
    char res_lh[2 * len +2];
    char res_ll[2 * len +2];
    char endresult[4*len+2];

    for(int j = 0; j < len *4+1; j++){
            endresult[j] = '0';
        }
    endresult[len*4+1] = '\0';

    ssize_t size;

    close(pipes[1][1]);
    size = read(pipes[1][0], res_hh, sizeof(res_hh));
    res_hh[size-1] = '\0';
    close(pipes[1][0]);

    close(pipes[3][1]);
    size = read(pipes[3][0], res_hl, sizeof(res_hl));
    res_hl[size-1] = '\0';
    close(pipes[3][0]);

    close(pipes[5][1]);
    size = read(pipes[5][0], res_lh, sizeof(res_lh));
    res_lh[size-1] = '\0';
    close(pipes[5][0]);

    close(pipes[7][1]);
    size = read(pipes[7][0], res_ll, sizeof(res_ll));
    res_ll[size-1] = '\0';
    close(pipes[7][0]);

    add_hex_numbers(endresult, res_ll, 0);
    add_hex_numbers(endresult, res_hl, hlen);
    add_hex_numbers(endresult, res_lh, hlen);
    add_hex_numbers(endresult, res_hh, 2*hlen);

    remove_zeros(endresult);

    printf("%s\n", endresult);
}

/**
 * @brief Removes leading zeros of a string
 * 
 * @param str input string
 */
void remove_zeros(char *str) {
    char *p = str;
    while(p[0] == '0') {
        p++;
    }
    strcpy(str,p);
}

/**
 * @brief Parses a hex-char to a int
 * valid input chars [0-9a-zA-Z]
 * 
 * @param character hex-char
 * @return int, hex-char as int. Returns -1 if the the char couldn't be parsed
 */
int hex_char_to_int(char character)
{
	if (character >= '0' && character <= '9')
		return character - '0';
	if (character >= 'A' && character <= 'F')
		return character - 'A' + 10;
	if (character >= 'a' && character <= 'f')
		return character - 'a' + 10;
	return -1;
}

/**
 * @brief parses a int to a hex-char
 * 
 * 
 * @param i input int to be parsed to a hex char
 * @return char, parsed char.
 */
char int_to_hex_char(int i)
{
	if (i < 10)
		return '0' + i;
	else
		return 'A' + i - 10;
}

/**
 * @brief Sums up two string hex-numbers. The result is safed into x.
 * y can be offseted. 
 * 
 * @param x input hex and result output
 * @param y input hex
 * @param offset int, y is offsetted to "the left"
 */
void add_hex_numbers(char *x, char *y, int offset) {
    int x_pos = strlen(x) - 1;
    int y_pos = strlen(y) - 1;
    char out_hex[x_pos+y_pos+2];
    int x_hex = 0, y_hex = 0, result = 0, overflow = 0, w_pos = 0, tmp_result = 0;
    memset(out_hex, '\0', sizeof(char)*strlen(out_hex));
    

    while(x_pos >= 0 || y_pos >= 0) {
        tmp_result = 0;
        if (x_pos >= 0) {
            x_hex = hex_char_to_int(x[x_pos]);
            tmp_result += x_hex;
            x_pos--;
        }

        if (y_pos >= 0 && offset <= 0) {
            y_hex = hex_char_to_int(y[y_pos]);
            tmp_result += y_hex;
            y_pos--;
        }
        tmp_result += overflow;
        overflow = tmp_result / 16;
        result = tmp_result % 16;
        out_hex[w_pos++] = int_to_hex_char(result);
        offset--;
    }

    out_hex[w_pos] = '\0';

    memset(x, '\0', sizeof(char)*strlen(out_hex));

    // reverse out_hex and put it into a
    int i;
    for(i = strlen(out_hex)-1; i >= 0; i--) {
       x[strlen(out_hex)-1-i] = out_hex[i];
    }
}

/**
 * @brief Inits the pipes used by child process. 
 * Each child process is assigned with a read and a write pipe. 
 * If childId 0: read pipe = pipes[0], wirte pipe = pipe[1]
 * stdin is redirected to read pipe
 * stdo is redirected to write pipe
 * 
 * @param pipes read and write pipes
 * @param pid childID
 */
void init_pipes(int pipes[8][2], int pid) {
    pid *= 2;
    // stdin -> read
    close(pipes[pid][1]);
    if(dup2(pipes[pid][0], STDIN_FILENO) == -1)
        exit_error("read dup failed");
    

    // stdout -> write
    close(pipes[pid+1][0]);
    if(dup2(pipes[pid+1][1], STDOUT_FILENO) == -1)
        exit_error("write dup failed");

    // close all other open pipes
    int i;
    for (i = 0; i < 8; i++) {
        if(i != pid && i != pid +1) {
            close(pipes[i][0]);
            close(pipes[i][1]);
        }
    }
}

/**
 * @brief Multiplies two string hex numbers and safes
 * the result into result.
 * 
 * @param a hex number
 * @param b hex number
 * @param result result of multiplication as int representing a hex-char
 */
void mult_hex(char *a, char *b, int *result) {
    int hexA, hexB;
    parse_str_to_hex(a, &hexA);
    parse_str_to_hex(b, &hexB);
    *result = hexA * hexB;
}

/**
 * @brief Parses a string into an integer and safes the result into hex
 * 
 * @param valStr hex nubmer
 * @param hex 
 */
void parse_str_to_hex(char *valStr, int *hex) {
    int hexnum;
    if((hexnum = strtol(valStr, NULL, 16)) == 0 && (errno == EINVAL || errno == ERANGE)) {
        fprintf(stderr, "ERROR: HEX input error: %s\n", strerror(errno));
        exit(EXIT_FAILURE);
    }
    *hex = hexnum;
}

/**
 * @brief This function handles stdin. It reads the 
 * first two lines of stdin. Validates if the the lines
 * are hex-numbers and if the string length is the same and even.
 * Safes the validated hexnumbers into the helper struct values.
 * 
 * @param values struct, where the hex numbers are safed
 */
void handle_stdin(struct values *values) {
    char *line = NULL;
    size_t size = 0;
    ssize_t linelen;
    int prevlen = -1;
    int isSetA = 0;
    int isSetB = 0;
    int lineCount = 0;

    while((linelen = getline(&line, &size, stdin)) > 0) {
        lineCount++;

        if(line[linelen-1] == '\n') {
            line[linelen-1] = '\0';
        }

        int lineLen = strlen(line);

        if(prevlen != -1 && prevlen != lineLen) {
            exit_error( "Inputs don't have same length");
        }

        if(lineLen % 2 != 0 && lineLen != 1) {
            exit_error("numbler length must be even");
        }

        prevlen = strlen(line);

        if (!isSetA) {
            values->a = malloc(lineLen+1);
            strcpy(values->a, line);
            isSetA = 1;
        } else if (!isSetB) {
            values->b = malloc(lineLen+1);
            strcpy(values->b, line);
            isSetB = 1;
        } else {
            break;
        }

        if(lineCount == 2) {
            break;
        }
    }

    if(lineCount != 2) {
        exit_error("stdin needs two input lines");
    }

    free(line);
}

/**
 * @brief Prints an erreor and exits
 * 
 * @param msg, error message
 */
void exit_error(char *msg) {
    fprintf(stderr, "%s\n", msg);
    exit(EXIT_FAILURE);
}

/**
 * @brief Prints the usage of the program and exits
 * 
 */
void usage(void) {
    fprintf(stderr, "USAGE: %s", prg_name);
    exit(EXIT_FAILURE);
}