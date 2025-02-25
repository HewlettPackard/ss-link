#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/select.h>
#include <getopt.h>
#include <fcntl.h>
#include <time.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

#define VERSION              "1.1.0"

/* Notification */
#define NOTIF_ENV            "SL_TEST_LGRP_DEBUGFS_NOTIFS"
#define NOTIF_SIZE           1024
#define NOTIF_TIMESTAMP_SIZE 64
#define NOTIF_TYPE_SIZE      64 
#define NOTIF_INFO_SIZE      256
#define NOTIF_NUM_FIELDS     7

/* Command options */
#define SHORT_OPTIONS_LEN    32
#define FILTER_NAME_LEN      32

#define HELP_SHORT_OPT       'h'
#define VERSION_SHORT_OPT    'v'
#define TIMEOUT_SHORT_OPT    't'
#define CONTINUOUS_SHORT_OPT 'c'
#define HUMAN_SHORT_OPT      'H'
#define FILTER_SHORT_OPT     'f'
#define REMOVE_SHORT_OPT     'r'
#define EXPECT_SHORT_OPT     'e'

static const char getopt_short_opts[] = {
	HELP_SHORT_OPT,
	VERSION_SHORT_OPT,
	TIMEOUT_SHORT_OPT,
	':',                  /* Has argument */
	CONTINUOUS_SHORT_OPT,
	HUMAN_SHORT_OPT,
	FILTER_SHORT_OPT,
	':',                  /* Has argument */
	REMOVE_SHORT_OPT,
	EXPECT_SHORT_OPT,
	':',                  /* Has argument */
};

static const char *lgrp_notifs_filename;
static int read_timeout = -1; /* Wait indefinitely */
static char filter_name[FILTER_NAME_LEN];
static char expect_name[FILTER_NAME_LEN];

enum {
	OPT_HELP,
	OPT_VERSION,
	OPT_TIMEOUT,
	OPT_CONTINUOUS,
	OPT_HUMAN,
	OPT_FILTER,
	OPT_REMOVE,
	OPT_EXPECT,
	NUM_OPTS,
};

//TODO: Maybe this should be common somewhere
static const char *notif_names[] = {
	"invalid",
	"link-up",
	"link-up-fail",
	"link-async-down",
	"link-error",
	"llr-data",
	"llr-setup-timeout",
	"llr-start-timeout",
	"llr-running",
	"llr-error",
	"media-present",
	"media-not-present",
	"media-error",
	"link-ucw-warn",
	"link-ccw-warn",
	"an-data",
	"an-timeout",
	"an-error",
	"llr-canceled",
	"link-down",
	"unrecognized",
};

static const struct cmd_option {
	const char  short_option;
	const char *long_option;
	const char *description;
	const char *argument_name;
	const int   required;
} cmd_options[] = {
	[OPT_HELP]       = {HELP_SHORT_OPT,       "help",       "This help message.",                           NULL,      no_argument       },
	[OPT_VERSION]    = {VERSION_SHORT_OPT,    "version",    "Print the version information.",               NULL,      no_argument       },
	[OPT_TIMEOUT]    = {TIMEOUT_SHORT_OPT,    "timeout",    "Set the read timeout in milliseconds.",        "TIMEOUT", required_argument },
	[OPT_CONTINUOUS] = {CONTINUOUS_SHORT_OPT, "continuous", "Continuously append notifications to stdout.", NULL,      no_argument       },
	[OPT_HUMAN]      = {HUMAN_SHORT_OPT,      "human",      "Human-readable timestamp.",                    NULL,      no_argument       },
	[OPT_FILTER]     = {FILTER_SHORT_OPT,     "filter",     "Filter notifications by type",                 "FILTER",  required_argument },
	[OPT_REMOVE]     = {REMOVE_SHORT_OPT,     "remove",     "Remove (flush) all current notifications.",    NULL,      no_argument       },
	[OPT_EXPECT]     = {EXPECT_SHORT_OPT,     "expect",     "Expect notifications.",                        "EXPECT",  required_argument },
};

