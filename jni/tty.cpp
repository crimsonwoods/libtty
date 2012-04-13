#include "tty.h"
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>
#include <errno.h>
#include <cstdlib>
#include <cstring>

#include <android/log.h>

#define LOG_TAG "libtty"
#define LOGE(fmt, ...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGD(fmt, ...) __android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, fmt, ##__VA_ARGS__)
#define LOGI(fmt, ...) __android_log_print(ANDROID_LOG_INFO,  LOG_TAG, fmt, ##__VA_ARGS__)

struct tty_dev_t {
	int          fd_;
	tty_config_t config_;

	tty_dev_t(int fd) : fd_(fd), config_() {
		config_.baud   = 0;
		config_.data   = TTY_DATA_8;
		config_.parity = TTY_PARITY_NONE;
		config_.stop   = TTY_STOP_1;
		config_.flow   = TTY_FLOW_NONE;
		config_.flags  = 0;
	}

	~tty_dev_t() {
		if (0 <= fd_) {
			close(fd_);
			fd_ = -1;
		}
	}
};

#define TTYDEV(h) static_cast<tty_dev_t*>(handle)
#define TTYCDEV(h) const_cast<tty_dev_t const*>(TTYDEV(h))

static inline tcflag_t to_baudbits(int baud);
static inline int to_baudnum(tcflag_t baud);
static inline int to_ttyerror(int err);

int tty_open_device(tty_handle_t *handle, char const * const path) {
	if (NULL == handle) {
		return TTY_ERR_INVALID_ARGUMENT;
	}
	*handle = INVALID_TTY_HANDLE;

	int fd = open(path, O_RDWR | O_NOCTTY);
	if (0 > fd) {
		LOGE("failed to open device (%s).", strerror(errno));
		return to_ttyerror(errno);
	}

	tty_dev_t *dev = new tty_dev_t(fd);
	if (NULL == handle) {
		close(fd);
		LOGE("failed to allocate memory (%s).", strerror(errno));
		return TTY_ERR_INSUFFICIENT_MEMORY;
	}

	*handle = dev;

	tty_config_t def_config = {
		4800,
		TTY_DATA_8,
		TTY_PARITY_NONE,
		TTY_STOP_1,
		TTY_FLOW_NONE,
		0,
	};
	(void)tty_set_config(*handle, &def_config);

	return TTY_ERR_OK;
}

void tty_close_device(tty_handle_t handle) {
	if (NULL == handle) {
		return;
	}
	tty_dev_t *dev = TTYDEV(handle);
	delete dev;
}

int  tty_set_config(tty_handle_t handle, tty_config_t const *config) {
	if (NULL == handle || NULL == config) {
		return TTY_ERR_INVALID_ARGUMENT;
	}

	tty_dev_t *dev = TTYDEV(handle);
	termios attr = { 0, };

	tcflag_t baud = to_baudbits(config->baud);

	attr.c_iflag = 0;
	attr.c_oflag = 0;
	attr.c_cflag = baud | CREAD;
	attr.c_lflag = 0;

	if (config->data == TTY_DATA_7) {
		attr.c_cflag |= CS7;
	} else if (config->data == TTY_DATA_8){
		attr.c_cflag |= CS8;
	} else {
		LOGE("unknown data bit width.");
		return TTY_ERR_INVALID_ARGUMENT;
	}
	if (config->parity == TTY_PARITY_NONE) {
		attr.c_iflag |= IGNPAR;
	} else {
		attr.c_iflag |= INPCK;
		attr.c_cflag |= PARENB;
		if (config->parity == TTY_PARITY_ODD) {
			attr.c_cflag |= PARODD;
		} else if (config->parity != TTY_PARITY_EVEN) {
			LOGE("unknown parity bit option.");
			return TTY_ERR_INVALID_ARGUMENT;
		}
	}
	if (config->stop == TTY_STOP_2) {
		attr.c_cflag |= CSTOPB;
	} else if (config->stop != TTY_STOP_1) {
		LOGE("unknown stop bit option.");
		return TTY_ERR_INVALID_ARGUMENT;
	}
	if (config->flow == TTY_FLOW_HARDWARE) {
		attr.c_cflag |= CRTSCTS;
	} else if (config->flow == TTY_FLOW_NONE) {
		attr.c_cflag |= CLOCAL;
	} else {
		LOGE("unknown flow control option.");
		return TTY_ERR_INVALID_ARGUMENT;
	}

	if (0 > tcsetattr(dev->fd_, TCSADRAIN, &attr)) {
		LOGE("failed to set terminal attributes (%s).", strerror(errno));
		return to_ttyerror(errno);
	}

	dev->config_ = *config;

	return TTY_ERR_OK;
}

