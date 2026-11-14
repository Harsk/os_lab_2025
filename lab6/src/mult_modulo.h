#ifndef MULT_MODULO_H
#define MULT_MODULO_H

#include <stdint.h>

// Считает (a * b) % mod без переполнения uint64_t
uint64_t MultModulo(uint64_t a, uint64_t b, uint64_t mod);

#endif
