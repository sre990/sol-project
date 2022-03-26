/**
 * @brief header file for the communication api between server and client
*/

#ifndef _API_H_
#define _API_H_

#include <sys/time.h>

// if set to true, it will print to stdout
extern bool verbose_mode;
// if set to true, it will exit on fatal errors
extern bool strict_mode;

/**
 * @brief connect a client to the socket given as param
 * @returns 0 on success, -1 on failure.
 * @param sockname must be != NULL with length < 108 (UNIX standard).
 * @param msec must be >= 0.
 * @exception errno is set to EINVAL for invalid params, to EISCONN if the client is already connected
 * to a socket, to EAGAIN if a connection has not been established before abstime.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
 */
int openConnection(const char* sockname, int msec, const struct timespec abstime);

/**
 * @brief closes the connection to the socket.
 * @returns 0 on success, -1 on failure.
 * @param sockname must be != NULL.
 * @exception errno is set to EINVAL for invalid params, to ENOTCONN if client is not connected to the socket.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
 */
int closeConnection(const char* sockname);

/**
 * @brief opens a file with flags.
 * @returns 0 on success, -1 on failure.
 * @param pathname must be != NULL with length < 108 (UNIX standard).
 * @param flags equal to O_CREATE, a new file will be created. flags equal to O_LOCK, the file will be locked.
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message. errno will also be set if there is no space for creating
 * a new file or - if the file is locked - the owner of the lock is not the client calling this function.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int openFile(const char* pathname, int flags);

/**
 * @brief reads a file of a certain size and returns a pointer to ana area allocated on the heap in the buf
 * param.
 * @returns 0 on success, -1 on failure.
 * @param pathname must be != NULL with length < 108 (UNIX standard).
 * @param buf can be NULL if size is NULL.
 * @param size can be NULL if buf is NULL.
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message. errno will also be set if the file has not been
 * already opened by the client.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int readFile(const char* pathname, void** buf, size_t* size);

/**
 * @brief reads N files and saves them to dirname.
 * @returns 0 on success, -1 on failure.
 * @param N if it is 0 or bigger than the number of files in the server, all files will be read.
 * @param dirname == NULL will not store read files inside the cache.
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int readNFiles(int N, const char* dirname);

/**
 * @brief writes a file to server.
 * @returns 0 on success, -1 on failure.
 * @param pathname must be != NULL with length < 108 (UNIX standard).
 * @param dirname == NULL will not store evicted files inside the cache.
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message. errno is also set if the previous operation performed by
 * the client is an openFile with O_CREATE and O_LOCK set.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int writeFile(const char* pathname, const char* dirname);

/**
 * @brief appends 'size' number of bytes inside the buffer to a file inside the server.
 * @returns 0 on success, -1 on failure.
 * @param pathname must be != NULL with length < 108 (UNIX standard).
 * @param dirname == NULL will not store the files evicted inside the cache.
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message. errno will also be set if the file is not opened by
 * the client.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int appendToFile(const char* pathname, void* buf, size_t size, const char* dirname);

/**
 * @brief sets the O_LOCK flag for the requested file.
 * @returns 0 on success, -1 on failure.
 * @param pathname must be != NULL with length < 108 (UNIX standard).
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message. errno is also set if the file is not opened by the
 * client and if another client owns a lock over the file.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int lockFile(const char* pathname);

/**
 * @brief resets the O_LOCK flag for the requested file.
 * @returns 0 on success, -1 on failure.
 * @param pathname must be != NULL with length < 108 (UNIX standard).
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message. errno is also set if the client calling this routine
 * is not the owner of the lock over the requested file.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int unlockFile(const char* pathname);

/**
 * @brief closes the file.
 * @returns 0 on success, -1 on failure.
 * @param pathname must be != NULL with length < 108 (UNIX standard).
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message. errno is also set if the file is not currently opened
 * by the client calling this routine.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int closeFile(const char* pathname);

/**
 * @brief removes a file from server.
 * @returns 0 on success, -1 on failure.
 * @param pathname must be != NULL with length < 108 (UNIX standard).
 * @exception errno is set to EINVAL for invalid params,to ENOTCONN if client is not connected to the socket, to
 * EBADMSG if the socket responds with an invalid message. errno is also set if the client calling this routine
 * is not the file's lock owner.
 * @note strict_mode toggled will exit on fatal errors.
 * verbose_mode toggled will print the operation details to stdout.
*/
int removeFile(const char* pathname);

#endif