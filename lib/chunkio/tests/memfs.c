/* -*- Mode: C; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 4 -*- */

/*  Chunk I/O
 *  =========
 *  Copyright 2018 Eduardo Silva <eduardo@monkey.io>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <sys/mman.h>
#include <arpa/inet.h>

#include <chunkio/chunkio.h>
#include <chunkio/cio_log.h>
#include <chunkio/cio_scan.h>
#include <chunkio/cio_file.h>
#include <chunkio/cio_memfs.h>
#include <chunkio/cio_meta.h>
#include <chunkio/cio_stream.h>
#include <chunkio/cio_utils.h>

#include "cio_tests_internal.h"

#define CIO_ENV           "/tmp/cio-fs-test/"
#define CIO_FILE_400KB    CIO_TESTS_DATA_PATH "/data/400kb.txt"

/* Logging callback, once called it just turn on the log_check flag */
static int log_cb(struct cio_ctx *ctx, const char *file, int line,
                  char *str)
{
    (void) ctx;

    printf("[cio-test-fs] %-60s => %s:%i\n",  str, file, line);
    return 0;
}

/* Test API generating files to the file system and then scanning them back */
static void test_memfs_write()
{
    int i;
    int ret;
    int n_files = 100;
    int flags;
    char *in_data;
    size_t in_size;
    char tmp[255];
    struct cio_ctx *ctx;
    struct cio_stream *stream;
    struct cio_chunk *chunk;
    struct cio_chunk **carr;

    /* Dummy break line for clarity on acutest output */
    printf("\n");

    flags = CIO_CHECKSUM;

    /* cleanup environment */
    cio_utils_recursive_delete(CIO_ENV);

    /* Create main context */
    ctx = cio_create(CIO_ENV, log_cb, CIO_INFO, flags);
    TEST_CHECK(ctx != NULL);

    /* Try to create a file with an invalid stream */
    chunk = cio_chunk_open(ctx, NULL, "invalid", 0, 0);
    TEST_CHECK(chunk == NULL);

    /* Check invalid stream */
    stream = cio_stream_create(ctx, "", CIO_STORE_MEM);
    TEST_CHECK(stream == NULL);

    /* Another invalid name */
    stream = cio_stream_create(ctx, "/", CIO_STORE_MEM);
    TEST_CHECK(stream == NULL);

    /* Create valid stream */
    stream = cio_stream_create(ctx, "test-write", CIO_STORE_MEM);
    TEST_CHECK(stream != NULL);

    /*
     * Load sample data file and with the same content through multiple write
     * operations generating other files.
     */
    ret = cio_utils_read_file(CIO_FILE_400KB, &in_data, &in_size);
    TEST_CHECK(ret == 0);
    if (ret == -1) {
        cio_destroy(ctx);
        exit(EXIT_FAILURE);
    }

    /* Number of test files to create */
    n_files = 100;

    /* Allocate files array */
    carr = calloc(1, sizeof(struct cio_chunk) * n_files);
    if (!carr) {
        perror("calloc");
        exit(EXIT_FAILURE);
    }


    for (i = 0; i < n_files; i++) {
        snprintf(tmp, sizeof(tmp), "api-test-%04i.txt", i);
        carr[i] = cio_chunk_open(ctx, stream, tmp, CIO_OPEN, 1000000);

        if (carr[i] == NULL) {
            continue;
        }

        cio_chunk_write(carr[i], in_data, in_size);
        cio_chunk_write(carr[i], in_data, in_size);

        /* update metadata */
        //cio_meta_write(farr[i], tmp, len);

        /* continue appending data to content area */
        cio_chunk_write(carr[i], in_data, in_size);
        cio_chunk_write(carr[i], in_data, in_size);
        cio_chunk_write(carr[i], in_data, in_size);
    }

    /* Release file data and destroy context */
    free(carr);
    munmap(in_data, in_size);

    cio_scan_dump(ctx);
    cio_destroy(ctx);
}

TEST_LIST = {
    {"memfs_write",   test_memfs_write},
    { 0 }
};