int  tty_get_config(tty_handle_t handle, tty_config_t *config) {
	if (NULL == handle || NULL == config) {
		return TTY_ERR_INVALID_ARGUMENT;
	}
	tty_dev_t const *dev = TTYCDEV(handle);
	*config = dev->config_;
	return TTY_ERR_OK;
}

int tty_write(tty_handle_t handle, void const *buf, size_t size) {
	if (NULL == handle || NULL == buf) {
		return TTY_ERR_INVALID_ARGUMENT;
	}
	tty_dev_t const *dev = TTYCDEV(handle);
	return write(dev->fd_, buf, size);
}

int tty_read(tty_handle_t handle, void *buf, size_t size) {
	if (NULL == handle || NULL == buf) {
		return TTY_ERR_INVALID_ARGUMENT;
	}
	tty_dev_t const *dev = TTYCDEV(handle);
	return read(dev->fd_, buf, size);
}

static char const *ERROR_MESSAGES[COUNT_OF_TTY_ERRORS] = {
	"no error", // TTY_ERR_OK
	"inavlid argument",
	"operation not permitted",
	"I/O error",
	"insufficient memory",
	"interrupted",
	"unknown error",
};

char const * tty_get_error_string(int err) {
	if ((err < 0) || (err >= COUNT_OF_TTY_ERRORS)) {
		return "unknown error";
	}
	return ERROR_MESSAGES[err];
}

struct baud_entry_t {
	int ibaud;
	tcflag_t tbaud;
};

static const baud_entry_t BAUDRATES[] = {
	{      0, B0      },
	{   1200, B1200   },
	{   1800, B1800   },
	{   2400, B2400   },
	{   4800, B4800   },
	{   9600, B9600   },
	{  19200, B19200  },
	{  38400, B38400  },
	{  57600, B57600  },
	{ 115200, B115200 },
	{     -1, 0       }, // sentinel
};

static inline tcflag_t to_baudbits(int baud) {
	tcflag_t ret = B0;
	int min = INT_MAX;
	for (int i = 0; 0 <= BAUDRATES[i].ibaud; ++i) {
		int const diff = std::abs(BAUDRATES[i].ibaud - baud);
		if (min > diff) {
			min = diff;
			ret = BAUDRATES[i].tbaud;
		}
		if (0 == diff) {
			break;
		}
	}
	return ret;
}

static inline int to_baudnum(tcflag_t baud) {
	int ret = -1;
	baud &= CBAUD;
	for (int i = 0; 0 < BAUDRATES[i].ibaud; ++i) {
		if (baud == BAUDRATES[i].tbaud) {
			ret = BAUDRATES[i].ibaud;
			break;
		}
	}
	return ret;
}

static inline int to_ttyerror(int err) {
	switch(err) {
	case EINTR:  return TTY_ERR_INTERRUPTED;
	case EINVAL: return TTY_ERR_INVALID_ARGUMENT;
	case EPERM:  return TTY_ERR_NO_PERMISSION;
	case EIO:    return TTY_ERR_IO_ERROR;
	case ENOMEM: return TTY_ERR_INSUFFICIENT_MEMORY;
	default:     return TTY_ERR_UNKNOWN;
	}
}