static struct option_desc {
	const struct cmd_option *info;
	int                      flag;
	void                    *argument;
} option_descs[] = {
	[OPT_HELP]       = { &cmd_options[OPT_HELP],        0,  NULL },
	[OPT_VERSION]    = { &cmd_options[OPT_VERSION],     0,  NULL },
	[OPT_TIMEOUT]    = { &cmd_options[OPT_TIMEOUT],     0,  &read_timeout },
	[OPT_CONTINUOUS] = { &cmd_options[OPT_CONTINUOUS],  0,  NULL },
	[OPT_HUMAN]      = { &cmd_options[OPT_HUMAN],       0,  NULL },
	[OPT_FILTER]     = { &cmd_options[OPT_FILTER],      0,  filter_name },
	[OPT_REMOVE]     = { &cmd_options[OPT_REMOVE],      0,  NULL },
	[OPT_EXPECT]     = { &cmd_options[OPT_EXPECT],      0,  expect_name },
};

#define LONG_OPTION_FLAG_ONLY(_opt) { cmd_options[(_opt)].long_option, cmd_options[(_opt)].required, &option_descs[(_opt)].flag, cmd_options[(_opt)].short_option}

/* flag MUST be NULL otherwise getopt_long() won't return which short option was found */
#define LONG_OPTION_WITH_ARG(_opt) { cmd_options[(_opt)].long_option, cmd_options[(_opt)].required, NULL, cmd_options[(_opt)].short_option}

#define SET_OPTION(_opt) option_descs[(_opt)].flag = option_descs[(_opt)].info->short_option
#define OPTION_SET(_opt) (option_descs[(_opt)].flag == option_descs[(_opt)].info->short_option)

static struct option long_options[] = {
	LONG_OPTION_FLAG_ONLY(OPT_HELP),
	LONG_OPTION_FLAG_ONLY(OPT_VERSION),
	LONG_OPTION_WITH_ARG(OPT_TIMEOUT),
	LONG_OPTION_FLAG_ONLY(OPT_CONTINUOUS),
	LONG_OPTION_FLAG_ONLY(OPT_HUMAN),
	LONG_OPTION_WITH_ARG(OPT_FILTER),
	LONG_OPTION_FLAG_ONLY(OPT_REMOVE),
	LONG_OPTION_WITH_ARG(OPT_EXPECT),
	{0, 0, 0, 0}
};

void sigint_handler(int sig) {
	if (sig == SIGINT) {
		exit(EXIT_SUCCESS);
	}
}

int read_notifs(int fd, char *notif, size_t len, int *timeout, bool remove)
{
	int           rtn;
	struct pollfd rfds;
	ssize_t       bytes_read;


	rfds.fd = fd;
	rfds.events = POLLIN;

	/* POLLOUT signifies the notification queue is empty */
	if (remove)
		rfds.events |= POLLOUT;

	rtn = poll(&rfds, 1, *timeout);
	if (rtn == -1) {
		rtn = errno;
		perror("poll failed");
		return rtn;
	} else if (rtn == 0) {
		fprintf(stderr, "timedout\n");
		return ETIMEDOUT;
	}

	if (rfds.revents & POLLIN) {
		bytes_read = read(fd, notif, len);
		if (bytes_read == -1) {
			rtn = errno;
			perror("read failed");
			return rtn;
		}
	} else if (rfds.revents & POLLOUT) {
		return ENODATA;
	} else {
		fprintf(stderr, "wrong event\n");
		return EINVAL;
	}

	return 0;
}

int convert_timestamp_to_local(char *timestamp_ns, char *timestamp_local, size_t len)
{
	int                 rtn;
	struct tm          *local_time;
	unsigned long long  nanoseconds;
	struct timespec     timestamp_ts;
	size_t              count;

	nanoseconds = strtoull(timestamp_ns, NULL, 0);

	timestamp_ts.tv_sec = nanoseconds / 1000000000;
	timestamp_ts.tv_nsec = nanoseconds % 1000000000;

	local_time = localtime(&timestamp_ts.tv_sec);
	if (!local_time) {
		rtn = errno;
		perror("localtime failed");
		return rtn;
	}

	count = strftime(timestamp_local, len, "%F %T", local_time);
	if (count == 0) {
		rtn = errno;
		perror("strftime failed");
		return rtn;
	}

	count = snprintf(&timestamp_local[count], len, ".%05ld", timestamp_ts.tv_nsec);
	if (count == 0) {
		rtn = errno;
		perror("snprintf failed");
		return rtn;
	}
	
	return 0;
}

