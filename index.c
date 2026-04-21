// index.c — Index / staging area implementation for PES-VCS

#include "index.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);

// Find file in index
IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

// Load .pes/index
int index_load(Index *index) {
    index->count = 0;

    FILE *fp = fopen(INDEX_FILE, "r");
    if (!fp) return 0;

    while (index->count < MAX_INDEX_ENTRIES) {
        IndexEntry *e = &index->entries[index->count];

        char hex[65];

        int rc = fscanf(fp, "%u %64s %lu %u %511s",
                        &e->mode,
                        hex,
                        &e->mtime_sec,
                        &e->size,
                        e->path);

        if (rc != 5)
            break;

        hex_to_hash(hex, &e->hash);
        index->count++;
    }

    fclose(fp);
    return 0;
}

// Sort by path
static int cmp_entries(const void *a, const void *b) {
    const IndexEntry *x = (const IndexEntry *)a;
    const IndexEntry *y = (const IndexEntry *)b;
    return strcmp(x->path, y->path);
}

// Save .pes/index
int index_save(const Index *index) {
    qsort((void *)index->entries,
          index->count,
          sizeof(IndexEntry),
          cmp_entries);

    FILE *fp = fopen(INDEX_FILE, "w");
    if (!fp) return -1;

    for (int i = 0; i < index->count; i++) {
        char hex[65];
        hash_to_hex(&index->entries[i].hash, hex);

        fprintf(fp, "%u %s %lu %u %s\n",
                index->entries[i].mode,
                hex,
                index->entries[i].mtime_sec,
                index->entries[i].size,
                index->entries[i].path);
    }

    fclose(fp);
    return 0;
}

// Add file to staging
int index_add(Index *index, const char *path) {
    FILE *fp = fopen(path, "rb");
    if (!fp) return -1;

    fseek(fp, 0, SEEK_END);
    long size = ftell(fp);
    rewind(fp);

    void *buf = malloc(size);
    if (!buf) {
        fclose(fp);
        return -1;
    }

    fread(buf, 1, size, fp);
    fclose(fp);

    ObjectID oid;

    if (object_write(OBJ_BLOB, buf, size, &oid) != 0) {
        free(buf);
        return -1;
    }

    free(buf);

    struct stat st;
    stat(path, &st);

    IndexEntry *entry = index_find(index, path);

    if (!entry) {
        if (index->count >= MAX_INDEX_ENTRIES)
            return -1;

        entry = &index->entries[index->count++];
        strcpy(entry->path, path);
    }

    entry->mode = 100644;
    entry->hash = oid;
    entry->mtime_sec = st.st_mtime;
    entry->size = st.st_size;

    return index_save(index);
}

// Remove from index
int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            for (int j = i; j < index->count - 1; j++) {
                index->entries[j] = index->entries[j + 1];
            }

            index->count--;
            return index_save(index);
        }
    }

    return -1;
}

// Show status
int index_status(const Index *index) {
    printf("Staged changes:\n");

    if (index->count == 0)
        printf("  (nothing to show)\n");
    else {
        for (int i = 0; i < index->count; i++)
            printf("  staged:     %s\n", index->entries[i].path);
    }

    printf("\nUnstaged changes:\n");
    printf("  (nothing to show)\n");

    printf("\nUntracked files:\n");
    printf("  (nothing to show)\n\n");

    return 0;
}
// Remove a file from the index.
// Returns 0 on success, -1 if path not in index.
int index_remove(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0) {
            int remaining = index->count - i - 1;
            if (remaining > 0)
                memmove(&index->entries[i], &index->entries[i + 1],
                        remaining * sizeof(IndexEntry));
            index->count--;
            return index_save(index);
        }
    }
    fprintf(stderr, "error: '%s' is not in the index\n", path);
    return -1;
}

// Print the status of the working directory.
//
// Identifies files that are staged, unstaged (modified/deleted in working dir),
// and untracked (present in working dir but not in index).
// Returns 0.
int index_status(const Index *index) {
    printf("Staged changes:\n");
    int staged_count = 0;
    // Note: A true Git implementation deeply diffs against the HEAD tree here. 
    // For this lab, displaying indexed files represents the staging intent.
    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
        staged_count++;
    }
    if (staged_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    printf("Unstaged changes:\n");
    int unstaged_count = 0;
    for (int i = 0; i < index->count; i++) {
        struct stat st;
        if (stat(index->entries[i].path, &st) != 0) {
            printf("  deleted:    %s\n", index->entries[i].path);
            unstaged_count++;
        } else {
            // Fast diff: check metadata instead of re-hashing file content
            if (st.st_mtime != (time_t)index->entries[i].mtime_sec || st.st_size != (off_t)index->entries[i].size) {
                printf("  modified:   %s\n", index->entries[i].path);
                unstaged_count++;
            }
        }
    }
    if (unstaged_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    printf("Untracked files:\n");
    int untracked_count = 0;
    DIR *dir = opendir(".");
    if (dir) {
        struct dirent *ent;
        while ((ent = readdir(dir)) != NULL) {
            // Skip hidden directories, parent directories, and build artifacts
            if (strcmp(ent->d_name, ".") == 0 || strcmp(ent->d_name, "..") == 0) continue;
            if (strcmp(ent->d_name, ".pes") == 0) continue;
            if (strcmp(ent->d_name, "pes") == 0) continue; // compiled executable
            if (strstr(ent->d_name, ".o") != NULL) continue; // object files

            // Check if file is tracked in the index
            int is_tracked = 0;
            for (int i = 0; i < index->count; i++) {
                if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                    is_tracked = 1; 
                    break;
                }
            }
            
            if (!is_tracked) {
                struct stat st;
                stat(ent->d_name, &st);
                if (S_ISREG(st.st_mode)) { // Only list regular files for simplicity
                    printf("  untracked:  %s\n", ent->d_name);
                    untracked_count++;
                }
            }
        }
        closedir(dir);
    }
    if (untracked_count == 0) printf("  (nothing to show)\n");
    printf("\n");

    return 0;
}

// ─── TODO: Implement these ───────────────────────────────────────────────────

// Load the index from .pes/index.
//
// HINTS - Useful functions:
//   - fopen (with "r"), fscanf, fclose : reading the text file line by line
//   - hex_to_hash                      : converting the parsed string to ObjectID
//
// Returns 0 on success, -1 on error.
int index_load(Index *index) {
    // TODO: Implement index loading
    // (See Lab Appendix for logical steps)
    (void)index;
    return -1;
}

// Save the index to .pes/index atomically.
//
// HINTS - Useful functions and syscalls:
//   - qsort                            : sorting the entries array by path
//   - fopen (with "w"), fprintf        : writing to the temporary file
//   - hash_to_hex                      : converting ObjectID for text output
//   - fflush, fileno, fsync, fclose    : flushing userspace buffers and syncing to disk
//   - rename                           : atomically moving the temp file over the old index
//
// Returns 0 on success, -1 on error.
int index_save(const Index *index) {
    // TODO: Implement atomic index saving
    // (See Lab Appendix for logical steps)
    (void)index;
    return -1;
}

// Stage a file for the next commit.
//
// HINTS - Useful functions and syscalls:
//   - fopen, fread, fclose             : reading the target file's contents
//   - object_write                     : saving the contents as OBJ_BLOB
//   - stat / lstat                     : getting file metadata (size, mtime, mode)
//   - index_find                       : checking if the file is already staged
//
// Returns 0 on success, -1 on error.
int index_add(Index *index, const char *path) {
    // TODO: Implement file staging
    // (See Lab Appendix for logical steps)
    (void)index; (void)path;
    return -1;
}
