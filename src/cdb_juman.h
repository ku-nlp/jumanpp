

#include "cdb/cdb.h"
struct CDB_FILE {
    int fd;
    int mode;
    struct cdb_make cdbm; /* for creation */
    struct cdb cdb;       /* for querying */
};
typedef CDB_FILE *DBM_FILE;

#define DB_open(name, rw, mode) db_read_open(name)
#define DB_close(dbf) db_close(dbf)

CDB_FILE *db_read_open(const char *filename);
void db_close(CDB_FILE *db);
char *db_get(CDB_FILE *db, const char *buf);