int parse_notif(char *notif, bool human_readable, char *filter, char *expect)
{
	int            rtn;
	int            count;
	char           notif_timestamp[NOTIF_TIMESTAMP_SIZE];
	char           local_timestamp[NOTIF_TIMESTAMP_SIZE];
	char          *timestamp;
	unsigned int   ldev_num;
	unsigned int   lgrp_num;
	unsigned int   link_num;
	unsigned int   info_map;
	char           type[NOTIF_TYPE_SIZE];
	char           info[NOTIF_INFO_SIZE];

	timestamp = notif_timestamp;

	count = sscanf(notif, "%s %u %u %u %x %s %s", notif_timestamp, &ldev_num, &lgrp_num, &link_num, &info_map, type, info);
	if (count != NOTIF_NUM_FIELDS) {
		fprintf(stderr, "parse failure (count = %d)\n", count);
		return EINVAL;
	}

	if (human_readable) {
		rtn = convert_timestamp_to_local(notif_timestamp, local_timestamp, sizeof(local_timestamp));
		if (rtn) {
			fprintf(stderr, "convert_timestamp_to_local failed [%d]\n", rtn);
			return rtn;
		}
		timestamp = local_timestamp;
	}

	if (filter && strncmp(type, filter, strnlen(type, NOTIF_TYPE_SIZE))) {
		fprintf(stderr, "%s %u %u %u 0x%X %s %s\n", timestamp, ldev_num, lgrp_num, link_num, info_map, type, info);
		return EAGAIN;
	}

	if (expect) {
		if (strncmp(type, expect, strnlen(type, NOTIF_TYPE_SIZE)) == 0) {
			printf("%s %u %u %u 0x%X %s %s\n", timestamp, ldev_num, lgrp_num, link_num, info_map, type, info);
			return 0;
		} else {
			fprintf(stderr, "%s %u %u %u 0x%X %s %s\n", timestamp, ldev_num, lgrp_num, link_num, info_map, type, info);
			return ENOENT;
		}
	}

	printf("%s %u %u %u 0x%X %s %s\n", timestamp, ldev_num, lgrp_num, link_num, info_map, type, info);
	return 0;
}

void print_help(char *name)
{
	int                      opt;
	size_t                   i;
	const struct cmd_option *o;

	printf("usage: %s ", name);

	for (opt = 0; opt < NUM_OPTS; ++opt) {
		o = option_descs[opt].info;

		if (o->argument_name)
			printf("[-%c %s] ", o->short_option, o->argument_name);
		else
			printf("[-%c] ", o->short_option);
	}

	printf("\n\nRead sl notifications\n\noptional arguments:\n");

	for (opt = 0; opt < NUM_OPTS; ++opt) {
		o = option_descs[opt].info;
		printf("  -%c, --%-10s %-10s %s\n", o->short_option, o->long_option,
			o->argument_name ? o->argument_name : " ", o->description);
	}

	printf("\nfilters:\n");
	for (i = 0; i < (sizeof(notif_names) / sizeof(notif_names[0])); ++i)
		printf("%s\n", notif_names[i]);
}

