#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/epoll.h>

#include <systemd/sd-bus.h>
#include <systemd/sd-event.h>

#include "config.h"

#define PLATFORM_PROFILE_FILE "/sys/firmware/acpi/platform_profile"

static sd_bus *bus = NULL;
static uint32_t notification_id = 0;

static void warn(const char *msg, int err) {
	fprintf(stderr, "%s: %s (%d)\n", msg, strerror(err), err);
}

static void send_notification(const char *profile) {
	sd_bus_error error = SD_BUS_ERROR_NULL;
	sd_bus_message *reply;
	int ret = sd_bus_call_method(
			bus,
			"org.freedesktop.Notifications",
			"/org/freedesktop/Notifications",
			"org.freedesktop.Notifications",
			"Notify",
			&error,
			&reply,
			"susssasa{sv}i",
			PROJECT_NAME,
			notification_id,
			"",
			"Power profile",
			profile,
			0,
			0,
			(int32_t) -1
	);
	if (ret < 0) {
		warn("Could not send notification", -ret);
		return;
	}
	if (sd_bus_error_is_set(&error)) {
		fprintf(stderr, "Could not send notification %s %s\n", error.name, error.message);
		sd_bus_error_free(&error);
		return;
	}
	if (!reply)
		return;

	ret = sd_bus_message_read(reply, "u", &notification_id);
	if (ret < 0)
		warn("Could not read notification_id", -ret);
	sd_bus_message_unref(reply);
}

static int platform_profile_updated(sd_event_source *s, int fd_, uint32_t revents, void *userdata) {
	(void) s;
	(void) userdata;
	(void) revents;
	(void) fd_;

	char buf[32];
	int fd = open(PLATFORM_PROFILE_FILE, O_RDONLY);
	if (fd == -1) {
		warn("Could not open platform_profile", errno);
		return 0;
	}

	errno = 0;
	ssize_t r = read(fd, buf, sizeof(buf) - 1);
	close(fd);

	if (r <= 0) {
		warn("Could not read platform_profile", errno);
		return 0;
	}

	buf[r] = '\0';
	if (buf[r - 1] == '\n')
		buf[r - 1] = '\0';

	send_notification(buf);

	return 0;
}

int main() {
	int fd = -1;
	sd_event *event = NULL;
	int ret = sd_event_default(&event);
	if (ret < 0) {
		warn("Could not create event loop", -ret);
		goto err;
	}

	fd = open(PLATFORM_PROFILE_FILE, O_RDONLY);
	if (fd == -1) {
		warn("Could not open platform_profile", errno);
		ret = -errno;
		goto err;
	}

	ret = sd_bus_default_user(&bus);
	if (ret < 0) {
		warn("Could not get user bus", -ret);
		goto err;
	}

	sd_event_source *event_source;
	ret = sd_event_add_io(event, &event_source, fd, EPOLLIN | EPOLLET, platform_profile_updated, NULL);
	if (ret < 0) {
		warn("Could file watch", -ret);
		goto err;
	}

	ret = sd_event_source_set_io_fd_own(event_source, fd);
	if (ret < 0) {
		warn("Could not transfer fd ownership", -ret);
		goto err;
	}
	fd = -1;

	ret = sd_event_loop(event);
	if (ret < 0) {
		warn("Could not start event loop", -ret);
		goto err;
	}

err:
	if (bus)
		sd_bus_unref(bus);

	if (fd != -1)
		close(fd);

	if (event)
		sd_event_unref(event);

	return -ret;
}
