/* SPDX-License-Identifier: MIT */
#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define MAX_FILE (64u * 1024u * 1024u)
#define RVA_LIMIT (UINT64_C(1) << 32)

struct section {
    unsigned char name[8];
    uint64_t rva, virtual_size, raw_offset, raw_size;
};

struct image {
    uint64_t image_base, entry_rva, headers;
    unsigned count, directory_count;
    struct {
        uint64_t address, size;
    } directories[16];
    struct section sections[96];
};

struct diagnostic {
    const char *code, *field;
    uint64_t offset;
};

static int report(int status, const char *code, const char *field, uint64_t offset) {
    fprintf(stderr, "{\"code\":\"%s\",\"field\":\"%s\",\"offset\":\"0x%" PRIx64 "\"}\n", code,
            field, offset);
    return status;
}

/* A subtração evita somar um offset arbitrário ao tamanho solicitado. */
static bool contains(size_t size, uint64_t offset, uint64_t length) {
    return offset <= size && length <= size - offset;
}

/* Chamadores validam o intervalo completo antes de decodificar seus campos. */
static uint64_t little(const unsigned char *data, size_t offset, unsigned width) {
    uint64_t value = 0;
    for (unsigned i = 0; i < width; i++) {
        value |= (uint64_t)data[offset + i] << (8 * i);
    }
    return value;
}

static bool reject(struct diagnostic *error, const char *code, const char *field, uint64_t offset) {
    *error = (struct diagnostic){code, field, offset};
    return false;
}

static bool overlap(uint64_t a, uint64_t length_a, uint64_t b, uint64_t length_b) {
    return length_a != 0 && length_b != 0 && a < b + length_b && b < a + length_a;
}

static bool parse(const unsigned char *data, size_t size, struct image *image,
                  struct diagnostic *error) {
    if (!contains(size, 0, 64)) {
        return reject(error, "truncated_dos", "dos", 0);
    }
    if (memcmp(data, "MZ", 2) != 0) {
        return reject(error, "unsupported_format", "dos.signature", 0);
    }
    uint64_t pe = little(data, 0x3c, 4);
    if (!contains(size, pe, 24)) {
        return reject(error, "truncated_pe", "pe", pe);
    }
    if (memcmp(data + pe, "PE\0\0", 4) != 0 || little(data, pe + 4, 2) != 0x8664) {
        return reject(error, "unsupported_format", "pe.signature_or_machine", pe);
    }
    image->count = (unsigned)little(data, pe + 6, 2);
    if (image->count == 0 || image->count > 96) {
        return reject(error, "invalid_count", "coff.sections", pe + 6);
    }
    uint64_t optional = pe + 24;
    uint64_t optional_size = little(data, pe + 20, 2);
    if (optional_size < 112 || !contains(size, optional, optional_size)) {
        return reject(error, "truncated_optional", "optional", optional);
    }
    if (little(data, optional, 2) != 0x20b) {
        return reject(error, "unsupported_format", "optional.magic", optional);
    }
    uint64_t directories = little(data, optional + 108, 4);
    if (directories > 16 || directories * 8 > optional_size - 112) {
        return reject(error, "invalid_directories", "optional.directories", optional + 108);
    }
    image->directory_count = (unsigned)directories;
    image->image_base = little(data, optional + 24, 8);
    image->entry_rva = little(data, optional + 16, 4);
    image->headers = little(data, optional + 60, 4);
    for (unsigned i = 0; i < directories; i++) {
        image->directories[i].address = little(data, optional + 112 + i * 8, 4);
        image->directories[i].size = little(data, optional + 116 + i * 8, 4);
    }
    /* pe é uint32, optional_size é uint16 e count <= 96: somas cabem em uint64. */
    uint64_t table = optional + optional_size;
    uint64_t table_size = (uint64_t)image->count * 40;
    if (!contains(size, table, table_size)) {
        return reject(error, "truncated_sections", "sections", table);
    }
    if (image->headers < table + table_size || image->headers > size) {
        return reject(error, "invalid_headers", "optional.size_of_headers", optional + 60);
    }
    for (unsigned i = 0; i < image->count; i++) {
        uint64_t offset = table + (uint64_t)i * 40;
        struct section *s = &image->sections[i];
        memcpy(s->name, data + offset, 8);
        s->virtual_size = little(data, offset + 8, 4);
        s->rva = little(data, offset + 12, 4);
        s->raw_size = little(data, offset + 16, 4);
        s->raw_offset = little(data, offset + 20, 4);
        if (s->rva + s->virtual_size > RVA_LIMIT) {
            return reject(error, "rva_overflow", "section.virtual_size", offset + 8);
        }
        if ((s->virtual_size == 0 && s->raw_size != 0) ||
            (s->virtual_size != 0 && s->rva < image->headers) ||
            (s->raw_size != 0 && s->raw_offset < image->headers)) {
            return reject(error, "unsupported_layout", "section", offset);
        }
        if (s->raw_size != 0 && !contains(size, s->raw_offset, s->raw_size)) {
            return reject(error, "truncated_raw", "section.raw", offset + 20);
        }
        for (unsigned j = 0; j < i; j++) {
            const struct section *previous = &image->sections[j];
            if (overlap(s->raw_offset, s->raw_size, previous->raw_offset, previous->raw_size)) {
                return reject(error, "raw_overlap", "section.raw", offset + 20);
            }
            if (overlap(s->rva, s->virtual_size, previous->rva, previous->virtual_size)) {
                return reject(error, "virtual_overlap", "section.rva", offset + 12);
            }
        }
    }
    return true;
}