int main(int argc, char *argv[])
{
	int     rtn;
	int     opt;
	int     options_index;
	int     fd;
	char    notif[NOTIF_SIZE];
	bool    again;
	size_t  i;
	char   *expect;
	char   *filter;

	options_index = 0;
	expect = NULL;
	filter = NULL;

	if (signal(SIGINT, sigint_handler) == SIG_ERR) {
		rtn = errno;
		perror("signal failed");
		exit(rtn);
	}

	while(1) {
		options_index = 0;
		opt = getopt_long(argc, argv, getopt_short_opts, long_options, &options_index);

		if (opt == -1)
			break;

		/* long option sets flag */
		if (opt == 0)
			continue;

		switch(opt) {
		case HELP_SHORT_OPT:
			SET_OPTION(OPT_HELP);
			break;
		case VERSION_SHORT_OPT:
			SET_OPTION(OPT_VERSION);
			break;
		case TIMEOUT_SHORT_OPT:
			SET_OPTION(OPT_TIMEOUT);
			read_timeout = strtoull(optarg, NULL, 0);
			break;
		case CONTINUOUS_SHORT_OPT:
			SET_OPTION(OPT_CONTINUOUS);
			break;
		case HUMAN_SHORT_OPT:
			SET_OPTION(OPT_HUMAN);
			break;
		case FILTER_SHORT_OPT:
			SET_OPTION(OPT_FILTER);

			strncpy(filter_name, optarg, (FILTER_NAME_LEN - 1));

			for (i = 0; i < (sizeof(notif_names) / sizeof(notif_names[0])); ++i) {
				rtn = strncmp(filter_name, notif_names[i], strnlen(filter_name, FILTER_NAME_LEN));
				if (rtn == 0)
					break;
			}

			if (rtn != 0) {
				fprintf(stderr, "strncmp failed [%d]\n", rtn);
				fprintf(stderr, "notif not available.\n");
				print_help(argv[0]);
				return EINVAL;
			}

			filter = option_descs[OPT_FILTER].argument;

			break;
		case REMOVE_SHORT_OPT:
			SET_OPTION(OPT_REMOVE);
			break;
		case EXPECT_SHORT_OPT:
			SET_OPTION(OPT_EXPECT);

			strncpy(expect_name, optarg, (FILTER_NAME_LEN - 1));

			for (i = 0; i < (sizeof(notif_names) / sizeof(notif_names[0])); ++i) {
				rtn = strncmp(expect_name, notif_names[i], strnlen(expect_name, FILTER_NAME_LEN));
				if (rtn == 0)
					break;
			}

			if (rtn != 0) {
				fprintf(stderr, "strncmp failed [%d]\n", rtn);
				fprintf(stderr, "notif not available.\n");
				print_help(argv[0]);
				return EINVAL;
			}

			expect = option_descs[OPT_EXPECT].argument;

			break;
		default:
			fprintf(stderr, "invalid option (opt = %c)\n", opt);
			print_help(argv[0]);
			exit(EXIT_FAILURE);
		}
	}

	if (filter && expect) {
		if (strncmp(expect, filter, strnlen(expect, FILTER_NAME_LEN))) {
			fprintf(stderr, "mismatch (expect = %s, filter = %s)", expect, filter);
			exit(EINVAL);
		}
	}

	if (OPTION_SET(OPT_HELP)) {
		print_help(argv[0]);
		exit(EXIT_SUCCESS);
	}

	if (OPTION_SET(OPT_VERSION)) {
		printf("Version: %s\n", VERSION);
		exit(EXIT_SUCCESS);
	}

	lgrp_notifs_filename = getenv(NOTIF_ENV);

	if (!lgrp_notifs_filename) {
		fprintf(stderr, "%s undefined\n", NOTIF_ENV);
		fprintf(stderr, "source /usr/bin/sl_test_scripts/sl_test_env.sh\n");
		exit(EINVAL);
	}

	fd = open(lgrp_notifs_filename, O_RDONLY);
	if (fd == -1) {
		rtn = errno;
		perror("open failed");
		exit(rtn);
	}

	again = OPTION_SET(OPT_CONTINUOUS) || OPTION_SET(OPT_REMOVE);
	do {
		rtn = read_notifs(fd, notif, NOTIF_SIZE, option_descs[OPT_TIMEOUT].argument, OPTION_SET(OPT_REMOVE));
		switch (rtn) {
		case 0:
			break;
		case ENODATA:
			rtn = 0;
			again = false;
			continue;
		default:
			fprintf(stderr, "notifs_read failed [%d]\n", rtn);
			goto out;
		}

		rtn = parse_notif(notif, OPTION_SET(OPT_HUMAN), filter, expect);
		switch (rtn) {
		case 0:
			break;
		case ENOENT:
			again = false;
			break;
		case EAGAIN:
			again = true;
			break;
		default:
			fprintf(stderr, "parse_notif failed [%d]\n", rtn);
			goto out;
		}

	} while (OPTION_SET(OPT_CONTINUOUS) || again);

out:
       close(fd);
       exit(rtn);
}

