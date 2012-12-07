#ifndef _DB_H
#define _DB_H
/* This code generated by make_db_h.   Copyright (c) 2007-2012 Tokutek */
#ident "Copyright (c) 2007-2012 Tokutek Inc.  All rights reserved."
#include <sys/types.h>
/*stdio is needed for the FILE* in db->verify*/
#include <stdio.h>
#include <stdint.h>
#if defined(__cplusplus) || defined(__cilkplusplus)
extern "C" {
#endif
#define TOKUDB 1
#define TOKUDB_NATIVE_H 1
#define DB_VERSION_MAJOR 4
#define DB_VERSION_MINOR 6
/* As of r40364 (post TokuDB 5.2.7), the patch version number is 100+ the BDB header patch version number.*/
#define DB_VERSION_PATCH 119
#define DB_VERSION_STRING "Tokutek: TokuDB 4.6.119"
#define DB_GID_SIZE 128
typedef struct toku_xa_xid_s { /* This struct is intended to be binary compatible with the XID in the XA architecture.  See source:/import/opengroup.org/C193.pdf */
    long formatID;                  /* format identifier */
    long gtrid_length;              /* value from 1 through 64 */
    long bqual_length;              /* value from 1 through 64 */
    char data[DB_GID_SIZE];
} TOKU_XA_XID;
#ifndef TOKU_OFF_T_DEFINED
#define TOKU_OFF_T_DEFINED
typedef int64_t toku_off_t;
#endif
typedef struct __toku_db_env DB_ENV;
typedef struct __toku_db_key_range DB_KEY_RANGE;
typedef struct __toku_db_lsn DB_LSN;
typedef struct __toku_db DB;
typedef struct __toku_db_txn DB_TXN;
typedef struct __toku_db_txn_active DB_TXN_ACTIVE;
typedef struct __toku_db_txn_stat DB_TXN_STAT;
typedef struct __toku_dbc DBC;
typedef struct __toku_dbt DBT;
typedef struct __toku_db_preplist { DB_TXN *txn; uint8_t gid[DB_GID_SIZE]; } DB_PREPLIST;
typedef uint32_t db_recno_t;
typedef int(*YDB_CALLBACK_FUNCTION)(DBT const*, DBT const*, void*);
#include "tdb-internal.h"
typedef struct __toku_db_btree_stat64 {
  uint64_t bt_nkeys; /* how many unique keys (guaranteed only to be an estimate, even when flattened)          */
  uint64_t bt_ndata; /* how many key-value pairs (an estimate, but exact when flattened)                       */
  uint64_t bt_dsize; /* how big are the keys+values (not counting the lengths) (an estimate, unless flattened) */
  uint64_t bt_fsize; /* how big is the underlying file                                                         */
  uint64_t bt_create_time_sec; /* Creation time, in seconds */
  uint64_t bt_modify_time_sec; /* Time of last serialization, in seconds */
  uint64_t bt_verify_time_sec; /* Time of last verification, in seconds */
} DB_BTREE_STAT64;
typedef enum toku_compression_method {
    TOKU_NO_COMPRESSION = 0,
    TOKU_ZLIB_METHOD    = 8,
    TOKU_QUICKLZ_METHOD = 9,
    TOKU_LZMA_METHOD    = 10,
    TOKU_DEFAULT_COMPRESSION_METHOD = 1,
    TOKU_FAST_COMPRESSION_METHOD = 2,
    TOKU_SMALL_COMPRESSION_METHOD = 3,
} TOKU_COMPRESSION_METHOD;
typedef struct __toku_loader DB_LOADER;
struct __toku_loader_internal;
struct __toku_loader {
  struct __toku_loader_internal *i;
  int (*set_error_callback)(DB_LOADER *loader, void (*error_cb)(DB *db, int i, int err, DBT *key, DBT *val, void *error_extra), void *error_extra); /* set the error callback */
  int (*set_poll_function)(DB_LOADER *loader, int (*poll_func)(void *extra, float progress), void *poll_extra);             /* set the polling function */
  int (*put)(DB_LOADER *loader, DBT *key, DBT* val);                                                      /* give a row to the loader */
  int (*close)(DB_LOADER *loader);                                                                        /* finish loading, free memory */
  int (*abort)(DB_LOADER *loader);                                                                        /* abort loading, free memory */
};
typedef struct __toku_indexer DB_INDEXER;
struct __toku_indexer_internal;
struct __toku_indexer {
  struct __toku_indexer_internal *i;
  int (*set_error_callback)(DB_INDEXER *indexer, void (*error_cb)(DB *db, int i, int err, DBT *key, DBT *val, void *error_extra), void *error_extra); /* set the error callback */
  int (*set_poll_function)(DB_INDEXER *indexer, int (*poll_func)(void *extra, float progress), void *poll_extra);             /* set the polling function */
  int (*build)(DB_INDEXER *indexer);  /* build the indexes */
  int (*close)(DB_INDEXER *indexer);  /* finish indexing, free memory */
  int (*abort)(DB_INDEXER *indexer);  /* abort  indexing, free memory */
};
typedef enum { 
    FS_GREEN = 0,                                   // green zone  (we have lots of space) 
    FS_YELLOW = 1,                                  // yellow zone (issue warning but allow operations) 
    FS_RED = 2,                                     // red zone    (prevent insert operations) 
    FS_BLOCKED = 3                                  // For reporting engine status, completely blocked 
} fs_redzone_state;
// engine status info
// engine status is passed to handlerton as an array of TOKU_ENGINE_STATUS_ROW_S[]
typedef enum {
   FS_STATE = 0,   // interpret as file system state (redzone) enum 
   UINT64,         // interpret as uint64_t 
   CHARSTR,        // interpret as char * 
   UNIXTIME,       // interpret as time_t 
   TOKUTIME,       // interpret as tokutime_t 
   PARCOUNT,       // interpret as PARTITIONED_COUNTER
   MAXCOUNT        // interpret as MAX_PARTITIONED_COUNTER
} toku_engine_status_display_type; 
typedef struct __toku_engine_status_row {
  const char * keyname;                  // info schema key, should not change across revisions without good reason 
  const char * legend;                   // the text that will appear at user interface 
  toku_engine_status_display_type type;  // how to interpret the value 
  union {              
         uint64_t num; 
         const char *   str; 
         struct partitioned_counter *parcount;
         struct max_partitioned_counter *maxcount;
  } value;       
} * TOKU_ENGINE_STATUS_ROW, TOKU_ENGINE_STATUS_ROW_S; 
typedef enum {
 DB_BTREE=1,
 DB_UNKNOWN=5
} DBTYPE;
#define DB_VERB_DEADLOCK 1
#define DB_VERB_RECOVERY 8
#define DB_VERB_REPLICATION 32
#define DB_VERB_WAITSFOR 64
#define DB_ARCH_ABS 1
#define DB_ARCH_LOG 4
#define DB_CREATE 1
#define DB_CXX_NO_EXCEPTIONS 1
#define DB_EXCL 16384
#define DB_PRIVATE 8388608
#define DB_RDONLY 32
#define DB_RECOVER 64
#define DB_RUNRECOVERY -30975
#define DB_THREAD 128
#define DB_TXN_NOSYNC 512
#define DB_BLACKHOLE 524288
#define DB_LOCK_DEFAULT 1
#define DB_LOCK_OLDEST 7
#define DB_LOCK_RANDOM 8
#define DB_KEYFIRST 13
#define DB_KEYLAST 14
#define DB_NOOVERWRITE 20
#define DB_NODUPDATA 19
#define DB_NOOVERWRITE_NO_ERROR 1
#define DB_OPFLAGS_MASK 255
#define DB_AUTO_COMMIT 33554432
#define DB_INIT_LOCK 131072
#define DB_INIT_LOG 262144
#define DB_INIT_MPOOL 524288
#define DB_INIT_TXN 2097152
#define DB_KEYEXIST -30996
#define DB_LOCK_DEADLOCK -30995
#define DB_LOCK_NOTGRANTED -30994
#define DB_NOTFOUND -30989
#define DB_SECONDARY_BAD -30974
#define DB_DONOTINDEX -30998
#define DB_BUFFER_SMALL -30999
#define DB_BADFORMAT -30500
#define DB_DELETE_ANY 65536
#define DB_FIRST 7
#define DB_LAST 15
#define DB_CURRENT 6
#define DB_NEXT 16
#define DB_NEXT_NODUP 18
#define DB_PREV 23
#define DB_PREV_NODUP 25
#define DB_SET 26
#define DB_SET_RANGE 27
#define DB_CURRENT_BINDING 253
#define DB_SET_RANGE_REVERSE 252
#define DB_RMW 1073741824
#define DB_IS_RESETTING_OP 0x01000000
#define DB_PRELOCKED 0x00800000
#define DB_PRELOCKED_WRITE 0x00400000
#define DB_IS_HOT_INDEX 0x00100000
#define DBC_DISABLE_PREFETCHING 0x20000000
#define DB_UPDATE_CMP_DESCRIPTOR 0x40000000
#define DB_DBT_APPMALLOC 1
#define DB_DBT_DUPOK 2
#define DB_DBT_MALLOC 8
#define DB_DBT_MULTIPLE 16
#define DB_DBT_REALLOC 64
#define DB_DBT_USERMEM 256
#define DB_LOG_AUTOREMOVE 524288
#define DB_TXN_WRITE_NOSYNC 4096
#define DB_TXN_NOWAIT 1024
#define DB_TXN_SYNC 16384
#define DB_TXN_SNAPSHOT 268435456
#define DB_READ_UNCOMMITTED 134217728
#define DB_READ_COMMITTED 67108864
#define DB_INHERIT_ISOLATION 1
#define DB_SERIALIZABLE 2
/* TOKUDB specific error codes */
#define TOKUDB_OUT_OF_LOCKS -100000
#define TOKUDB_SUCCEEDED_EARLY -100001
#define TOKUDB_FOUND_BUT_REJECTED -100002
#define TOKUDB_USER_CALLBACK_ERROR -100003
#define TOKUDB_DICTIONARY_TOO_OLD -100004
#define TOKUDB_DICTIONARY_TOO_NEW -100005
#define TOKUDB_DICTIONARY_NO_HEADER -100006
#define TOKUDB_CANCELED -100007
#define TOKUDB_NO_DATA -100008
#define TOKUDB_ACCEPT -100009
#define TOKUDB_MVCC_DICTIONARY_TOO_NEW -100010
#define TOKUDB_UPGRADE_FAILURE -100011
#define TOKUDB_TRY_AGAIN -100012
#define TOKUDB_NEEDS_REPAIR -100013
#define TOKUDB_CURSOR_CONTINUE -100014
#define TOKUDB_BAD_CHECKSUM -100015
/* LOADER flags */
#define LOADER_USE_PUTS 1
typedef int (*generate_row_for_put_func)(DB *dest_db, DB *src_db, DBT *dest_key, DBT *dest_val, const DBT *src_key, const DBT *src_val);
typedef int (*generate_row_for_del_func)(DB *dest_db, DB *src_db, DBT *dest_key, const DBT *src_key, const DBT *src_val);
struct __toku_db_env {
  struct __toku_db_env_internal *i;
#define db_env_struct_i(x) ((x)->i)
  int (*checkpointing_set_period)             (DB_ENV*, uint32_t) /* Change the delay between automatic checkpoints.  0 means disabled. */;
  int (*checkpointing_get_period)             (DB_ENV*, uint32_t*) /* Retrieve the delay between automatic checkpoints.  0 means disabled. */;
  int (*cleaner_set_period)                   (DB_ENV*, uint32_t) /* Change the delay between automatic cleaner attempts.  0 means disabled. */;
  int (*cleaner_get_period)                   (DB_ENV*, uint32_t*) /* Retrieve the delay between automatic cleaner attempts.  0 means disabled. */;
  int (*cleaner_set_iterations)               (DB_ENV*, uint32_t) /* Change the number of attempts on each cleaner invokation.  0 means disabled. */;
  int (*cleaner_get_iterations)               (DB_ENV*, uint32_t*) /* Retrieve the number of attempts on each cleaner invokation.  0 means disabled. */;
  int (*checkpointing_postpone)               (DB_ENV*) /* Use for 'rename table' or any other operation that must be disjoint from a checkpoint */;
  int (*checkpointing_resume)                 (DB_ENV*) /* Alert tokudb 'postpone' is no longer necessary */;
  int (*checkpointing_begin_atomic_operation) (DB_ENV*) /* Begin a set of operations (that must be atomic as far as checkpoints are concerned). i.e. inserting into every index in one table */;
  int (*checkpointing_end_atomic_operation)   (DB_ENV*) /* End   a set of operations (that must be atomic as far as checkpoints are concerned). */;
  int (*set_default_bt_compare)               (DB_ENV*,int (*bt_compare) (DB *, const DBT *, const DBT *)) /* Set default (key) comparison function for all DBs in this environment.  Required for RECOVERY since you cannot open the DBs manually. */;
  int (*get_engine_status_num_rows)           (DB_ENV*, uint64_t*)  /* return number of rows in engine status */;
  int (*get_engine_status)                    (DB_ENV*, TOKU_ENGINE_STATUS_ROW, uint64_t, fs_redzone_state*, uint64_t*, char*, int) /* Fill in status struct and redzone state, possibly env panic string */;
  int (*get_engine_status_text)               (DB_ENV*, char*, int)     /* Fill in status text */;
  int (*crash)                                (DB_ENV*, const char*/*expr_as_string*/,const char */*fun*/,const char*/*file*/,int/*line*/, int/*errno*/);
  int (*get_iname)                            (DB_ENV* env, DBT* dname_dbt, DBT* iname_dbt) /* FOR TEST ONLY: lookup existing iname */;
  int (*create_loader)                        (DB_ENV *env, DB_TXN *txn, DB_LOADER **blp,    DB *src_db, int N, DB *dbs[/*N*/], uint32_t db_flags[/*N*/], uint32_t dbt_flags[/*N*/], uint32_t loader_flags);
  int (*create_indexer)                       (DB_ENV *env, DB_TXN *txn, DB_INDEXER **idxrp, DB *src_db, int N, DB *dbs[/*N*/], uint32_t db_flags[/*N*/], uint32_t indexer_flags);
  int (*put_multiple)                         (DB_ENV *env, DB *src_db, DB_TXN *txn,
                                               const DBT *src_key, const DBT *src_val,
                                               uint32_t num_dbs, DB **db_array, DBT *keys, DBT *vals, uint32_t *flags_array) /* insert into multiple DBs */;
  int (*set_generate_row_callback_for_put)    (DB_ENV *env, generate_row_for_put_func generate_row_for_put);
  int (*del_multiple)                         (DB_ENV *env, DB *src_db, DB_TXN *txn,
                                               const DBT *src_key, const DBT *src_val,
                                               uint32_t num_dbs, DB **db_array, DBT *keys, uint32_t *flags_array) /* delete from multiple DBs */;
  int (*set_generate_row_callback_for_del)    (DB_ENV *env, generate_row_for_del_func generate_row_for_del);
  int (*update_multiple)                      (DB_ENV *env, DB *src_db, DB_TXN *txn,
                                               DBT *old_src_key, DBT *old_src_data,
                                               DBT *new_src_key, DBT *new_src_data,
                                               uint32_t num_dbs, DB **db_array, uint32_t *flags_array,
                                               uint32_t num_keys, DBT *keys,
                                               uint32_t num_vals, DBT *vals) /* update multiple DBs */;
  int (*get_redzone)                          (DB_ENV *env, int *redzone) /* get the redzone limit */;
  int (*set_redzone)                          (DB_ENV *env, int redzone) /* set the redzone limit in percent of total space */;
  int (*set_lk_max_memory)                    (DB_ENV *env, uint64_t max);
  int (*get_lk_max_memory)                    (DB_ENV *env, uint64_t *max);
  void (*set_update)                          (DB_ENV *env, int (*update_function)(DB *, const DBT *key, const DBT *old_val, const DBT *extra, void (*set_val)(const DBT *new_val, void *set_extra), void *set_extra));
  int (*set_lock_timeout)                     (DB_ENV *env, uint64_t lock_wait_time_msec);
  int (*get_lock_timeout)                     (DB_ENV *env, uint64_t *lock_wait_time_msec);
  int (*txn_xa_recover)                       (DB_ENV*, TOKU_XA_XID list[/*count*/], long count, /*out*/ long *retp, uint32_t flags);
  int (*get_txn_from_xid)                 (DB_ENV*, /*in*/ TOKU_XA_XID *, /*out*/ DB_TXN **);
  int (*get_cursor_for_directory)            (DB_ENV*, /*in*/ DB_TXN *, /*out*/ DBC **);
  void *app_private;
  void *api1_internal;
  int  (*close) (DB_ENV *, uint32_t);
  int  (*dbremove) (DB_ENV *, DB_TXN *, const char *, const char *, uint32_t);
  int  (*dbrename) (DB_ENV *, DB_TXN *, const char *, const char *, const char *, uint32_t);
  void (*err) (const DB_ENV *, int, const char *, ...) __attribute__ (( format (printf, 3, 4) ));
  int  (*get_cachesize) (DB_ENV *, uint32_t *, uint32_t *, int *);
  int  (*get_flags) (DB_ENV *, uint32_t *);
  int  (*get_lg_max) (DB_ENV *, uint32_t*);
  int  (*get_lk_max_locks) (DB_ENV *, uint32_t *);
  int  (*log_archive) (DB_ENV *, char **[], uint32_t);
  int  (*log_flush) (DB_ENV *, const DB_LSN *);
  int  (*open) (DB_ENV *, const char *, uint32_t, int);
  int  (*set_cachesize) (DB_ENV *, uint32_t, uint32_t, int);
  int  (*set_data_dir) (DB_ENV *, const char *);
  void (*set_errcall) (DB_ENV *, void (*)(const DB_ENV *, const char *, const char *));
  void (*set_errfile) (DB_ENV *, FILE*);
  void (*set_errpfx) (DB_ENV *, const char *);
  int  (*set_flags) (DB_ENV *, uint32_t, int);
  int  (*set_lg_bsize) (DB_ENV *, uint32_t);
  int  (*set_lg_dir) (DB_ENV *, const char *);
  int  (*set_lg_max) (DB_ENV *, uint32_t);
  int  (*set_lk_detect) (DB_ENV *, uint32_t);
  int  (*set_lk_max_locks) (DB_ENV *, uint32_t);
  int  (*set_tmp_dir) (DB_ENV *, const char *);
  int  (*set_verbose) (DB_ENV *, uint32_t, int);
  int  (*txn_begin) (DB_ENV *, DB_TXN *, DB_TXN **, uint32_t);
  int  (*txn_checkpoint) (DB_ENV *, uint32_t, uint32_t, uint32_t);
  int  (*txn_recover) (DB_ENV *, DB_PREPLIST preplist[/*count*/], long count, /*out*/ long *retp, uint32_t flags);
  int  (*txn_stat) (DB_ENV *, DB_TXN_STAT **, uint32_t);
};
struct __toku_db_key_range {
  double less;
  double equal;
  double greater;
};
struct __toku_db_lsn {
};
struct __toku_dbt {
  void*data;
  uint32_t size;
  uint32_t ulen;
  uint32_t flags;
};
typedef struct __toku_descriptor {
    DBT       dbt;
} *DESCRIPTOR, DESCRIPTOR_S;
//One header is included in 'data'
//One header is included in 'additional for checkpoint'
typedef struct __toku_db_fragmentation {
  uint64_t file_size_bytes;               //Total file size in bytes
  uint64_t data_bytes;                    //Compressed User Data in bytes
  uint64_t data_blocks;                   //Number of blocks of compressed User Data
  uint64_t checkpoint_bytes_additional;   //Additional bytes used for checkpoint system
  uint64_t checkpoint_blocks_additional;  //Additional blocks used for checkpoint system 
  uint64_t unused_bytes;                  //Unused space in file
  uint64_t unused_blocks;                 //Number of contiguous regions of unused space
  uint64_t largest_unused_block;          //Size of largest contiguous unused space
} *TOKU_DB_FRAGMENTATION, TOKU_DB_FRAGMENTATION_S;
struct __toku_db {
  struct __toku_db_internal *i;
#define db_struct_i(x) ((x)->i)
  int (*key_range64)(DB*, DB_TXN *, DBT *, uint64_t *less, uint64_t *equal, uint64_t *greater, int *is_exact);
  int (*stat64)(DB *, DB_TXN *, DB_BTREE_STAT64 *);
  int (*pre_acquire_table_lock)(DB*, DB_TXN*);
  int (*pre_acquire_fileops_lock)(DB*, DB_TXN*);
  const DBT* (*dbt_pos_infty)(void) /* Return the special DBT that refers to positive infinity in the lock table.*/;
  const DBT* (*dbt_neg_infty)(void)/* Return the special DBT that refers to negative infinity in the lock table.*/;
  void (*get_max_row_size) (DB*, uint32_t *max_key_size, uint32_t *max_row_size);
  DESCRIPTOR descriptor /* saved row/dictionary descriptor for aiding in comparisons */;
  DESCRIPTOR cmp_descriptor /* saved row/dictionary descriptor for aiding in comparisons */;
  int (*change_descriptor) (DB*, DB_TXN*, const DBT* descriptor, uint32_t) /* change row/dictionary descriptor for a db.  Available only while db is open */;
  int (*getf_set)(DB*, DB_TXN*, uint32_t, DBT*, YDB_CALLBACK_FUNCTION, void*) /* same as DBC->c_getf_set without a persistent cursor) */;
  int (*optimize)(DB*) /* Run garbage collecion and promote all transactions older than oldest. Amortized (happens during flattening) */;
  int (*hot_optimize)(DB*, int (*progress_callback)(void *progress_extra, float progress), void *progress_extra);
  int (*get_fragmentation)(DB*,TOKU_DB_FRAGMENTATION);
  int (*change_pagesize)(DB*,uint32_t);
  int (*change_readpagesize)(DB*,uint32_t);
  int (*get_readpagesize)(DB*,uint32_t*);
  int (*set_readpagesize)(DB*,uint32_t);
  int (*change_compression_method)(DB*,TOKU_COMPRESSION_METHOD);
  int (*get_compression_method)(DB*,TOKU_COMPRESSION_METHOD*);
  int (*set_compression_method)(DB*,TOKU_COMPRESSION_METHOD);
  int (*set_indexer)(DB*, DB_INDEXER*);
  void (*get_indexer)(DB*, DB_INDEXER**);
  int (*verify_with_progress)(DB *, int (*progress_callback)(void *progress_extra, float progress), void *progress_extra, int verbose, int keep_going);
  int (*update)(DB *, DB_TXN*, const DBT *key, const DBT *extra, uint32_t flags);
  int (*update_broadcast)(DB *, DB_TXN*, const DBT *extra, uint32_t flags);
  void *app_private;
  DB_ENV *dbenv;
  void *api_internal;
  int (*close) (DB*, uint32_t);
  int (*cursor) (DB *, DB_TXN *, DBC **, uint32_t);
  int (*del) (DB *, DB_TXN *, DBT *, uint32_t);
  int (*fd) (DB *, int *);
  int (*get) (DB *, DB_TXN *, DBT *, DBT *, uint32_t);
  int (*get_flags) (DB *, uint32_t *);
  int (*get_pagesize) (DB *, uint32_t *);
  int (*key_range) (DB *, DB_TXN *, DBT *, DB_KEY_RANGE *, uint32_t);
  int (*open) (DB *, DB_TXN *, const char *, const char *, DBTYPE, uint32_t, int);
  int (*put) (DB *, DB_TXN *, DBT *, DBT *, uint32_t);
  void (*set_errfile) (DB *, FILE*);
  int (*set_flags) (DB *, uint32_t);
  int (*set_pagesize) (DB *, uint32_t);
  int (*stat) (DB *, void *, uint32_t);
  int (*verify) (DB *, const char *, const char *, FILE *, uint32_t);
};
struct __toku_db_txn_active {
  uint32_t txnid;
  DB_LSN lsn;
};
typedef struct __toku_txn_progress {
  uint64_t entries_total;
  uint64_t entries_processed;
  uint8_t  is_commit;
  uint8_t  stalled_on_checkpoint;
} *TOKU_TXN_PROGRESS, TOKU_TXN_PROGRESS_S;
typedef void(*TXN_PROGRESS_POLL_FUNCTION)(TOKU_TXN_PROGRESS, void*);
struct txn_stat {
  uint64_t rollback_raw_count;
};
struct __toku_db_txn {
  int (*txn_stat)(DB_TXN *, struct txn_stat **);
  struct toku_list open_txns;
  int (*commit_with_progress)(DB_TXN*, uint32_t, TXN_PROGRESS_POLL_FUNCTION, void*);
  int (*abort_with_progress)(DB_TXN*, TXN_PROGRESS_POLL_FUNCTION, void*);
  int (*xa_prepare) (DB_TXN*, TOKU_XA_XID *);
  uint64_t (*id64) (DB_TXN*);
  DB_ENV *mgrp /*In TokuDB, mgrp is a DB_ENV not a DB_TXNMGR*/;
  DB_TXN *parent;
  void *api_internal;
  int (*abort) (DB_TXN *);
  int (*commit) (DB_TXN*, uint32_t);
  uint32_t (*id) (DB_TXN *);
  int (*prepare) (DB_TXN*, uint8_t gid[DB_GID_SIZE]);
};
struct __toku_db_txn_stat {
  uint32_t st_nactive;
  DB_TXN_ACTIVE *st_txnarray;
};
struct __toku_dbc {
  int (*c_getf_first)(DBC *, uint32_t, YDB_CALLBACK_FUNCTION, void *);
  int (*c_getf_last)(DBC *, uint32_t, YDB_CALLBACK_FUNCTION, void *);
  int (*c_getf_next)(DBC *, uint32_t, YDB_CALLBACK_FUNCTION, void *);
  int (*c_getf_prev)(DBC *, uint32_t, YDB_CALLBACK_FUNCTION, void *);
  int (*c_getf_current)(DBC *, uint32_t, YDB_CALLBACK_FUNCTION, void *);
  int (*c_getf_current_binding)(DBC *, uint32_t, YDB_CALLBACK_FUNCTION, void *);
  int (*c_getf_set)(DBC *, uint32_t, DBT *, YDB_CALLBACK_FUNCTION, void *);
  int (*c_getf_set_range)(DBC *, uint32_t, DBT *, YDB_CALLBACK_FUNCTION, void *);
  int (*c_getf_set_range_reverse)(DBC *, uint32_t, DBT *, YDB_CALLBACK_FUNCTION, void *);
  int (*c_pre_acquire_range_lock)(DBC*, const DBT*, const DBT*);
  DB *dbp;
  int (*c_close) (DBC *);
  int (*c_count) (DBC *, db_recno_t *, uint32_t);
  int (*c_get) (DBC *, DBT *, DBT *, uint32_t);
};
int db_env_create(DB_ENV **, uint32_t) __attribute__((__visibility__("default")));
int db_create(DB **, DB_ENV *, uint32_t) __attribute__((__visibility__("default")));
const char *db_strerror(int) __attribute__((__visibility__("default")));
const char *db_version(int*,int *,int *) __attribute__((__visibility__("default")));
int log_compare (const DB_LSN*, const DB_LSN *) __attribute__((__visibility__("default")));
int db_env_set_func_fsync (int (*)(int)) __attribute__((__visibility__("default")));
int toku_set_trace_file (const char *fname) __attribute__((__visibility__("default")));
int toku_close_trace_file (void) __attribute__((__visibility__("default")));
int db_env_set_func_free (void (*)(void*)) __attribute__((__visibility__("default")));
int db_env_set_func_malloc (void *(*)(size_t)) __attribute__((__visibility__("default")));
int db_env_set_func_realloc (void *(*)(void*, size_t)) __attribute__((__visibility__("default")));
int db_env_set_func_pwrite (ssize_t (*)(int, const void *, size_t, toku_off_t)) __attribute__((__visibility__("default")));
int db_env_set_func_full_pwrite (ssize_t (*)(int, const void *, size_t, toku_off_t)) __attribute__((__visibility__("default")));
int db_env_set_func_write (ssize_t (*)(int, const void *, size_t)) __attribute__((__visibility__("default")));
int db_env_set_func_full_write (ssize_t (*)(int, const void *, size_t)) __attribute__((__visibility__("default")));
int db_env_set_func_fdopen (FILE* (*)(int, const char *)) __attribute__((__visibility__("default")));
int db_env_set_func_fopen (FILE* (*)(const char *, const char *)) __attribute__((__visibility__("default")));
int db_env_set_func_open (int (*)(const char *, int, int)) __attribute__((__visibility__("default")));
int db_env_set_func_fclose (int (*)(FILE*)) __attribute__((__visibility__("default")));
int db_env_set_func_pread (ssize_t (*)(int, void *, size_t, off_t)) __attribute__((__visibility__("default")));
void db_env_set_func_loader_fwrite (size_t (*fwrite_fun)(const void*,size_t,size_t,FILE*)) __attribute__((__visibility__("default")));
void db_env_set_checkpoint_callback (void (*)(void*), void*) __attribute__((__visibility__("default")));
void db_env_set_checkpoint_callback2 (void (*)(void*), void*) __attribute__((__visibility__("default")));
void db_env_set_recover_callback (void (*)(void*), void*) __attribute__((__visibility__("default")));
void db_env_set_recover_callback2 (void (*)(void*), void*) __attribute__((__visibility__("default")));
void db_env_set_loader_size_factor (uint32_t) __attribute__((__visibility__("default")));
void db_env_set_mvcc_garbage_collection_verification(uint32_t) __attribute__((__visibility__("default")));
void db_env_enable_engine_status(uint32_t) __attribute__((__visibility__("default")));
void db_env_set_flusher_thread_callback (void (*)(int, void*), void*) __attribute__((__visibility__("default")));
#if defined(__cplusplus) || defined(__cilkplusplus)
}
#endif
#endif