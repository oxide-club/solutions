#ifndef OXIDE_API_H
#define OXIDE_API_H
#include <stddef.h>
#include <stdint.h>
/* Retorna cópia independente; chamador possui e libera o resultado. */
unsigned char *copy_bytes(const unsigned char *, size_t);
#endif
