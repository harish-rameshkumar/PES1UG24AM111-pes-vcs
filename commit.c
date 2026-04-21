// commit.c — Commit object implementation for PES-VCS

#include "commit.h"
#include "index.h"
#include "tree.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <inttypes.h>

int object_write(ObjectType type, const void *data, size_t len, ObjectID *id_out);
int object_read(const ObjectID *id, ObjectType *type_out, void **data_out, size_t *len_out);

// Read HEAD commit hash
int head_read(ObjectID *id_out) {
    FILE *fp = fopen(HEAD_FILE, "r");
    if (!fp) return -1;

    char line[512];

    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    line[strcspn(line, "\n")] = '\0';

    if (strncmp(line, "ref: ", 5) == 0) {
        char path[512];
        snprintf(path, sizeof(path), "%s/%s", PES_DIR, line + 5);

        fp = fopen(path, "r");
        if (!fp) return -1;

        if (!fgets(line, sizeof(line), fp)) {
            fclose(fp);
            return -1;
        }

        fclose(fp);
        line[strcspn(line, "\n")] = '\0';
    }

    return hex_to_hash(line, id_out);
}

// Update HEAD branch
int head_update(const ObjectID *new_commit) {
    FILE *fp = fopen(HEAD_FILE, "r");
    if (!fp) return -1;

    char line[512];

    if (!fgets(line, sizeof(line), fp)) {
        fclose(fp);
        return -1;
    }

    fclose(fp);

    line[strcspn(line, "\n")] = '\0';

    char path[512];

    if (strncmp(line, "ref: ", 5) == 0)
        snprintf(path, sizeof(path), "%s/%s", PES_DIR, line + 5);
    else
        strcpy(path, HEAD_FILE);

    fp = fopen(path, "w");
    if (!fp) return -1;

    char hex[65];
    hash_to_hex(new_commit, hex);

    fprintf(fp, "%s\n", hex);
    fclose(fp);

    return 0;
}

// Serialize commit object
int commit_serialize(const Commit *c, void **data_out, size_t *len_out) {
    char tree_hex[65];
    char parent_hex[65];

    hash_to_hex(&c->tree, tree_hex);

    char buf[8192];
    int n = 0;

    n += sprintf(buf + n, "tree %s\n", tree_hex);

    if (c->has_parent) {
        hash_to_hex(&c->parent, parent_hex);
        n += sprintf(buf + n, "parent %s\n", parent_hex);
    }

    n += sprintf(buf + n,
                 "author %s %" PRIu64 "\n"
                 "committer %s %" PRIu64 "\n"
                 "\n"
                 "%s",
                 c->author, c->timestamp,
                 c->author, c->timestamp,
                 c->message);

    *data_out = malloc(n + 1);
    memcpy(*data_out, buf, n + 1);

    *len_out = n;
    return 0;
}

// Parse commit object
int commit_parse(const void *data, size_t len, Commit *out) {
    (void)len;

    const char *p = data;
    char hex[65];

    if (sscanf(p, "tree %64s", hex) != 1)
        return -1;

    hex_to_hash(hex, &out->tree);

    p = strchr(p, '\n') + 1;

    if (strncmp(p, "parent ", 7) == 0) {
        sscanf(p, "parent %64s", hex);
        hex_to_hash(hex, &out->parent);
        out->has_parent = 1;
        p = strchr(p, '\n') + 1;
    } else {
        out->has_parent = 0;
    }

    sscanf(p, "author %255[^\n]", out->author);

    char *last = strrchr(out->author, ' ');
    out->timestamp = strtoull(last + 1, NULL, 10);
    *last = '\0';

    p = strstr(p, "\n\n");
    if (!p) return -1;

    strcpy(out->message, p + 2);

    return 0;
}

// Create commit
int commit_create(const char *message, ObjectID *commit_id_out) {
    ObjectID tree_id;

    if (tree_from_index(&tree_id) != 0)
        return -1;

    Commit c;
    memset(&c, 0, sizeof(c));

    c.tree = tree_id;

    if (head_read(&c.parent) == 0)
        c.has_parent = 1;
    else
        c.has_parent = 0;

    strcpy(c.author, pes_author());
    c.timestamp = (uint64_t)time(NULL);
    strcpy(c.message, message);

    void *raw;
    size_t raw_len;

    commit_serialize(&c, &raw, &raw_len);

    if (object_write(OBJ_COMMIT, raw, raw_len, commit_id_out) != 0) {
        free(raw);
        return -1;
    }

    free(raw);

    return head_update(commit_id_out);
}

