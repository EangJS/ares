#ifndef HTTP_TIME_CLIENT_H
#define HTTP_TIME_CLIENT_H

#ifdef __cplusplus
extern "C" {
#endif

/*============================================================
 *  Public Function Prototypes
 *============================================================*/

/**
 * @brief Fetch current date and time from an HTTP server
 *
 * Sends an HTTP HEAD request and prints the Date header.
 *
 * @return 0 on success, -1 on failure
 */
int http_client(char* time_str);

#ifdef __cplusplus
}
#endif

#endif /* HTTP_TIME_CLIENT_H */
