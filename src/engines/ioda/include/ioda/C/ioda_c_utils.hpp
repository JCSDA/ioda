/*
 * (C) Copyright 2022-2023 UCAR
 *
 * This software is licensed under the terms of the Apache Licence Version 2.0
 * which can be obtained at http://www.apache.org/licenses/LICENSE-2.0.
 */
#pragma once
#include <cstdio>
#include <cstdlib>
#include <cerrno>
#include <fcntl.h>
#include <cstdint>
#include <unistd.h>
#include <signal.h>
#include <cstring>
#include <ctime>
#include <stdexcept>

extern "C" {

void set_exit_fun( void (*f)() );

void fatal_error();

char * Strdup(const char *s);

void * Malloc(size_t n);

//#define MALLOC_PTR(type) ( type * ) Malloc( sizeof ( type ) )
//#define MALLOC_ARRAY(type,n) ( type * )Malloc(sizeof(type)*(n))
//#define CHAR_ALLOC(n) (char*) Malloc(n)

void * Calloc(size_t n);

int Open(const char *name,int flgs);

FILE * Fdopen(int fdes,const char *mode);

FILE * Fopen(const char *name,const char *mode);

FILE * Fmemopen(char *b,size_t bsize,const char *mode);

void Write(int fd,const void *buff,size_t sz);

void Read(int fd,void *buff,size_t sz);

void BlockingRead(int fd,void *buff,size_t sz);

void Fwrite(const void *p,size_t osize,size_t cnt,FILE *fp);

void Fread(void *p,size_t osize,size_t cnt,FILE *fp);

void Fseek(FILE *fp,long pos,int whence);

void Pipe(int *fds);

int Fork();

} // end extern "C"

namespace putils {

struct Stopwatch
{
    double acc;
    struct timespec ts;
    struct timespec tf;
    Stopwatch():acc(0.) {}    
    constexpr void clear() noexcept { acc = 0.; }
    void start() { clock_gettime(CLOCK_MONOTONIC,&ts);}
    void stop() { 
        clock_gettime(CLOCK_MONOTONIC,&tf);
        acc += (tf.tv_sec - ts.tv_sec) + 1.e-9 * ( tf.tv_nsec - ts.tv_nsec);
    }
    constexpr double elapsed_time() const noexcept { return acc;}
};

} // end namespace putils
