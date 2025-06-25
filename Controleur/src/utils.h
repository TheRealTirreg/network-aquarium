#ifndef UTILS_H
#define UTILS_H

typedef long long microseconds_t;

typedef struct {
    int x;
    int y;
} Tuple;

void trim(char* str);

// e.g. "hello, world" -> "hello"
void rm_comma(char* str);

// Gets current time in microseconds
microseconds_t get_time_usec();

#endif // UTILS_H
