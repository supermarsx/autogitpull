include(CheckCSourceCompiles)

# Try Linux/glibc style: st_mtim.tv_nsec
set(_SRC_LINUX "
#define _GNU_SOURCE 1
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
int main(void){
  struct stat s; (void)s.st_mtim.tv_nsec; return 0;
}")
check_c_source_compiles("${_SRC_LINUX}" HAVE_STAT_NSEC_ST_MTIM)

# Try Darwin/BSD style: st_mtimespec.tv_nsec
set(_SRC_DARWIN "
#define _DARWIN_C_SOURCE 1
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
int main(void){
  struct stat s; (void)s.st_mtimespec.tv_nsec; return 0;
}")
check_c_source_compiles("${_SRC_DARWIN}" HAVE_STAT_NSEC_ST_MTIMESPEC)

# Try legacy: st_mtimensec
set(_SRC_LEGACY "
#include <sys/types.h>
#include <sys/stat.h>
#include <time.h>
int main(void){
  struct stat s; (void)s.st_mtimensec; return 0;
}")
check_c_source_compiles("${_SRC_LEGACY}" HAVE_STAT_NSEC_ST_MTIMENSEC)

# Decide
if(HAVE_STAT_NSEC_ST_MTIM)
  set(STAT_NSEC_FIELD "st_mtim.tv_nsec" PARENT_SCOPE)
elseif(HAVE_STAT_NSEC_ST_MTIMESPEC)
  set(STAT_NSEC_FIELD "st_mtimespec.tv_nsec" PARENT_SCOPE)
elseif(HAVE_STAT_NSEC_ST_MTIMENSEC)
  set(STAT_NSEC_FIELD "st_mtimensec" PARENT_SCOPE)
else()
  set(STAT_NSEC_FIELD "" PARENT_SCOPE)
endif()

