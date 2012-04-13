#ifndef LIB_TTY_H_
#define LIB_TTY_H_

#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void* tty_handle_t;

#define INVALID_TTY_HANDLE NULL

enum TTY_ERRORS {
	TTY_ERR_OK,
	TTY_ERR_INVALID_ARGUMENT,
	TTY_ERR_NO_PERMISSION,
	TTY_ERR_IO_ERROR,
	TTY_ERR_INSUFFICIENT_MEMORY,
	TTY_ERR_INTERRUPTED,
	TTY_ERR_UNKNOWN,
	COUNT_OF_TTY_ERRORS,
};

typedef struct tty_config_t_ {
	int baud;       // baudrate
	int data;       // data bit width : 7 or 8
	int parity;     // parity bit     : 0=none, 1=odd, 2=even
	int stop;       // stop bit       : 1 or 2
	int flow;       // flow control   : 0=none, 1=hardware
	unsigned flags; // flags
} tty_config_t;

#define TTY_DATA_7        7
#define TTY_DATA_8        8

#define TTY_PARITY_NONE   0
#define TTY_PARITY_ODD    1
#define TTY_PARITY_EVEN   2

#define TTY_STOP_1        1
#define TTY_STOP_2        2

#define TTY_FLOW_NONE     0
#define TTY_FLOW_HARDWARE 1

extern int  tty_open_device(tty_handle_t *handle, char const * const path);
extern void tty_close_device(tty_handle_t handle);
extern int  tty_set_config(tty_handle_t handle, tty_config_t const *config);
extern int  tty_get_config(tty_handle_t handle, tty_config_t *config);
extern int  tty_write(tty_handle_t handle, void const *buf, size_t size);
extern int  tty_read(tty_handle_t handle, void *buf, size_t size);
extern char const * tty_get_error_string(int err);

#ifdef __cplusplus
}
#endif

#endif
