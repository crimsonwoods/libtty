#include <cstdint>
#include <string>
#include <cstdio>
#include <cstring>
#include <unistd.h>

#include "tty.h"

#define DEF_TTY_DEVICE "/dev/ttyUSB0"

#define PERR(fmt, ...) fprintf(stderr, "error: " fmt, ##__VA_ARGS__)

struct app_args_t{
	std::string device;

	app_args_t() : device(DEF_TTY_DEVICE) {
	}
	~app_args_t() {
	}
};

static void parse_args(app_args_t &args, int argc, char **argv);
static bool validate_args(app_args_t const &args);
static void usage();
static void do_action(tty_handle_t handle);
static int  do_command(tty_handle_t handle, uint8_t motor, uint8_t angle);

int main(int argc, char **argv) {
	app_args_t args;

	parse_args(args, argc, argv);
	if (!validate_args(args)) {
		return -1;
	}

	tty_handle_t handle;
	int ret = tty_open_device(&handle, args.device.c_str());
	if (TTY_ERR_OK != ret) {
		PERR("failed to open TTY device.\n");
		return -1;
	}

	tty_config_t config = {
		4800,
		TTY_DATA_8,
		TTY_PARITY_NONE,
		TTY_STOP_1,
		TTY_FLOW_NONE,
		0,
	};
	ret = tty_set_config(handle, &config);
	if (TTY_ERR_OK != ret) {
		PERR("failed to set configuration.\n");
	} else {
		do_action(handle);
	}

	tty_close_device(handle);

	return 0;
}

static void do_action(tty_handle_t handle) {
	// init: free all motors
	for (int i = 0; i < 4; ++i) {
		do_command(handle, i, 0);
	}

	// move to center position
	for (int i = 0; i < 4; ++i) {
		do_command(handle, i, 110);
	}

	for (int i = 0; i < 4; ++i) {
		for (int a = 1; a <= 220; ++a) {
			do_command(handle, i, a);
		}
	}

	// finish: free all motors
	for (int i = 0; i < 4; ++i) {
		do_command(handle, i, 0);
	}
}

static int  do_command(tty_handle_t handle, uint8_t motor, uint8_t angle) {
	if (motor > 8 || angle > 220) {
		return -1;
	}
	uint8_t command[4] = {
		0xfd, (uint8_t)((0xdd + motor) & 0xff), angle, 0,
	};
	command[3] = (uint8_t)((command[1] - 220 + command[2]) & 0xff);
	return tty_write(handle, command, sizeof(command));
}

static void usage() {
	printf("Usage: ptrb [options]\n");
	printf("[Options]\n");
	printf("  -d device  : path to tty device (default: %s).\n", DEF_TTY_DEVICE);
	exit(0);
}

static void parse_args(app_args_t &args, int argc, char **argv) {
	if ((2 == argc) && (0 == strncmp(argv[1], "--help", 7))) {
		usage();
	}
	int opt;
	while(-1 != (opt = getopt(argc, argv, "d:"))) {
		switch(opt) {
		case 'd':
			args.device = optarg;
			break;
		}
	}
}

static bool validate_args(app_args_t const &args) {
	// nothing to do.
	return true;
}

