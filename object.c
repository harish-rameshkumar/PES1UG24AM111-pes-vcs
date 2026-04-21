// object.c — Object storage implementation for PES-VCS

#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <openssl/evp.h>

// Convert binary hash to hex string
void hash_to_hex(const ObjectID *id, char *hex_out) {
    for (int i = 0; i < HASH_SIZE; i++) {
        sprintf(hex_out + (i * 2), "%02x", id->hash[i]);
    }
    hex_out[HASH_HEX_SIZE] = '\0';
}

// Convert hex string to binary hash
int hex_to_hash(const char *hex, ObjectID *id_out) {
    if (strlen(hex) != HASH_HEX_SIZE) return -1;

    for (int i = 0; i < HASH_SIZE; i++) {
        unsigned int byte;
        if (sscanf(hex + (i * 2), "%2x", &byte) != 1)
            return -1;
        id_out->hash[i] = (uint8_t)byte;
    }
    return 0;
}

// Compute SHA-256 hash
static void compute_hash(const void *data, size_t len, ObjectID *id_out) {
    unsigned int out_len = 0;

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    EVP_DigestInit_ex(ctx, EVP_sha256(), NULL);
    EVP_DigestUpdate(ctx, data, len);
    EVP_DigestFinal_ex(ctx, id_out->hash, &out_len);
    EVP_MD_CTX_free(ctx);
}

// Create path from object id
static void object_path(const ObjectID *id, char *path_out, size_t size) {
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id, hex);

    snprintf(path_out, size, "%s/%.2s/%s",
             OBJECTS_DIR,
             hex,
             hex + 2);
}

// Check if object exists
static int object_exists(const ObjectID *id) {
    char path[512];
    object_path(id, path, sizeof(path));
    return access(path, F_OK) == 0;
}

// Write object to store
int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out) {
    const char *type_name;

    if (type == OBJ_BLOB) type_name = "blob";
    else if (type == OBJ_TREE) type_name = "tree";
    else type_name = "commit";

    char header[64];
    int header_len = snprintf(header, sizeof(header), "%s %zu", type_name, len) + 1;

    size_t total = header_len + len;
    unsigned char *buf = malloc(total);
    if (!buf) return -1;

    memcpy(buf, header, header_len);
    memcpy(buf + header_len, data, len);

    compute_hash(buf, total, id_out);

    if (object_exists(id_out)) {
        free(buf);
        return 0;
    }

    mkdir(PES_DIR, 0755);
    mkdir(OBJECTS_DIR, 0755);

    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(id_out, hex);

    char dir[512];
    snprintf(dir, sizeof(dir), "%s/%.2s", OBJECTS_DIR, hex);
    mkdir(dir, 0755);

    char final_path[512];
    object_path(id_out, final_path, sizeof(final_path));

    FILE *fp = fopen(final_path, "wb");
    if (!fp) {
        free(buf);
        return -1;
    }

    fwrite(buf, 1, total, fp);
    fclose(fp);

    free(buf);
    return 0;
}

// Read object from store
int object_read(const ObjectID *id,
                ObjectType *type_out,
                void **data_out,
                size_t *len_out) {
    char path[512];
    object_path(id, path, sizeof(path));

    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    unsigned char *buf = malloc(size);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    fread(buf, 1, size, fp);
    fclose(fp);

    char *nul = memchr(buf, '\0', size);
    if (!nul) {
        free(buf);
        return -1;
    }

    char type_name[16];
    size_t data_len;

    sscanf((char *)buf, "%15s %zu", type_name, &data_len);

    if (strcmp(type_name, "blob") == 0) *type_out = OBJ_BLOB;
    else if (strcmp(type_name, "tree") == 0) *type_out = OBJ_TREE;
    else *type_out = OBJ_COMMIT;

    size_t header_len = (nul - (char *)buf) + 1;

    *data_out = malloc(data_len);
    if (!*data_out) {
        free(buf);
        return -1;
    }

    memcpy(*data_out, buf + header_len, data_len);
    *len_out = data_len;

    free(buf);
    return 0;
}
