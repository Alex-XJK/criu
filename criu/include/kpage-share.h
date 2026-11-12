#ifndef __CR_KPAGE_SHARE_H__
#define __CR_KPAGE_SHARE_H__

#include <linux/types.h>
#include <linux/ioctl.h>

/*
 * User API of the kpage_share kernel module (copy of kpage_share.h from the
 * module tree). KPS_MAPV maps the pages holding file bytes
 * [off, off + sum(iov_len)) read-only at the iov addresses: a preadv() that
 * shares pages instead of copying them. done says how many bytes were
 * mapped from the start of iov; the caller copies the rest.
 */
#define KPS_DEVICE	"/dev/kpage_share"
#define KPS_MAX_IOV	1024

#define KPS_NOCREATE	(1u << 0)
#define KPS_KEEP_CACHE	(1u << 1)
#define KPS_RELEASE_ALL	(1u << 0)

enum {
	KPS_R_NONE = 0,
	KPS_R_NOSPC,
	KPS_R_NOENT,
	KPS_R_STALE,
	KPS_R_VMA,
	KPS_R_BUSY,
	KPS_R_NOMEM,
	KPS_R_IO,
	KPS_R_FAULT,
	KPS_R_OTHER,
	KPS_R_MAX
};

struct kps_mapv {
	__s32 fd;
	__u32 flags;
	__u64 off;
	__u64 iov;
	__u32 nr_iov;
	__u32 reason;
	__u64 done;
};

struct kps_pool_open {
	__s32 fd;
	__u32 flags;
	__u64 nr_pages;
	__u32 created;
	__u32 pad;
};

struct kps_pool_release {
	__s32 fd;
	__u32 flags;
	__u32 pad;
	__u64 pools;
	__u64 pages;
	__u64 pages_mapped;
};

struct kps_stats {
	__u64 nr_pools;
	__u64 pool_pages;
	__u64 max_pages;
	__u64 builds;
	__u64 build_failures;
	__u64 evictions;
	__u64 releases;
	__u64 mapv_calls;
	__u64 pages_mapped;
	__u64 fallback[KPS_R_MAX];
	__u64 pad[8];
};

#define KPS_IOC_MAGIC		'k'
#define KPS_MAPV		_IOWR(KPS_IOC_MAGIC, 1, struct kps_mapv)
#define KPS_POOL_OPEN		_IOWR(KPS_IOC_MAGIC, 2, struct kps_pool_open)
#define KPS_POOL_RELEASE	_IOWR(KPS_IOC_MAGIC, 3, struct kps_pool_release)
#define KPS_STATS		_IOR(KPS_IOC_MAGIC, 4, struct kps_stats)

#ifndef CR_NOGLIBC
#include <sys/types.h>

/* true once --kpage-share opened the device (service fd KPS_FD_OFF) */
extern bool kps_enabled;

extern int kpage_share_init(void);
/* Map file bytes [off, off + len) of fd at addr; returns the bytes mapped. */
extern long kpage_share_map(int fd, off_t off, void *addr, size_t len);
extern void kpage_share_log_stats(void);
#endif

#endif /* __CR_KPAGE_SHARE_H__ */
