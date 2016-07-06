// CDB.c

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/file.h>
#include <cstring>
#include <unistd.h>
#include <time.h>
#include <stdlib.h>

extern "C" {
#include "cdb_juman.h"
}

/*==================================================================*/
void *malloc_data(size_t size, char *comment)
/*==================================================================*/
{
    void *p;
    if (!(p = malloc(size))) {
        fprintf(stderr, "Can't allocate memory for %s\n", comment);
        exit(-1);
    }
    return p;
}

/* DB open for reading */
DBM_FILE db_read_open(const char *filename) {
    DBM_FILE db;

    db = (DBM_FILE)malloc_data(sizeof(CDB_FILE), (char *)"db_read_open");
    db->mode = O_RDONLY;

    if ((db->fd = open(filename, db->mode)) < 0) {
#ifdef DEBUG
        fprintf(stderr, "db_read_open: %s\n", filename);
#endif
        free(db);
        return NULL;
    }

    if (cdb_init(&(db->cdb), db->fd) != 0) { /* mmap error? */
#ifdef DEBUG
        fprintf(stderr, "db_read_open (cdb_init): %s\n", filename);
#endif
        free(db);
        return NULL;
    }
    return db;
}

/* DB close */
void db_close(DBM_FILE db) {
    if (db->mode & O_CREAT) {
        cdb_make_finish(&(db->cdbm));
    }
    close(db->fd);
    free(db);
}

/* DB get */
char *db_get(DBM_FILE db, const char *buf) {
    if (cdb_find(&(db->cdb), buf, strlen(buf)) > 0) {
        char *rbuf;
        unsigned int datalen;

        datalen = cdb_datalen(&(db->cdb));
        rbuf = (char *)malloc_data(datalen + 1, (char *)"db_get");
        cdb_read(&(db->cdb), rbuf, datalen, cdb_datapos(&(db->cdb)));
        *(rbuf + datalen) = '\0';
        return rbuf;
    }
    return NULL;
}
