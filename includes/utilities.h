/**
 * @brief Header for utilities such as readn and writen.
 *
*/

#ifndef _UTILITIES_H_
#define _UTILITIES_H_

#include <errno.h>
#include <linux/limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <error_handlers.h>
#include <defines.h>
#include <api.h>

/**
 * @brief writes n bytes to file descriptor and copies them to ptr
 * @note taken from http://didawiki.di.unipi.it/doku.php/informatica/sol/laboratorio21/esercitazionib/readnwriten
 */
static inline int readn(long fd, void* ptr, size_t n){
   size_t   nleft;
   ssize_t  nread;

   nleft = n;
   while (nleft > 0) {
      if((nread = read(fd, ptr, nleft)) < 0) {
         if (nleft == n) return -1; /* error, return -1 */
         else break; /* error, return amount read so far */
      } else if (nread == 0) break; /* EOF */
      nleft -= nread;
      ptr   += nread;
   }
   return(n - nleft); /* return >= 0 */
}

/**
 * @brief writes n bytes to file descriptor copying the from ptr
 * @note taken from http://didawiki.di.unipi.it/doku.php/informatica/sol/laboratorio21/esercitazionib/readnwriten
 */
static inline int writen(long fd, void* ptr, size_t n){
      size_t   nleft;
      ssize_t  nwritten;

      nleft = n;
      while (nleft > 0) {
         if((nwritten = write(fd, ptr, nleft)) < 0) {
            if (nleft == n) return -1; /* error, return -1 */
            else break; /* error, return amount written so far */
         } else if (nwritten == 0) break;
         nleft -= nwritten;
         ptr   += nwritten;
      }
      return(n - nleft); /* return >= 0 */
   }

/**
 * @brief checks if the string is a number.
 * @returns 0 on success, 1 on failure, to for under/overflow.
 * @note taken from http://didawiki.di.unipi.it/doku.php/informatica/sol/laboratorio21
*/
static inline int isNumber(const char* s, long* n){
   if (!n || !s || strlen(s) == 0){
      errno = EINVAL;
      return 1;
   }
   char* e = NULL;
   errno = 0;
   long val = strtol(s, &e, 10);
   if (errno == ERANGE) return 2;
   if (e != NULL && *e == (char)0){
      *n = val;
      return 0;
   }
   return 1;
}

/**
 * @brief mimics the mkdir -p command in C.
 * @returns 0 on success, -1 on failure.
 * @param path must be != NULL or longer than system-defined PATH_MAX.
 * @exception errno is set to EINVAL for invalid params, to ENAMETOOLONG if the length of the path
 * is > 4096.
 * @note taken from: https://gist.github.com/JonathonReinhart/8c0d90191c38af2dcadb102c4e202950
*/
static inline int mkdir_p(const char *path){
   if (!path){
      errno = EINVAL;
      return -1;
   }
   const size_t len = strlen(path);
   char _path[PATH_MAX];
   char *p;
   errno = 0;
   /* Copy string so its mutable */
   if (len > sizeof(_path)-1){
      errno = ENAMETOOLONG;
      return -1;
   }
   strcpy(_path, path);

   // parses the string and truncates at '/', creating directories
   for (p = _path + 1; *p; p++){
      if (*p == '/'){
         *p = '\0';
         //if the directory already exists, ignore the error
         if (mkdir(_path, S_IRWXU) != 0){
            if (errno != EEXIST)
               return -1;
         }
         *p = '/';
      }
   }
   //creates the last, innermost, directory
   //if it exists already, ignore the error
   if (mkdir(_path, S_IRWXU) != 0){
      if (errno != EEXIST)
         return -1;
   }

   return 0;
}

/**
 * @brief saves the contents to path.
 * @returns 0 on success, -1 on failure.
 * @param path must be != NULL.
 * @param contents must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOMEM for malloc failure.
*/
static inline int save_file(const char* path, const char* contents){
   if (!path || !contents){
      errno = EINVAL;
      return -1;
   }
   int len = strlen(path);
   char* tmp_path = (char*) malloc(sizeof(char) * (len + 1));
   if (!tmp_path){
      errno = ENOMEM;
      return -1;
   }
   memset(tmp_path, 0, len + 1);
   strcpy(tmp_path, path);
   //pointer to the last occurence of '/'
   char* tmp = strrchr(tmp_path, '/');
   if (tmp) *tmp = '\0';
   //creates a directory
   if (mkdir_p(tmp_path) != 0){
      free(tmp_path);
      return -1;
   }
   //opens the file for reading and writing
   FILE* file = fopen(path, "w+");
   if (!file){
      free(tmp_path);
      return -1;
   }
   free(tmp_path);

   fprintf(file, "%s", contents);
   fclose(file);

   return 0;
}

