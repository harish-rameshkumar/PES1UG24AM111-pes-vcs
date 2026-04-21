// tree.c — Tree object implementation for PES-VCS

#include "tree.h"
#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
int hex_to_hash(const char *hex, ObjectID *id_out);

// Sort tree entries by name
static int cmp_entries(const void *a, const void *b) {
    const TreeEntry *x = (const TreeEntry *)a;
    const TreeEntry *y = (const TreeEntry *)b;
    return strcmp(x->name, y->name);
}

// Parse raw tree object
int tree_parse(const void *data, size_t len, Tree *tree_out) {
    tree_out->count = 0;

    const unsigned char *ptr = data;
    const unsigned char *end = ptr + len;

    while (ptr < end && tree_out->count < MAX_TREE_ENTRIES) {
        TreeEntry *e = &tree_out->entries[tree_out->count];

        const unsigned char *space = memchr(ptr, ' ', end - ptr);
        if (!space) return -1;

        char modebuf[16] = {0};
        memcpy(modebuf, ptr, space - ptr);
        e->mode = strtol(modebuf, NULL, 8);

        ptr = space + 1;

        const unsigned char *nul = memchr(ptr, '\0', end - ptr);
        if (!nul) return -1;

        size_t name_len = nul - ptr;
        memcpy(e->name, ptr, name_len);
        e->name[name_len] = '\0';

        ptr = nul + 1;

        if (ptr + HASH_SIZE > end) return -1;

        memcpy(e->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;

        tree_out->count++;
    }

    return 0;
}

// Serialize tree
int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    Tree copy = *tree;

    qsort(copy.entries, copy.count, sizeof(TreeEntry), cmp_entries);

    size_t max = copy.count * 320;
    unsigned char *buf = malloc(max);
    if (!buf) return -1;

    size_t off = 0;

    for (int i = 0; i < copy.count; i++) {
        TreeEntry *e = &copy.entries[i];

        int n = sprintf((char *)buf + off, "%o %s", e->mode, e->name);
        off += n + 1;

        memcpy(buf + off, e->hash.hash, HASH_SIZE);
        off += HASH_SIZE;
    }

    *data_out = buf;
    *len_out = off;

    return 0;
}

// Build tree from .pes/index
int tree_from_index(ObjectID *id_out) {
    FILE *fp = fopen(".pes/index", "r");
    if (!fp) return -1;

    Tree tree;
    tree.count = 0;

    while (tree.count < MAX_TREE_ENTRIES) {
        TreeEntry *e = &tree.entries[tree.count];

        char hex[65];
        unsigned int mode;
        unsigned long long mtime;
        unsigned int size;
        char path[512];

        int rc = fscanf(fp, "%u %64s %llu %u %511s",
                        &mode,
                        hex,
                        &mtime,
                        &size,
                        path);

        if (rc != 5) break;

        e->mode = mode;
        hex_to_hash(hex, &e->hash);
        strcpy(e->name, path);

        tree.count++;
    }

    fclose(fp);

    void *raw;
    size_t raw_len;

    if (tree_serialize(&tree, &raw, &raw_len) != 0)
        return -1;

    int rc = object_write(OBJ_TREE, raw, raw_len, id_out);

    free(raw);
    return rc;
}// Parse binary tree data into a Tree struct safely.
// Returns 0 on success, -1 on parse error.
int tree_parse(const void *data, size_t len, Tree *tree_out) {
    tree_out->count = 0;
    const uint8_t *ptr = (const uint8_t *)data;
    const uint8_t *end = ptr + len;

    while (ptr < end && tree_out->count < MAX_TREE_ENTRIES) {
        TreeEntry *entry = &tree_out->entries[tree_out->count];

        // 1. Safely find the space character for the mode
        const uint8_t *space = memchr(ptr, ' ', end - ptr);
        if (!space) return -1; // Malformed data

        // Parse mode into an isolated buffer
        char mode_str[16] = {0};
        size_t mode_len = space - ptr;
        if (mode_len >= sizeof(mode_str)) return -1;
        memcpy(mode_str, ptr, mode_len);
        entry->mode = strtol(mode_str, NULL, 8);

        ptr = space + 1; // Skip space

        // 2. Safely find the null terminator for the name
        const uint8_t *null_byte = memchr(ptr, '\0', end - ptr);
        if (!null_byte) return -1; // Malformed data

        size_t name_len = null_byte - ptr;
        if (name_len >= sizeof(entry->name)) return -1;
        memcpy(entry->name, ptr, name_len);
        entry->name[name_len] = '\0'; // Ensure null-terminated

        ptr = null_byte + 1; // Skip null byte

        // 3. Read the 32-byte binary hash
        if (ptr + HASH_SIZE > end) return -1; 
        memcpy(entry->hash.hash, ptr, HASH_SIZE);
        ptr += HASH_SIZE;

        tree_out->count++;
    }
    return 0;
}

// Helper for qsort to ensure consistent tree hashing
static int compare_tree_entries(const void *a, const void *b) {
    return strcmp(((const TreeEntry *)a)->name, ((const TreeEntry *)b)->name);
}

// Serialize a Tree struct into binary format for storage.
// Caller must free(*data_out).
// Returns 0 on success, -1 on error.
int tree_serialize(const Tree *tree, void **data_out, size_t *len_out) {
    // Estimate max size: (6 bytes mode + 1 byte space + 256 bytes name + 1 byte null + 32 bytes hash) per entry
    size_t max_size = tree->count * 296; 
    uint8_t *buffer = malloc(max_size);
    if (!buffer) return -1;

    // Create a mutable copy to sort entries (Git requirement)
    Tree sorted_tree = *tree;
    qsort(sorted_tree.entries, sorted_tree.count, sizeof(TreeEntry), compare_tree_entries);

    size_t offset = 0;
    for (int i = 0; i < sorted_tree.count; i++) {
        const TreeEntry *entry = &sorted_tree.entries[i];
        
        // Write mode and name (%o writes octal correctly for Git standards)
        int written = sprintf((char *)buffer + offset, "%o %s", entry->mode, entry->name);
        offset += written + 1; // +1 to step over the null terminator written by sprintf
        
        // Write binary hash
        memcpy(buffer + offset, entry->hash.hash, HASH_SIZE);
        offset += HASH_SIZE;
    }

    *data_out = buffer;
    *len_out = offset;
    return 0;
}

// ─── TODO: Implement these ──────────────────────────────────────────────────

// Build a tree hierarchy from the current index and write all tree
// objects to the object store.
//
// HINTS - Useful functions and concepts for this phase:
//   - index_load      : load the staged files into memory
//   - strchr          : find the first '/' in a path to separate directories from files
//   - strncmp         : compare prefixes to group files belonging to the same subdirectory
//   - Recursion       : you will likely want to create a recursive helper function 
//                       (e.g., `write_tree_level(entries, count, depth)`) to handle nested dirs.
//   - tree_serialize  : convert your populated Tree struct into a binary buffer
//   - object_write    : save that binary buffer to the store as OBJ_TREE
//
// Returns 0 on success, -1 on error.
int tree_from_index(ObjectID *id_out) {
    // TODO: Implement recursive tree building
    // (See Lab Appendix for logical steps)
    (void)id_out;
    return -1;
}
