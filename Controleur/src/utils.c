#include "utils.h"
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <sys/time.h>

// Function to trim leading and trailing whitespaces and '\n' from a string
void trim(char* str) {
    if (str == NULL) {
        return;
    }

    int start = 0;
    int end = strlen(str) - 1;

    // Remove leading whitespaces and newline characters
    while (start <= end && (isspace((unsigned char)str[start]) || str[start] == '\n')) {
        start++;
    }

    // Remove trailing whitespaces and newline characters
    while (end >= start && (isspace((unsigned char)str[end]) || str[end] == '\n')) {
        end--;
    }

    // Shift the string to remove leading whitespaces and newline characters
    memmove(str, str + start, end - start + 1);
    str[end - start + 1] = '\0';
}

void rm_comma(char* str) {
    if (str == NULL) {
        return;
    }

    char* comma = strchr(str, ',');
    if (comma != NULL) {
        *comma = '\0';  // Replace the first comma with a null terminator
    }
}

microseconds_t get_time_usec() {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (microseconds_t)tv.tv_sec * 1000000LL + tv.tv_usec;
}
