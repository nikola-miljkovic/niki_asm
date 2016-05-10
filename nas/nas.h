#ifndef NIKI_ASM_NAS_H
#define NIKI_ASM_NAS_H

#include <stdlib.h>

#define ASSERT_AND_EXIT(condition, message, ...) if (condition) { printf(message, ##__VA_ARGS__); exit(0); }
#define ASSERT(condition, message, ...) if (condition) { printf(message, ##__VA_ARGS__); }

int main(int argc, char *argv[]);

#endif //NIKI_ASM_NAS_H
