#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <unistd.h>

#include "common/page.h"
#include "cr_options.h"
#include "log.h"
#include "servicefd.h"
#include "kpage-share.h"

bool kps_enabled;

static unsigned long kps_calls, kps_mapped, kps_short[KPS_R_MAX];

int kpage_share_init(void)
{
	int fd;

	if (!opts.kpage_share)
		return 0;
	fd = open(KPS_DEVICE, O_RDWR | O_CLOEXEC);
	if (fd < 0) {
		pr_warn("kpage_share: can't open %s, pages are copied instead: %s\n", KPS_DEVICE,
			strerror(errno));
		return 0;
	}
	fd = install_service_fd(KPS_FD_OFF, fd);
	if (fd < 0)
		return -1;
	kps_enabled = true;
	pr_info("kpage_share: using %s\n", KPS_DEVICE);
	return 0;
}

long kpage_share_map(int fd, off_t off, void *addr, size_t len)
{
	struct iovec iov = { addr, len };
	struct kps_mapv m = {
		.fd = fd,
		.off = off,
		.iov = (uintptr_t)&iov,
		.nr_iov = 1,
	};

	if (!kps_enabled)
		return 0;
	kps_calls++;
	if (ioctl(get_service_fd(KPS_FD_OFF), KPS_MAPV, &m) < 0 && !m.done) {
		pr_debug("kpage_share: mapv %zu bytes at %p off %lld: %s (reason %u)\n", len, addr,
			 (long long)off, strerror(errno), m.reason);
		m.done = 0;
	}
	if (m.reason < KPS_R_MAX)
		kps_short[m.reason]++;
	kps_mapped += m.done >> PAGE_SHIFT;
	return m.done;
}

void kpage_share_log_stats(void)
{
	static const char *const names[KPS_R_MAX] = {
		"ok", "nospc", "noent", "stale", "vma", "busy", "nomem", "io", "fault", "other",
	};
	int i;

	if (!kps_enabled)
		return;
	pr_info("kpage_share: %lu mapv calls, %lu pages mapped\n", kps_calls, kps_mapped);
	for (i = 1; i < KPS_R_MAX; i++)
		if (kps_short[i])
			pr_info("kpage_share: %lu calls stopped short: %s\n", kps_short[i], names[i]);
}