static void print_query(const struct image *image, uint64_t rva, uint64_t size) {
    const char *kind = "unmapped";
    int section_index = -1;
    uint64_t file_offset = 0;
    bool has_offset = false;
    if (rva < image->headers) {
        kind = rva + size <= image->headers ? "headers" : "cross_region";
        file_offset = rva;
        has_offset = rva + size <= image->headers;
    } else {
        for (unsigned i = 0; i < image->count; i++) {
            const struct section *s = &image->sections[i];
            if (rva < s->rva || rva >= s->rva + s->virtual_size) {
                continue;
            }
            section_index = (int)i;
            uint64_t raw_prefix = s->raw_size < s->virtual_size ? s->raw_size : s->virtual_size;
            uint64_t relative = rva - s->rva;
            if (relative < raw_prefix) {
                has_offset = size <= raw_prefix - relative;
                kind = has_offset ? "section_raw" : "cross_region";
                file_offset = s->raw_offset + relative;
            } else {
                kind = size <= s->virtual_size - relative ? "zero_fill" : "cross_region";
            }
            break;
        }
    }
    printf("{\"rva\":\"0x%" PRIx64 "\",\"size\":%" PRIu64 ",\"kind\":\"%s\",\"section_index\":",
           rva, size, kind);
    if (section_index < 0) {
        printf("null");
    } else {
        printf("%d", section_index);
    }
    if (has_offset) {
        printf(",\"file_offset\":\"0x%" PRIx64 "\"}", file_offset);
    } else {
        printf(",\"file_offset\":null}");
    }
}

static void print_image(const struct image *image, bool query, uint64_t rva, uint64_t size) {
    printf("{\"schema_version\":1,\"machine\":\"0x8664\",\"image_base\":\"0x%" PRIx64
           "\",\"entry_rva\":\"0x%" PRIx64 "\",\"size_of_headers\":%" PRIu64
           ",\"data_directories\":[",
           image->image_base, image->entry_rva, image->headers);
    for (unsigned i = 0; i < image->directory_count; i++) {
        printf("%s{\"address\":\"0x%" PRIx64 "\",\"size\":%" PRIu64 "}", i == 0 ? "" : ",",
               image->directories[i].address, image->directories[i].size);
    }
    printf("],\"sections\":[");
    for (unsigned i = 0; i < image->count; i++) {
        const struct section *s = &image->sections[i];
        printf("%s{\"name_hex\":\"", i == 0 ? "" : ",");
        for (unsigned j = 0; j < 8; j++) {
            printf("%02x", (unsigned)s->name[j]);
        }
        printf("\",\"rva\":\"0x%" PRIx64 "\",\"virtual_size\":%" PRIu64
               ",\"raw_offset\":\"0x%" PRIx64 "\",\"raw_size\":%" PRIu64 "}",
               s->rva, s->virtual_size, s->raw_offset, s->raw_size);
    }
    printf("],\"query\":");
    if (query) {
        print_query(image, rva, size);
    } else {
        printf("null");
    }
    printf("}\n");
}

/* Aceita somente dígitos: strtoull também aceitaria sinal e espaços iniciais. */
static bool number(const char *text, unsigned base, uint64_t maximum, uint64_t *out) {
    if (base == 16 && text[0] == '0' && (text[1] == 'x' || text[1] == 'X')) {
        text += 2;
    }
    if (*text == '\0') {
        return false;
    }
    uint64_t value = 0;
    for (; *text; text++) {
        unsigned digit;
        if (*text >= '0' && *text <= '9') {
            digit = (unsigned)(*text - '0');
        } else if (*text >= 'a' && *text <= 'f') {
            digit = (unsigned)(*text - 'a' + 10);
        } else if (*text >= 'A' && *text <= 'F') {
            digit = (unsigned)(*text - 'A' + 10);
        } else {
            return false;
        }
        if (digit >= base || value > (maximum - digit) / base) {
            return false;
        }
        value = value * base + digit;
    }
    *out = value;
    return true;
}

int main(int argc, char **argv) {
    uint64_t rva = 0, query_size = 0;
    bool query = argc == 6;
    if ((argc != 2 && !query) ||
        (query &&
         (strcmp(argv[2], "--rva") != 0 || strcmp(argv[4], "--size") != 0 ||
          !number(argv[3], 16, UINT32_MAX, &rva) || !number(argv[5], 10, 4096, &query_size) ||
          query_size == 0 || rva + query_size > RVA_LIMIT))) {
        return report(2, "usage", "arguments", 0);
    }
    FILE *file = fopen(argv[1], "rb");
    if (file == NULL) {
        return report(4, "io", "file", 0);
    }
    /* Um byte adicional distingue o limite aceito de um arquivo maior. */
    unsigned char *data = malloc((size_t)MAX_FILE + 1);
    if (data == NULL) {
        fclose(file);
        return report(4, "io", "allocation", 0);
    }
    size_t size = fread(data, 1, (size_t)MAX_FILE + 1, file);
    bool io_error = ferror(file) != 0;
    if (fclose(file) != 0) {
        io_error = true;
    }
    int status;
    struct image image = {0};
    struct diagnostic error;
    if (io_error) {
        status = report(4, "io", "file", 0);
    } else if (size > MAX_FILE) {
        status = report(3, "too_large", "file", MAX_FILE);
    } else if (!parse(data, size, &image, &error)) {
        status = report(3, error.code, error.field, error.offset);
    } else {
        print_image(&image, query, rva, query_size);
        status = fflush(stdout) == 0 ? 0 : report(4, "io", "stdout", 0);
    }
    free(data);
    return status;
}