/**
 * @brief checks whether the file located at pathname is a regular file.
 * @returns 1 if it is a regular file, 0 otherwise on success; -1 on failure.
 * @param path must be != NULL.
 * @exception errno is set to EINVAL for invalid params.
*/
static inline int is_file(const char *path){
   if (!path){
      errno = EINVAL;
      return -1;
   }
   struct stat stat_buf;
   // gets attributes for the file located at path
   // and puts them in stat_buf
   int err = stat(path, &stat_buf);
   if (err != 0) return -1;
   if (S_ISREG(stat_buf.st_mode))
      return 1;
   return 0;
}
/**
 * @brief utility for handling failure within the api, it takes a failing operation with
 * its arguments and prints the outcome to stdout
 * @returns -1 always
 * @param op_str is the name of the operation
 * @param flags if any
 * @param N number of files to be read, if applicable
 * @param path to file, if applicable
*/
static inline int fail_with(const char* op_str, int flags, int N,
                            const char* path, char* err_str, int err){
   strerror_r(err, err_str, REQ_LEN_MAX);
   if(strcmp(op_str,OPEN_CONN)||strcmp(op_str,CLOSE_CONN)){
      PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", FAILURE,op_str, path, err_str);
   }else if (strcmp(op_str,READ_N_FILES)){
      PRINT_IF(verbose_mode, "%s-> %s %d %s with errno = %s.\n", FAILURE,op_str, N, path, err_str);
   }else if(strcmp(op_str,WRITE_FILE) || strcmp(op_str,APPEND_TO_FILE)) {
      PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", FAILURE, op_str, path, err_str);
   }else if(strcmp(op_str,OPEN_FILE)){
      PRINT_IF(verbose_mode, "%s-> %s %s %d with errno = %s.\n", FAILURE,op_str, path, flags, err_str);
   }else{
      PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", FAILURE,op_str, path, err_str);
   }
   errno = err;
   return -1;
}

/**
 * @brief utility for handling fatal errors within the api, it takes an operation that caused
 * a fatal error with its arguments and prints the outcome to stdout
 * @returns -1 if strict_mode is false
 * @param op_str is the name of the operation
 * @param flags if any
 * @param N number of files to be read, if applicable
 * @param path to file, if applicable
 * @note strict_mode toggled exits with errno
*/
static inline int abort_with(const char* op_str, int flags, int N,
                             const char* path, char* err_str, int err) {
   strerror_r(err, err_str, REQ_LEN_MAX);
   if(strcmp(op_str,OPEN_CONN)||strcmp(op_str,CLOSE_CONN)){
      PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", EXIT_FATAL,op_str, path, err_str);
   }else if (strcmp(op_str,READ_N_FILES)){
      PRINT_IF(verbose_mode, "%s-> %s %d %s with errno = %s.\n", EXIT_FATAL,op_str, N, path, err_str);
   }else if(strcmp(op_str,WRITE_FILE) || strcmp(op_str, APPEND_TO_FILE)) {
      PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", EXIT_FATAL, op_str, path, err_str);
   }else if(strcmp(op_str,OPEN_FILE)){
      PRINT_IF(verbose_mode, "%s-> %s %s %d with errno = %s.\n", EXIT_FATAL,op_str, path, flags, err_str);
   }else{
      PRINT_IF(verbose_mode, "%s-> %s %s with errno = %s.\n", EXIT_FATAL,op_str, path, err_str);
   }
   errno = err;
   if (strict_mode) exit(errno);
   else return -1;
}

/**
 * @brief utility for handling successful operations within the api, it takes an operation with
 * its arguments and prints the successful outcome to stdout
 * @returns 0 always
 * @param op_str is the name of the operation
 * @param flags if any
 * @param N number of files to be read, if applicable
 * @param path to file, if applicable
 *
*/
static inline int succeed_with(const char* op_str, int flags, int N,
                               const char* path){
   if(strcmp(op_str,OPEN_CONN)||strcmp(op_str,CLOSE_CONN)){
      PRINT_IF(verbose_mode, "%s-> %s %s.\n", SUCCESS,op_str, path);
   }else if (strcmp(op_str,READ_N_FILES)){
      PRINT_IF(verbose_mode, "%s-> %s %d %s.\n", SUCCESS,op_str, N, path);
   }else if(strcmp(op_str,WRITE_FILE) || strcmp(op_str,APPEND_TO_FILE)) {
      PRINT_IF(verbose_mode, "%s-> %s %s.\n", SUCCESS, op_str, path);
   }else if(strcmp(op_str,OPEN_FILE)){
      PRINT_IF(verbose_mode, "%s-> %s %s %d.\n", SUCCESS,op_str, path, flags);
   }else {
      PRINT_IF(verbose_mode, "%s-> %s %s.\n", SUCCESS,op_str, path);
   }
   return 0;
}

#endif