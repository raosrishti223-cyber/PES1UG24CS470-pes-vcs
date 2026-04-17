#include "index.h"
#include "pes.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

// ---------------- PROVIDED ----------------

IndexEntry* index_find(Index *index, const char *path) {
    for (int i = 0; i < index->count; i++) {
        if (strcmp(index->entries[i].path, path) == 0)
            return &index->entries[i];
    }
    return NULL;
}

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

// ---------------- STATUS (already correct, keep as is) ----------------

int index_status(const Index *index) {
    printf("Staged changes:\n");
    int staged = 0;
    for (int i = 0; i < index->count; i++) {
        printf("  staged:     %s\n", index->entries[i].path);
        staged++;
    }
    if (!staged) printf("  (nothing to show)\n");

    printf("\nUnstaged changes:\n");
    int unstaged = 0;
    for (int i = 0; i < index->count; i++) {
        struct stat st;
        if (stat(index->entries[i].path, &st) != 0) {
            printf("  deleted:    %s\n", index->entries[i].path);
            unstaged++;
        } else {
            if (st.st_mtime != (time_t)index->entries[i].mtime_sec ||
                st.st_size != (off_t)index->entries[i].size) {
                printf("  modified:   %s\n", index->entries[i].path);
                unstaged++;
            }
        }
    }
    if (!unstaged) printf("  (nothing to show)\n");

printf("Untracked files:\n");

int untracked_count = 0;
DIR *dir = opendir(".");
if (dir) {
    struct dirent *ent;

    while ((ent = readdir(dir)) != NULL) {

        if (strcmp(ent->d_name, ".") == 0 ||
            strcmp(ent->d_name, "..") == 0 ||
            strcmp(ent->d_name, ".pes") == 0 ||
            strcmp(ent->d_name, "pes") == 0 ||
            strstr(ent->d_name, ".o") != NULL) {
            continue;
        }

        int is_tracked = 0;
        for (int i = 0; i < index->count; i++) {
            if (strcmp(index->entries[i].path, ent->d_name) == 0) {
                is_tracked = 1;
                break;
            }
        }

        if (!is_tracked) {
            printf("  untracked:  %s\n", ent->d_name);
            untracked_count++;
        }
    }
    closedir(dir);
}

if (untracked_count == 0)
    printf("  (nothing to show)\n");

printf("\n");
}
// ---------------- LOAD INDEX (FIXED) ----------------

int index_load(Index *index) {
    FILE *f = fopen(".pes/index", "r");
    index->count = 0;

    if (!f) return 0;

    while (!feof(f)) {
        IndexEntry e;
        char hash_hex[65];

        int r = fscanf(f, "%o %64s %lu %ld %255s",
                       &e.mode,
                       hash_hex,
                       &e.mtime_sec,
                       &e.size,
                       e.path);

        if (r != 5) continue;

        hex_to_hash(hash_hex, &e.hash);

        index->entries[index->count++] = e;
    }

    fclose(f);
    return 0;
}

// ---------------- SAVE INDEX (FIXED) ----------------

int index_save(const Index *index) {
    FILE *f = fopen(".pes/index.tmp", "w");
    if (!f) return -1;

    for (int i = 0; i < index->count; i++) {
        char hash_hex[65];
        hash_to_hex(&index->entries[i].hash, hash_hex);

        fprintf(f, "%o %s %lu %ld %s\n",
                index->entries[i].mode,
                hash_hex,
                index->entries[i].mtime_sec,
                index->entries[i].size,
                index->entries[i].path);
    }

    fflush(f);
    fsync(fileno(f));
    fclose(f);

    rename(".pes/index.tmp", ".pes/index");
    return 0;
}

// ---------------- ADD FILE (FIXED SAFELY) ----------------

int index_add(Index *index, const char *path) {
    struct stat st;
    if (stat(path, &st) != 0) {
        fprintf(stderr, "error: cannot access %s\n", path);
        return -1;
    }

    FILE *f = fopen(path, "rb");
    if (!f) return -1;

    char *buffer = malloc(st.st_size);
    if (!buffer) return -1;

    fread(buffer, 1, st.st_size, f);
    fclose(f);

    ObjectID oid;
    object_write(OBJ_BLOB, buffer, st.st_size, &oid);
    free(buffer);

    IndexEntry *e = index_find(index, path);
    if (!e) e = &index->entries[index->count++];

    e->mode = st.st_mode;
    e->hash = oid;
    e->mtime_sec = st.st_mtime;
    e->size = st.st_size;
    strncpy(e->path, path, sizeof(e->path) - 1);
    e->path[sizeof(e->path) - 1] = '\0';

    return index_save(index);
}