// Walk commit history
int commit_walk(commit_walk_fn callback, void *ctx) {
    ObjectID id;

    if (head_read(&id) != 0)
        return -1;

    while (1) {
        ObjectType type;
        void *raw;
        size_t len;

        if (object_read(&id, &type, &raw, &len) != 0)
            return -1;

        Commit c;
        commit_parse(raw, len, &c);
        free(raw);

        callback(&id, &c, ctx);

        if (!c.has_parent)
            break;

        id = c.parent;
    }

    return 0;
}    const char *p = (const char *)data;
    char hex[HASH_HEX_SIZE + 1];

    // "tree <hex>\n"
    if (sscanf(p, "tree %64s\n", hex) != 1) return -1;
    if (hex_to_hash(hex, &commit_out->tree) != 0) return -1;
    p = strchr(p, '\n') + 1;

    // optional "parent <hex>\n"
    if (strncmp(p, "parent ", 7) == 0) {
        if (sscanf(p, "parent %64s\n", hex) != 1) return -1;
        if (hex_to_hash(hex, &commit_out->parent) != 0) return -1;
        commit_out->has_parent = 1;
        p = strchr(p, '\n') + 1;
    } else {
        commit_out->has_parent = 0;
    }

    // "author <name> <timestamp>\n"
    char author_buf[256];
    uint64_t ts;
    if (sscanf(p, "author %255[^\n]\n", author_buf) != 1) return -1;
    // split off trailing timestamp
    char *last_space = strrchr(author_buf, ' ');
    if (!last_space) return -1;
    ts = (uint64_t)strtoull(last_space + 1, NULL, 10);
    *last_space = '\0';
    snprintf(commit_out->author, sizeof(commit_out->author), "%s", author_buf);
    commit_out->timestamp = ts;
    p = strchr(p, '\n') + 1;  // skip author line
    p = strchr(p, '\n') + 1;  // skip committer line
    p = strchr(p, '\n') + 1;  // skip blank line

    snprintf(commit_out->message, sizeof(commit_out->message), "%s", p);
    return 0;
}

// Serialize a Commit struct to the text format.
// Caller must free(*data_out).
int commit_serialize(const Commit *commit, void **data_out, size_t *len_out) {
    char tree_hex[HASH_HEX_SIZE + 1];
    char parent_hex[HASH_HEX_SIZE + 1];
    hash_to_hex(&commit->tree, tree_hex);

    char buf[8192];
    int n = 0;
    n += snprintf(buf + n, sizeof(buf) - n, "tree %s\n", tree_hex);
    if (commit->has_parent) {
        hash_to_hex(&commit->parent, parent_hex);
        n += snprintf(buf + n, sizeof(buf) - n, "parent %s\n", parent_hex);
    }
    n += snprintf(buf + n, sizeof(buf) - n,
                  "author %s %" PRIu64 "\n"
                  "committer %s %" PRIu64 "\n"
                  "\n"
                  "%s",
                  commit->author, commit->timestamp,
                  commit->author, commit->timestamp,
                  commit->message);

    *data_out = malloc(n + 1);
    if (!*data_out) return -1;
    memcpy(*data_out, buf, n + 1);
    *len_out = (size_t)n;
    return 0;
}

// Walk commit history from HEAD to the root.
int commit_walk(commit_walk_fn callback, void *ctx) {
    ObjectID id;
    if (head_read(&id) != 0) return -1;

    while (1) {
        ObjectType type;
        void *raw;
        size_t raw_len;
        if (object_read(&id, &type, &raw, &raw_len) != 0) return -1;

        Commit c;
        int rc = commit_parse(raw, raw_len, &c);
        free(raw);
        if (rc != 0) return -1;

        callback(&id, &c, ctx);

        if (!c.has_parent) break;
        id = c.parent;
    }
    return 0;
}

// Read the current HEAD commit hash.
int head_read(ObjectID *id_out) {
    FILE *f = fopen(HEAD_FILE, "r");
    if (!f) return -1;
    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    fclose(f);
    line[strcspn(line, "\r\n")] = '\0'; // strip newline

    char ref_path[512];
    if (strncmp(line, "ref: ", 5) == 0) {
        snprintf(ref_path, sizeof(ref_path), "%s/%s", PES_DIR, line + 5);
        f = fopen(ref_path, "r");
        if (!f) return -1; // Branch exists but has no commits yet
        if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
        fclose(f);
        line[strcspn(line, "\r\n")] = '\0';
    }
    return hex_to_hash(line, id_out);
}

// Update the current branch ref to point to a new commit atomically.
int head_update(const ObjectID *new_commit) {
    FILE *f = fopen(HEAD_FILE, "r");
    if (!f) return -1;
    char line[512];
    if (!fgets(line, sizeof(line), f)) { fclose(f); return -1; }
    fclose(f);
    line[strcspn(line, "\r\n")] = '\0';

    char target_path[520];
    if (strncmp(line, "ref: ", 5) == 0) {
        snprintf(target_path, sizeof(target_path), "%s/%s", PES_DIR, line + 5);
    } else {
        snprintf(target_path, sizeof(target_path), "%s", HEAD_FILE); // Detached HEAD
    }

    char tmp_path[528];
    snprintf(tmp_path, sizeof(tmp_path), "%s.tmp", target_path);
    
    f = fopen(tmp_path, "w");
    if (!f) return -1;
    
    char hex[HASH_HEX_SIZE + 1];
    hash_to_hex(new_commit, hex);
    fprintf(f, "%s\n", hex);
    
    fflush(f);
    fsync(fileno(f));
    fclose(f);
    
    return rename(tmp_path, target_path);
}

// ─── TODO: Implement these ───────────────────────────────────────────────────

// Create a new commit from the current staging area.
//
// HINTS - Useful functions to call:
//   - tree_from_index   : writes the directory tree and gets the root hash
//   - head_read         : gets the parent commit hash (if any)
//   - pes_author        : retrieves the author name string (from pes.h)
//   - time(NULL)        : gets the current unix timestamp
//   - commit_serialize  : converts the filled Commit struct to a text buffer
//   - object_write      : saves the serialized text as OBJ_COMMIT
//   - head_update       : moves the branch pointer to your new commit
//
// Returns 0 on success, -1 on error.
int commit_create(const char *message, ObjectID *commit_id_out) {
    // TODO: Implement commit creation
    // (See Lab Appendix for logical steps)
    (void)message; (void)commit_id_out;
    return -1;
}
