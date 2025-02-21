/* SPDX-License-Identifier: Marvell-Proprietary
 * Copyright (c) 2024 Marvell.
 */
#include <rte_ethdev.h>
#include <rte_malloc.h>

#include "dao_virtio_netdev.h"
#include "virtio_dev_priv.h"
#include "virtio_net_priv.h"

/** Virtio net devices */
struct dao_virtio_netdev dao_virtio_netdevs[DAO_VIRTIO_DEV_MAX + 1];

dao_net_desc_manage_fn_t dao_net_desc_manage_fns[VIRTIO_NET_DESC_MANAGE_LAST << 1] = {
#define M(name, flags)[flags] = virtio_net_desc_manage_##name,
	VIRTIO_NET_DESC_MANAGE_MODES
#undef M
};

struct dao_virtio_netdev_cbs user_cbs;
int virtio_netdev_clear_queue_info(struct virtio_netdev *netdev);

static int
virtio_netdev_feature_validate(struct virtio_dev *dev, uint64_t feature_bits)
{
	uint16_t dev_id = dev->dev_id;
	int i;

	if ((feature_bits | dev->dev_feature_bits) != dev->dev_feature_bits) {
		dao_err("[dev %u] Invalid feature bits negotiated 0x%" PRIx64 "(dev %" PRIx64 ")",
			dev_id, feature_bits, dev->dev_feature_bits);
		return -EINVAL;
	}

	/* Dump features enabled for debug purpose */
	dao_dbg("[dev %u] Features enabled:", dev_id);
	for (i = 0; i < 64; i++) {
		if (!(feature_bits & DAO_BIT(i)))
			continue;
		switch (i) {
		case VIRTIO_NET_F_CSUM:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_CSUM");
			break;
		case VIRTIO_NET_F_GUEST_CSUM:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_GUEST_CSUM");
			break;
		case VIRTIO_NET_F_MTU:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_MTU");
			break;
		case VIRTIO_NET_F_MAC:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_MAC");
			break;
		case VIRTIO_NET_F_GUEST_TSO4:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_GUEST_TSO4");
			break;
		case VIRTIO_NET_F_GUEST_TSO6:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_GUEST_TSO6");
			break;
		case VIRTIO_NET_F_GUEST_ECN:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_GUEST_ECN");
			break;
		case VIRTIO_NET_F_GUEST_UFO:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_GUEST_UFO");
			break;
		case VIRTIO_NET_F_HOST_TSO4:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_HOST_TSO4");
			break;
		case VIRTIO_NET_F_HOST_TSO6:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_HOST_TSO6");
			break;
		case VIRTIO_NET_F_HOST_ECN:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_HOST_ECN");
			break;
		case VIRTIO_NET_F_HOST_UFO:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_HOST_UFO");
			break;
		case VIRTIO_NET_F_MRG_RXBUF:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_MRG_RXBUF");
			break;
		case VIRTIO_NET_F_STATUS:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_STATUS");
			break;
		case VIRTIO_NET_F_CTRL_VQ:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_CTRL_VQ");
			break;
		case VIRTIO_NET_F_CTRL_RX:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_CTRL_RX");
			break;
		case VIRTIO_NET_F_CTRL_VLAN:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_CTRL_VLAN");
			break;
		case VIRTIO_NET_F_CTRL_RX_EXTRA:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_CTRL_RX_EXTRA");
			break;
		case VIRTIO_NET_F_GUEST_ANNOUNCE:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_GUEST_ANNOUNCE");
			break;
		case VIRTIO_NET_F_MQ:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_MQ");
			break;
		case VIRTIO_NET_F_CTRL_MAC_ADDR:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_CTRL_MAC_ADDR");
			break;
		case VIRTIO_NET_F_HASH_REPORT:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_HASH_REPORT");
			break;
		case VIRTIO_NET_F_GUEST_HDRLEN:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_GUEST_HDRLEN");
			break;
		case VIRTIO_NET_F_RSS:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_RSS");
			break;
		case VIRTIO_NET_F_SPEED_DUPLEX:
			dao_dbg("[dev %u]    +%s", dev_id, "VIRTIO_NET_F_SPEED_DUPLEX");
			break;
		case VIRTIO_F_VERSION_1:
			dao_dbg("[dev %u]    +%s", dev->dev_id, "VIRTIO_F_VERSION_1");
			break;
		case VIRTIO_F_IOMMU_PLATFORM:
			dao_dbg("[dev %u]    +%s", dev->dev_id, "VIRTIO_F_IOMMU_PLATFORM");
			break;
		case VIRTIO_F_RING_PACKED:
			dao_dbg("[dev %u]    +%s", dev->dev_id, "VIRTIO_F_RING_PACKED");
			break;
		case VIRTIO_F_IN_ORDER:
			dao_dbg("[dev %u]    +%s", dev->dev_id, "VIRTIO_F_IN_ORDER");
			break;
		case VIRTIO_F_ORDER_PLATFORM:
			dao_dbg("[dev %u]    +%s", dev->dev_id, "VIRTIO_F_ORDER_PLATFORM");
			break;
		case VIRTIO_F_NOTIFICATION_DATA:
			dao_dbg("[dev %u]    +%s", dev->dev_id, "VIRTIO_F_NOTIFICATION_DATA");
			break;
		default:
			dao_err("[dev %u] Unknown feature bit %d", dev_id, i);
			break;
		};
	}
	return 0;
}

static void
virtio_hash_types_to_hash_report(struct virtio_netdev *netdev, uint64_t virtio_hash_types)
{
	uint8_t *hash_report = netdev->hash_report;

	memset(hash_report, 0, sizeof(uint8_t) * DAO_HASH_REPORT_INDEX_MAX);
	/* IPv4 hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_IPV4) {
		hash_report[RTE_PTYPE_L3_IPV4 >> 4] = VIRTIO_NET_HASH_REPORT_IPv4;
		hash_report[RTE_PTYPE_L3_IPV4_EXT >> 4] = VIRTIO_NET_HASH_REPORT_IPv4;
		hash_report[RTE_PTYPE_L3_IPV4_EXT_UNKNOWN >> 4] = VIRTIO_NET_HASH_REPORT_IPv4;
		hash_report[(RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv4;
		hash_report[(RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv4;
	}

	/* TCPv4 hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_TCPV4)
		hash_report[(RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_TCPv4;

	/* UDPv4 hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_UDPV4)
		hash_report[(RTE_PTYPE_L3_IPV4 | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_UDPv4;

	/* IPv6 hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_IPV6) {
		hash_report[RTE_PTYPE_L3_IPV6 >> 4] = VIRTIO_NET_HASH_REPORT_IPv6;
		hash_report[RTE_PTYPE_L3_IPV6_EXT >> 4] = VIRTIO_NET_HASH_REPORT_IPv6;
		hash_report[RTE_PTYPE_L3_IPV6_EXT_UNKNOWN >> 4] = VIRTIO_NET_HASH_REPORT_IPv6;
		hash_report[(RTE_PTYPE_L3_IPV6 | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv6;
		hash_report[(RTE_PTYPE_L3_IPV6 | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv6;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT_UNKNOWN | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv6;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv6;
	}

	/* TCPv6 hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_TCPV6) {
		hash_report[(RTE_PTYPE_L3_IPV6 | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_TCPv6;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT_UNKNOWN | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_TCPv6;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_TCPv6;
	}

	/* UDPv6 hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_UDPV6) {
		hash_report[(RTE_PTYPE_L3_IPV6 | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_UDPv6;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT_UNKNOWN | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_UDPv6;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_UDPv6;
	}

	/* IPv6 EXT hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_IP_EX) {
		hash_report[RTE_PTYPE_L3_IPV6_EXT >> 4] = VIRTIO_NET_HASH_REPORT_IPv6_EX;
		hash_report[RTE_PTYPE_L3_IPV6_EXT_UNKNOWN >> 4] = VIRTIO_NET_HASH_REPORT_IPv6_EX;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv6_EX;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT_UNKNOWN | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv6_EX;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv6_EX;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT_UNKNOWN | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_IPv6_EX;
	}

	/* TCPv6 EXT hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_TCP_EX) {
		hash_report[(RTE_PTYPE_L3_IPV6_EXT | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_TCPv6_EX;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT_UNKNOWN | RTE_PTYPE_L4_TCP) >> 4] =
			VIRTIO_NET_HASH_REPORT_TCPv6_EX;
	}

	/* UDPv6 EXT hash report */
	if (virtio_hash_types & VIRTIO_NET_HASH_TYPE_UDP_EX) {
		hash_report[(RTE_PTYPE_L3_IPV6_EXT | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_UDPv6_EX;
		hash_report[(RTE_PTYPE_L3_IPV6_EXT_UNKNOWN | RTE_PTYPE_L4_UDP) >> 4] =
			VIRTIO_NET_HASH_REPORT_UDPv6_EX;
	}
}

static int
net_rss_setup(struct virtio_netdev *netdev, struct virtio_net_ctrl *ctrl_cmd)
{
	struct virtio_net_ctrl_rss *rss = (struct virtio_net_ctrl_rss *)ctrl_cmd->data;
	int ret;

	if (user_cbs.rss_cb == NULL)
		return -ENOTSUP;

	/* Set number of vq pairs to requested number of queues */
	netdev->vq_pairs_set = rss->max_tx_vq;

	/* Clear the core and queue map before updating the core map
	 * to requested number of queues.
	 */
	user_cbs.rss_cb(netdev->dev.dev_id, NULL);

	/* Update the core map to requested number of queues and
	 * configure rss.
	 */
	ret = user_cbs.rss_cb(netdev->dev.dev_id, rss);
	if (!ret)
		/* Set hash report values based on the requested hash types, which will be
		 * used in enqueue data path.
		 */
		virtio_hash_types_to_hash_report(netdev, rss->hash_types);

	return ret;
}

static int
net_promisc_setup(struct virtio_netdev *netdev, uint8_t enable)
{
	if (user_cbs.promisc_cb == NULL)
		return -ENOTSUP;
	return user_cbs.promisc_cb(netdev->dev.dev_id, enable);
}

static int
net_allmulti_setup(struct virtio_netdev *netdev, uint8_t enable)
{
	if (user_cbs.allmulti_cb == NULL)
		return -ENOTSUP;
	return user_cbs.allmulti_cb(netdev->dev.dev_id, enable);
}

static int
net_mac_set(struct virtio_netdev *netdev, uint8_t *mac)
{
	if (user_cbs.mac_set == NULL)
		return -ENOTSUP;
	return user_cbs.mac_set(netdev->dev.dev_id, mac);
}

static int
net_mac_add(struct virtio_netdev *netdev, struct virtio_net_ctrl_mac *mac_tbl, uint8_t type)
{
	if (user_cbs.mac_add == NULL)
		return -ENOTSUP;
	return user_cbs.mac_add(netdev->dev.dev_id, mac_tbl, type);
}

static int
net_vlan_add(struct virtio_netdev *netdev, struct virtio_net_ctrl_vlan *vlan)
{
	if (user_cbs.vlan_add == NULL)
		return -ENOTSUP;

	return user_cbs.vlan_add(netdev->dev.dev_id, vlan->tci);
}

static int
net_vlan_del(struct virtio_netdev *netdev, struct virtio_net_ctrl_vlan *vlan)
{
	if (user_cbs.vlan_del == NULL)
		return -ENOTSUP;

	return user_cbs.vlan_del(netdev->dev.dev_id, vlan->tci);
}

static int
net_mq_configure(struct virtio_netdev *netdev, struct virtio_net_ctrl *ctrl_cmd)
{
	uint16_t nb_qps;

	if (user_cbs.mq_configure == NULL)
		return -ENOTSUP;

	nb_qps = *(uint16_t *)ctrl_cmd->data;
	/* Set number of vq pairs to requested number of queues */
	netdev->vq_pairs_set = nb_qps;

	/* Clear the core and queue map before updating the core map
	 * to requested number of queues.
	 */
	user_cbs.mq_configure(netdev->dev.dev_id, false);

	/* Update the core map to requested number of queues. */
	return user_cbs.mq_configure(netdev->dev.dev_id, true);
}

static int
virtio_queue_driver_event_flag(struct virtio_dev *dev, struct virtio_net_queue *queue)
{
	struct vring_packed_desc_event *sd_driver_area;
	int16_t dev2mem = dao_dma_ctrl_dev2mem();
	bool has_err = 0;
	uint16_t tmo_ms;
	int cnt, rc;

	sd_driver_area = (struct vring_packed_desc_event *)queue->sd_driver_area;
	rc = rte_dma_copy(dev2mem, dev->dma_vchan, (rte_iova_t)queue->driver_area,
			  (rte_iova_t)sd_driver_area, sizeof(*sd_driver_area),
			  RTE_DMA_OP_FLAG_SUBMIT);
	if (rc < 0) {
		dao_err("[dev %u] Couldn't submit dma for virtqueue driver area", dev->dev_id);
		return rc;
	}

	tmo_ms = VIRTIO_DMA_TMO_MS;
	do {
		rte_delay_us_sleep(1000);
		cnt = rte_dma_completed(dev2mem, dev->dma_vchan, 1, NULL, &has_err);
		tmo_ms--;
		if (unlikely(has_err))
			dao_err("[dev %u] DMA failed for driver event flag", dev->dev_id);
		if (!tmo_ms) {
			dao_err("[dev %u] DMA timeout for driver event flag", dev->dev_id);
			return -EFAULT;
		}
	} while (cnt != 1);

	return sd_driver_area->desc_event_flags;
}

static void
virtio_netdev_cb_interrupt_conf(struct virtio_netdev *netdev)
{
	uint32_t max_vqs = netdev->dev.max_virtio_queues - 1;
	struct virtio_dev *dev = &netdev->dev;
	struct virtio_net_queue *queue;
	uint32_t i, intr_idx;

	intr_idx = 0;
	for (i = 0; i < max_vqs; i++) {
		queue = netdev->qs[i];
		if (!queue || i % 2)
			continue;

		queue->cb_intr_addr = dev->cb_intr_addr[intr_idx];
		queue->cb_notify_addr = queue->notify_addr + 1;
		__atomic_store_n(queue->cb_notify_addr, 0, __ATOMIC_RELAXED);
		intr_idx = (intr_idx + 1) % dev->nb_cb_intrs;
	}

	dao_dbg("[dev %u] Enabled driver events for %u queues", dev->dev_id, max_vqs);
}

static uint8_t
virtio_netdev_hdr_size(struct virtio_netdev *netdev)
{
	struct virtio_dev *dev = &netdev->dev;
	struct virtio_net_hdr *vnet_hdr;
	uint8_t virtio_hdr_sz;

	if (!(dev->features_ok))
		return 0;

	if (dev->feature_bits & RTE_BIT64(VIRTIO_NET_F_HASH_REPORT))
		virtio_hdr_sz = offsetof(struct virtio_net_hdr, padding_reserved) +
				sizeof(vnet_hdr->padding_reserved);
	else
		virtio_hdr_sz = offsetof(struct virtio_net_hdr, num_buffers) +
				sizeof(vnet_hdr->num_buffers);
	return virtio_hdr_sz;
}

static int
virtio_netdev_populate_queue_info(struct virtio_netdev *netdev, uint16_t queue_id)
{
	struct dao_virtio_netdev *dao_netdev = virtio_netdev_to_dao(netdev);
	uint32_t max_vqs = netdev->dev.max_virtio_queues - 1;
	struct virtio_dev *dev = &netdev->dev;
	struct virtio_queue_conf *q_conf;
	struct virtio_net_queue *queue;
	bool cb_enabled = false;
	uint32_t shadow_area;
	uint32_t mbuf_area;
	uint16_t buf_len;
	int event_flag;

	if (queue_id >= max_vqs)
		return -EINVAL;

	if (!(netdev->flags & DAO_VIRTIO_NETDEV_EXTBUF)) {
		/* Calculate first segment pkt data space */
		buf_len = netdev->pool->elt_size;
		buf_len -= sizeof(struct rte_mbuf);
		buf_len -= RTE_PKTMBUF_HEADROOM;
		buf_len -= rte_pktmbuf_priv_size(netdev->pool);
	} else {
		buf_len = netdev->dataroom_size;
	}

	q_conf = &dev->queue_conf[queue_id];
	if (!q_conf->queue_enable || netdev->qs[queue_id] != NULL)
		return 0;

	/* Setup only enabled queues assuming packed virt queue */
	shadow_area = RTE_ALIGN(q_conf->queue_size * 16 + 8, RTE_CACHE_LINE_SIZE);
	mbuf_area = RTE_ALIGN(q_conf->queue_size * 8, RTE_CACHE_LINE_SIZE);
	queue = rte_zmalloc("virtio_net_queue", sizeof(*queue) + shadow_area + mbuf_area,
			    RTE_CACHE_LINE_SIZE);
	if (!queue) {
		dao_err("[dev %u] Failed to allocate memory for virtio queue", dev->dev_id);
		return -ENOMEM;
	}

	queue->desc_base = (((uint64_t)q_conf->queue_desc_hi << 32) | (q_conf->queue_desc_lo));
	queue->q_sz = q_conf->queue_size;
	if (!(netdev->flags & DAO_VIRTIO_NETDEV_EXTBUF)) {
		queue->mp = netdev->pool;
		/* Populate data offset along with queue for fast path purpose */
		queue->data_off = (sizeof(struct rte_mbuf));
		queue->data_off += RTE_PKTMBUF_HEADROOM;
		queue->data_off += rte_pktmbuf_priv_size(netdev->pool);
	}

	queue->buf_len = buf_len;
	queue->notify_addr = (uint32_t *)(dev->notify_base + (queue_id * dev->notify_off_mltpr));
	queue->mbuf_arr = (struct rte_mbuf **)((uintptr_t)(queue + 1) + shadow_area);
	/* Initial queue wrap counter is 1 as per spec? */
	queue->sd_desc_off = RTE_BIT64(15);
	queue->sd_mbuf_off = RTE_BIT64(15);
	queue->last_off = RTE_BIT64(15);
	queue->compl_off = RTE_BIT64(15); /* Valid only for Rx queue */
	queue->auto_free = netdev->auto_free_en;
	queue->qid = queue_id;
	queue->dma_vchan = dev->dma_vchan;
	netdev->qs[queue_id] = queue;
	dao_netdev->qs[queue_id] = queue;
	queue->dao_netdev = dao_netdev;
	queue->netdev_id = netdev->dev.dev_id;
	queue->hash_report = netdev->hash_report;
	queue->virtio_hdr_sz = virtio_netdev_hdr_size(netdev);

	queue->driver_area = (((uint64_t)q_conf->queue_avail_hi << 32) | (q_conf->queue_avail_lo));
	queue->sd_driver_area = (uintptr_t)queue->sd_desc_base + queue->q_sz * 16;
	/* Fetch data only after driver ok */
	if (dev->driver_ok) {
		event_flag = virtio_queue_driver_event_flag(dev, queue);
		if (event_flag < 0)
			return -1;

		/* Disable call interrupts only if events are disabled for all queues */
		cb_enabled = (event_flag != RING_EVENT_FLAGS_DISABLE);
		if (cb_enabled)
			virtio_netdev_cb_interrupt_conf(netdev);
	}

	dao_dbg("[dev %u] Adding queue%d: desc_base %p q_sz %u", dev->dev_id, queue_id,
		(void *)queue->desc_base, queue->q_sz);
	dao_dbg("[dev %u] Adding queue[%d]: notify_addr %p val %08x", dev->dev_id, queue_id,
		queue->notify_addr, *queue->notify_addr);

	return 0;
}

static int
virtio_netdev_queue_enable(struct virtio_dev *dev, uint16_t queue_id)
{
	struct virtio_netdev *netdev = virtio_dev_to_netdev(dev);

	return virtio_netdev_populate_queue_info(netdev, queue_id);
}

static __rte_always_inline uint16_t
virtio_netdev_flush_enq_queue(struct virtio_netdev *netdev, uint16_t qid)
{
	struct dao_virtio_netdev *dao_netdev = virtio_netdev_to_dao(netdev);
	void *q = netdev->qs[qid];

	if (unlikely(!q))
		return 0;

	if (!(dao_netdev->mgmt_fn_id & VIRTIO_NET_DESC_MANAGE_EXTBUF))
		virtio_net_flush_enq(q);
	else
		virtio_net_flush_enq_ext(q);
	return 0;
}

static __rte_always_inline uint16_t
virtio_netdev_flush_deq_queue(struct virtio_netdev *netdev, uint16_t qid)
{
	struct dao_virtio_netdev *dao_netdev = virtio_netdev_to_dao(netdev);
	void *q = netdev->qs[qid];

	if (unlikely(!q))
		return 0;

	if (!(dao_netdev->mgmt_fn_id & VIRTIO_NET_DESC_MANAGE_EXTBUF))
		virtio_net_flush_deq(q);
	else
		virtio_net_flush_deq_ext(q);
	return 0;
}

int
virtio_netdev_clear_queue_info(struct virtio_netdev *netdev)
{
	struct dao_virtio_netdev *dao_netdev = virtio_netdev_to_dao(netdev);
	uint32_t max_vqs = netdev->dev.max_virtio_queues - 1;
	uint32_t i;

	for (i = 0; i < max_vqs; i++) {
		if (i % 2)
			virtio_netdev_flush_deq_queue(netdev, i);
		else
			virtio_netdev_flush_enq_queue(netdev, i);
		if (netdev->qs[i])
			rte_free(netdev->qs[i]);
		netdev->qs[i] = NULL;
		dao_netdev->qs[i] = NULL;
	}

	/* TODO: Free Rx mbufs ?? */
	return 0;
}

static void
virtio_netdev_cq_cmd_process(struct virtio_dev *dev, struct rte_dma_sge *src,
			     struct rte_dma_sge *dst, uint16_t nb_desc)
{
	struct virtio_net_ctrl *ctrl_cmd = (struct virtio_net_ctrl *)dst[0].addr;
	struct virtio_netdev *netdev = virtio_dev_to_netdev(dev);
	int16_t mem2dev = dao_dma_ctrl_mem2dev();
	uint8_t mac_addr[RTE_ETHER_ADDR_LEN];
	struct virtio_net_ctrl_mac *uc, *mc;
	struct virtio_net_ctrl_vlan *vlan;
	int status = VIRTIO_NET_ERR;
	uint8_t promisc, allmulti;
	bool has_err = 0;
	uint16_t tmo_ms;
	uint16_t cnt;
	int rc;

	dao_dbg("[dev %u] cq class: %u command: %u nb_desc %d", dev->dev_id, ctrl_cmd->class,
		ctrl_cmd->command, nb_desc);
	if (ctrl_cmd->class == VIRTIO_NET_CTRL_MQ) {
		switch (ctrl_cmd->command) {
		case VIRTIO_NET_CTRL_MQ_RSS_CONFIG:
			status = net_rss_setup(netdev, ctrl_cmd);
			break;
		case VIRTIO_NET_CTRL_MQ_VQ_PAIRS_SET:
			status = net_mq_configure(netdev, ctrl_cmd);
			break;
		default:
			dao_warn("[dev %u] class:command=%u:%u  is not supported", dev->dev_id,
				 ctrl_cmd->class, ctrl_cmd->command);
			break;
		}
	} else if (ctrl_cmd->class == VIRTIO_NET_CTRL_RX) {
		switch (ctrl_cmd->command) {
		case VIRTIO_NET_CTRL_RX_PROMISC:
			promisc = *(uint8_t *)ctrl_cmd->data;
			status = net_promisc_setup(netdev, promisc);
			break;
		case VIRTIO_NET_CTRL_RX_ALLMULTI:
			allmulti = *(uint8_t *)ctrl_cmd->data;
			status = net_allmulti_setup(netdev, allmulti);
			break;
		default:
			dao_warn("[dev %u] class:command=%u:%u  is not supported", dev->dev_id,
				 ctrl_cmd->class, ctrl_cmd->command);
			break;
		}
	} else if (ctrl_cmd->class == VIRTIO_NET_CTRL_MAC) {
		switch (ctrl_cmd->command) {
		case VIRTIO_NET_CTRL_MAC_ADDR_SET:
			memcpy(mac_addr, ctrl_cmd->data, RTE_ETHER_ADDR_LEN);
			status = net_mac_set(netdev, mac_addr);
			break;
		case VIRTIO_NET_CTRL_MAC_TABLE_SET:
			uc = (struct virtio_net_ctrl_mac *)ctrl_cmd->data;
			status = net_mac_add(netdev, uc, 0);
			if (status)
				dao_dbg("[dev %u] UC MAC add failed ret %d", dev->dev_id, status);
			if (status || nb_desc < 4)
				break;
			mc = (struct virtio_net_ctrl_mac *)(ctrl_cmd->data + sizeof(uc->entries) +
							    (uc->entries * RTE_ETHER_ADDR_LEN));
			status = net_mac_add(netdev, mc, 1);
			if (status)
				dao_dbg("[dev %u] MC MAC add failed ret %d", dev->dev_id, status);
			break;
		default:
			dao_warn("[dev %u] class:command=%u:%u  is not supported", dev->dev_id,
				 ctrl_cmd->class, ctrl_cmd->command);
			break;
		}
	} else if (ctrl_cmd->class == VIRTIO_NET_CTRL_VLAN) {
		vlan = (struct virtio_net_ctrl_vlan *)ctrl_cmd->data;
		switch (ctrl_cmd->command) {
		case VIRTIO_NET_CTRL_VLAN_ADD:
			status = net_vlan_add(netdev, vlan);
			if (status)
				dao_dbg("[dev %u] VLAN add failed ret %d", dev->dev_id, status);
			break;
		case VIRTIO_NET_CTRL_VLAN_DEL:
			status = net_vlan_del(netdev, vlan);
			if (status)
				dao_dbg("[dev %u] VLAN del failed ret %d", dev->dev_id, status);
			break;
		default:
			dao_warn("[dev %u] class:command=%u:%u  is not supported", dev->dev_id,
				 ctrl_cmd->class, ctrl_cmd->command);
			break;
		}
	}

	*((uint8_t *)dst[0].addr) = status;
	/* DMA ack status to the host */
	rc = rte_dma_copy(mem2dev, dev->dma_vchan, dst[0].addr, src[nb_desc - 1].addr,
			  sizeof(virtio_net_ctrl_ack), RTE_DMA_OP_FLAG_SUBMIT);
	if (rc < 0) {
		dao_err("[dev %u] Couldn't submit dma for cq ack status", dev->dev_id);
		return;
	}
	tmo_ms = VIRTIO_DMA_TMO_MS;
	do {
		rte_delay_us_sleep(1000);
		cnt = rte_dma_completed(mem2dev, dev->dma_vchan, 1, NULL, &has_err);
		tmo_ms--;
		if (unlikely(has_err))
			dao_err("[dev %u] DMA failed for driver event flag", dev->dev_id);
		if (!tmo_ms) {
			dao_err("[dev %u] DMA timeout for driver event flag", dev->dev_id);
			return;
		}
	} while (cnt != 1);
}

static int
virtio_netdev_status_cb(struct virtio_dev *dev, uint8_t status)
{
	struct virtio_netdev *netdev = virtio_dev_to_netdev(dev);
	struct dao_virtio_netdev *dao_netdev;
	struct virtio_net_queue *queue;
	bool cb_enabled = false;
	uint16_t gso_offload;
	uint8_t csum_offload;
	int event_flag, i;
	int rc;

	if (user_cbs.status_cb == NULL)
		return -ENOTSUP;

	/* Populate queue info for fast path */
	if (status & VIRTIO_DEV_DRIVER_OK) {
		dao_netdev = virtio_netdev_to_dao(netdev);

		/* Fetch event suppression data if queues is enabled */
		for (i = 0; i < netdev->dev.max_virtio_queues - 1; i++) {
			queue = dao_netdev->qs[i];
			if (!queue)
				continue;
			event_flag = virtio_queue_driver_event_flag(dev, queue);
			if (event_flag < 0)
				continue;

			cb_enabled = (event_flag != RING_EVENT_FLAGS_DISABLE);
			if (cb_enabled)
				break;
		}

		/* Enable interrupts event if one queue wants events delivered */
		if (cb_enabled)
			virtio_netdev_cb_interrupt_conf(netdev);

		/* Update checksum offload config */
		csum_offload = dev->drv_feature_bits_lo & 0xFF;
		dao_netdev->deq_fn_id &= ~VIRTIO_NET_DEQ_OFFLOAD_CHECKSUM;
		dao_netdev->enq_fn_id &= ~VIRTIO_NET_ENQ_OFFLOAD_CHECKSUM;
		dao_netdev->deq_fn_id &= ~VIRTIO_NET_DEQ_OFFLOAD_GSO;
		if (csum_offload & RTE_BIT64(VIRTIO_NET_F_CSUM)) {
			dao_netdev->deq_fn_id |= VIRTIO_NET_DEQ_OFFLOAD_CHECKSUM;

			gso_offload = dev->drv_feature_bits_lo & 0XFFFF;
			if (gso_offload & RTE_BIT64(VIRTIO_NET_F_HOST_TSO4) ||
			    gso_offload & RTE_BIT64(VIRTIO_NET_F_HOST_TSO6))
				dao_netdev->deq_fn_id |= VIRTIO_NET_DEQ_OFFLOAD_GSO;
		}
		if (csum_offload & RTE_BIT64(VIRTIO_NET_F_GUEST_CSUM))
			dao_netdev->enq_fn_id |= VIRTIO_NET_ENQ_OFFLOAD_CHECKSUM;

		dao_netdev->deq_fn_id &= ~VIRTIO_NET_DEQ_OFFLOAD_NOINOR;
		dao_netdev->mgmt_fn_id &= ~(VIRTIO_NET_DESC_MANAGE_NOINORDER |
					    VIRTIO_NET_DESC_MANAGE_MSEG);
		if (!(dev->feature_bits & RTE_BIT64(VIRTIO_F_IN_ORDER))) {
			dao_netdev->deq_fn_id |= VIRTIO_NET_DEQ_OFFLOAD_NOINOR;
			dao_netdev->mgmt_fn_id |= VIRTIO_NET_DESC_MANAGE_NOINORDER;
		}

		dao_netdev->enq_fn_id &= ~VIRTIO_NET_ENQ_OFFLOAD_MSEG;
		if (dev->drv_feature_bits_lo & RTE_BIT64(VIRTIO_NET_F_MRG_RXBUF)) {
			dao_netdev->enq_fn_id |= VIRTIO_NET_ENQ_OFFLOAD_MSEG;
			dao_netdev->mgmt_fn_id |= VIRTIO_NET_DESC_MANAGE_MSEG;
		}

		dao_netdev->enq_fn_id &= ~VIRTIO_NET_ENQ_OFFLOAD_HASH_REPORT;
		if (dev->feature_bits & RTE_BIT64(VIRTIO_NET_F_HASH_REPORT))
			dao_netdev->enq_fn_id |= VIRTIO_NET_ENQ_OFFLOAD_HASH_REPORT;

		return user_cbs.status_cb(netdev->dev.dev_id, status);
	} else if (status == VIRTIO_DEV_RESET) {
		struct virtio_net_queue *q;
		uint32_t i;

		rc = user_cbs.status_cb(netdev->dev.dev_id, status);
		for (i = 0; i < (DAO_VIRTIO_MAX_QUEUES - 1); i++) {
			if (netdev->qs[i]) {
				q = (struct virtio_net_queue *)netdev->qs[i];
				dao_dma_compl_wait(q->dma_vchan);
				break;
			}
		}
		/* Clear queue info after user callback */
		virtio_netdev_clear_queue_info(netdev);
		netdev->vq_pairs_set = 0;
		return rc;
	}

	return user_cbs.status_cb(netdev->dev.dev_id, status);
}

static uint16_t
virtio_netdev_cq_id_get(struct virtio_dev *dev, uint64_t feature_bits)
{
	RTE_SET_USED(dev);

	if (feature_bits & (1ULL << VIRTIO_NET_F_MQ))
		return dev->max_virtio_queues - 1;
	else
		return 2;
}

void
dao_virtio_netdev_cb_register(struct dao_virtio_netdev_cbs *cbs)
{
	user_cbs = *cbs;
}

void
dao_virtio_netdev_cb_unregister(void)
{
	memset(&user_cbs, 0, sizeof(user_cbs));
}

int
dao_virtio_netdev_init(uint16_t devid, struct dao_virtio_netdev_conf *conf)
{
	struct dao_virtio_netdev *virtio_netdev = &dao_virtio_netdevs[devid];
	struct virtio_netdev *netdev = virtio_netdev_priv(virtio_netdev);
	volatile struct virtio_net_config *dev_cfg;
	struct virtio_dev *dev = &netdev->dev;
	uint64_t feature_bits;
	int rc;

	dev->dev_id = devid;
	dev->dev_type = VIRTIO_DEV_TYPE_NET;
	dev->pem_devid = conf->pem_devid;
	dev->dma_vchan = conf->dma_vchan;
	if (!(conf->flags & DAO_VIRTIO_NETDEV_EXTBUF))
		netdev->pool = conf->pool;
	else
		netdev->dataroom_size = conf->dataroom_size;

	netdev->flags = conf->flags;
	netdev->reta_size = conf->reta_size;
	netdev->hash_key_size = conf->hash_key_size;
	netdev->auto_free_en = conf->auto_free_en;

	if (conf->max_virt_qps_limit)
		dev->max_virtio_queues_limit = (conf->max_virt_qps_limit * 2) + 1;

	/* Initialize base virtio device */
	rc = virtio_dev_init(dev);
	if (rc)
		return rc;

	/* Setup netdev config */
	dev_cfg = (volatile struct virtio_net_config *)dev->dev_cfg;
	feature_bits = RTE_BIT64(VIRTIO_NET_F_CTRL_VQ) | RTE_BIT64(VIRTIO_NET_F_MQ) |
		       RTE_BIT64(VIRTIO_NET_F_CTRL_RX) | RTE_BIT64(VIRTIO_NET_F_STATUS) |
		       RTE_BIT64(VIRTIO_NET_F_MAC) | RTE_BIT64(VIRTIO_NET_F_MRG_RXBUF) |
		       RTE_BIT64(VIRTIO_NET_F_SPEED_DUPLEX);

	if (conf->reta_size)
		feature_bits |= RTE_BIT64(VIRTIO_NET_F_RSS) | RTE_BIT64(VIRTIO_NET_F_HASH_REPORT);

	/* Enable add MAC support */
	feature_bits |= RTE_BIT64(VIRTIO_NET_F_CTRL_MAC_ADDR);
	feature_bits |= RTE_BIT64(VIRTIO_NET_F_CTRL_VLAN);

	if (conf->mtu) {
		feature_bits |= RTE_BIT64(VIRTIO_NET_F_MTU);
		dev_cfg->mtu = conf->mtu;
	}

	if (conf->csum_en) {
		/* Enable Checksum offload capability */
		feature_bits |= RTE_BIT64(VIRTIO_NET_F_CSUM) | RTE_BIT64(VIRTIO_NET_F_GUEST_CSUM);
		feature_bits |= RTE_BIT64(VIRTIO_NET_F_HOST_TSO4);
		feature_bits |= RTE_BIT64(VIRTIO_NET_F_HOST_TSO6);
		feature_bits |= (RTE_BIT64(VIRTIO_NET_F_GUEST_HDRLEN));
	}

	virtio_dev_feature_bits_set(dev, feature_bits);

	/* Copy default netdev config */
	dao_dev_memcpy(dev_cfg->mac, conf->mac, sizeof(dev_cfg->mac));
	dev_cfg->status = conf->link_info.status;
	dev_cfg->duplex = conf->link_info.duplex;
	dev_cfg->speed = conf->link_info.speed;
	dev_cfg->max_virtqueue_pairs = dev->max_virtio_queues / 2;
	dev_cfg->rss_max_key_size = netdev->hash_key_size;
	dev_cfg->rss_max_indirection_table_length = netdev->reta_size;
	dev_cfg->supported_hash_types = VIRTIO_NET_HASH_TYPE_MASK;

	/* Enable SW freeing if auto free is disabled */
	if (!netdev->auto_free_en)
		virtio_netdev->enq_fn_id = VIRTIO_NET_ENQ_OFFLOAD_NOFF;

	virtio_netdev->mgmt_fn_id &= ~VIRTIO_NET_DESC_MANAGE_EXTBUF;
	if (conf->flags & DAO_VIRTIO_NETDEV_EXTBUF)
		virtio_netdev->mgmt_fn_id |= VIRTIO_NET_DESC_MANAGE_EXTBUF;

	netdev->hash_report = rte_zmalloc(NULL, sizeof(uint8_t) * DAO_HASH_REPORT_INDEX_MAX, 0);
	if (!netdev->hash_report) {
		dao_err("[dev %u] Failed to allocate memory for hash report table", dev->dev_id);
		return -ENOMEM;
	}

	/* One time setup */
	dev_cbs[VIRTIO_DEV_TYPE_NET].dev_status = virtio_netdev_status_cb;
	dev_cbs[VIRTIO_DEV_TYPE_NET].cq_cmd_process = virtio_netdev_cq_cmd_process;
	dev_cbs[VIRTIO_DEV_TYPE_NET].cq_id_get = virtio_netdev_cq_id_get;
	dev_cbs[VIRTIO_DEV_TYPE_NET].queue_enable = virtio_netdev_queue_enable;
	dev_cbs[VIRTIO_DEV_TYPE_NET].feature_validate = virtio_netdev_feature_validate;
	return 0;
}

int
dao_virtio_netdev_fini(uint16_t devid)
{
	struct dao_virtio_netdev *virtio_netdev = &dao_virtio_netdevs[devid];
	struct virtio_netdev *netdev = virtio_netdev_priv(virtio_netdev);

	rte_free(netdev->hash_report);
	return virtio_dev_fini(&netdev->dev);
}

int
dao_virtio_netdev_queue_count(uint16_t devid)
{
	struct dao_virtio_netdev *virtio_netdev = &dao_virtio_netdevs[devid];
	struct virtio_netdev *netdev = virtio_netdev_priv(virtio_netdev);
	struct virtio_dev *dev = &netdev->dev;

	if (!(dev->common_cfg->device_status & VIRTIO_DEV_DRIVER_OK))
		return 0;

	/* Return vq pairs set count if set or default to 1 as per spec */
	if (netdev->vq_pairs_set)
		return netdev->vq_pairs_set * 2;
	return 2;
}

uint64_t
dao_virtio_netdev_feature_bits_get(uint16_t devid)
{
	struct dao_virtio_netdev *virtio_netdev = &dao_virtio_netdevs[devid];
	struct virtio_netdev *netdev = virtio_netdev_priv(virtio_netdev);
	struct virtio_dev *dev = &netdev->dev;

	if (!(dev->common_cfg->device_status & VIRTIO_DEV_DRIVER_OK))
		return 0;

	return dev->feature_bits;
}

uint8_t
dao_virtio_netdev_hdrlen_get(uint16_t devid)
{
	struct dao_virtio_netdev *virtio_netdev = &dao_virtio_netdevs[devid];
	struct virtio_netdev *netdev = virtio_netdev_priv(virtio_netdev);

	return virtio_netdev_hdr_size(netdev);
}

int
dao_virtio_netdev_queue_count_max(uint16_t pem_devid, uint16_t devid)
{
	int rc;

	/* Get virtio device max queues */
	rc = virtio_dev_max_virtio_queues(pem_devid, devid);
	if (rc <= 0)
		return rc;
	return rc - 1;
}

void
virtio_net_desc_validate(struct virtio_net_queue *q, uint16_t start, uint16_t count, bool avail,
			 bool used)
{
	struct virtio_netdev *netdev = virtio_netdev_priv(q->dao_netdev);
	uintptr_t sd_desc_base = (uintptr_t)q->sd_desc_base;
	struct virtio_dev *dev = &netdev->dev;
	uint16_t q_sz = q->q_sz, off;
	uint64_t flags;
	int i;

	for (i = 0; i < count; i++) {
		off = desc_off_add(start, i, q_sz);

		/* Check if we need to clear the flags with 0x0 as debug */
		if (!avail) {
			*DESC_PTR_OFF(sd_desc_base, off, 8) = 0;
			continue;
		}

		flags = *DESC_PTR_OFF(sd_desc_base, off, 8);
		if ((!!(flags & VIRT_PACKED_RING_DESC_F_AVAIL) != !!(off & RTE_BIT64(15))) ||
		    (flags == 0)) {
			dao_err("[dev %u] queue[%u]: avail does not match wrap bit,"
				" flags=%016lx addr=%p off=%08x",
				dev->dev_id, q->qid, flags,
				(void *)*DESC_PTR_OFF(sd_desc_base, off, 0), off);
			abort();
		}

		if ((!!(flags & VIRT_PACKED_RING_DESC_F_USED) !=
		     !!(flags & VIRT_PACKED_RING_DESC_F_AVAIL)) &&
		    used) {
			dao_err("[dev %u] queue[%u]: used not set, flags=%016lx addr=%p off=%08x",
				dev->dev_id, q->qid, flags,
				(void *)*DESC_PTR_OFF(sd_desc_base, off, 0), off);
			abort();
		}

		if ((!!(flags & VIRT_PACKED_RING_DESC_F_USED) ==
		     !!(flags & VIRT_PACKED_RING_DESC_F_AVAIL)) &&
		    !used) {
			dao_err("[dev %u] queue[%u]: used not clear, flags=%016lx addr=%p off=%08x",
				dev->dev_id, q->qid, flags,
				(void *)*DESC_PTR_OFF(sd_desc_base, off, 0), off);
			abort();
		}
	}
}

static  __rte_always_inline int
virtio_net_desc_manage(uint16_t devid, uint16_t qp_count, const uint16_t flags)
{
	struct dao_virtio_netdev *virtio_netdev = &dao_virtio_netdevs[devid];
	struct virtio_netdev *netdev = virtio_netdev_priv(virtio_netdev);
	struct dao_dma_vchan_info *vchan_info = RTE_PER_LCORE(dao_dma_vchan_info);
	struct dao_dma_vchan_state *dev2mem, *mem2dev;
	struct rte_dma_sge *src, *dst;
	struct virtio_net_queue *q;
	uint16_t compl_off, q_sz;
	uint16_t off, sg_i = 0;
	uint16_t dma_vchan;
	uint16_t nb_desc;
	int i;

	if (unlikely(!netdev->qs[qp_count * 2 - 1]))
		return 0;

	dma_vchan = netdev->qs[0]->dma_vchan;
	dev2mem = &vchan_info->dev2mem[dma_vchan];
	mem2dev = &vchan_info->mem2dev[dma_vchan];

	/* Fetch all DMA completed status */
	dao_dma_check_meta_compl(dev2mem, 1 /* ATOMIC update */);
	dao_dma_check_meta_compl(mem2dev, 1 /* ATOMIC update */);

	for (i = 0; i < qp_count; i++) {
		if (!dao_dma_flush(dev2mem, DAO_DMA_MAX_POINTER))
			break;

		/* Populate pointers for Host Rx queue */
		q = netdev->qs[(i * 2)];
		src = dao_dma_sge_src(dev2mem);
		dst = dao_dma_sge_dst(dev2mem);
		sg_i = fetch_enq_desc_prep(q, dev2mem, src, dst);
		dev2mem->src_i += sg_i;
		dev2mem->dst_i += sg_i;

		if (!dao_dma_flush(dev2mem, DAO_DMA_MAX_POINTER))
			break;

		/* Populate pointers for Host Tx queue */
		q = netdev->qs[(i * 2) + 1];
		src = dao_dma_sge_src(dev2mem);
		dst = dao_dma_sge_dst(dev2mem);
		sg_i = fetch_deq_desc_prep(q, dev2mem, src, dst, flags);
		dev2mem->src_i += sg_i;
		dev2mem->dst_i += sg_i;
	}

	/* Process Host Tx queue completion marking */
	for (i = 0; i < qp_count; i++) {
		q = netdev->qs[(i * 2) + 1];

		off = __atomic_load_n(&q->last_off, __ATOMIC_ACQUIRE);
		compl_off = q->compl_off;
		q_sz = q->q_sz;
		if (compl_off == off)
			continue;

		/* Need space for at least 1 pointer */
		if (!dao_dma_flush(mem2dev, 1))
			break;

		nb_desc = desc_off_diff(off, compl_off, q_sz);

		/* Enqueue Rx completion DMA */
		mark_deq_compl(q, mem2dev, compl_off, nb_desc, flags);
		q->compl_off = off;
	}

	/* Process Host Rx queue completion marking */
	for (i = 0; i < qp_count; i++) {
		q = netdev->qs[(i * 2)];

		/* Check descriptor DMA completion and trigger host interrupt */
		if (q->cb_intr_addr && q->pend_compl &&
		    dao_dma_op_status(mem2dev, q->pend_compl_idx)) {
			__atomic_store_n(q->cb_notify_addr, 1, __ATOMIC_RELAXED);
			__atomic_store_n(q->cb_intr_addr, (1UL << 59), __ATOMIC_RELAXED);
			q->pend_compl = 0;
		}

		off = __atomic_load_n(&q->sd_mbuf_off, __ATOMIC_ACQUIRE);
		compl_off = q->compl_off;
		if (compl_off == off)
			continue;

		/* Need space for at least 2 pointer */
		if (!dao_dma_flush(mem2dev, 2))
			break;

		/* Enqueue Tx completion DMA */
		mark_enq_compl(q, mem2dev, compl_off, off, flags);
		q->compl_off = off;

		/* Store tail to check descriptor DMA completion */
		q->pend_compl_idx = mem2dev->tail;
		q->pend_compl = 1;
	}

	return 0;
}

int
dao_virtio_netdev_link_sts_update(uint16_t devid, struct dao_virtio_netdev_link_info *link_info)
{
	struct dao_virtio_netdev *virtio_netdev = &dao_virtio_netdevs[devid];
	struct virtio_netdev *netdev = virtio_netdev_priv(virtio_netdev);
	volatile struct virtio_net_config *dev_cfg;
	struct virtio_dev *dev = &netdev->dev;

	if (!dev->cb_intr_addr[0])
		return -ENOTSUP;

	dev_cfg = (volatile struct virtio_net_config *)dev->dev_cfg;
	dev_cfg->status = link_info->status;
	dev_cfg->duplex = link_info->duplex;
	dev_cfg->speed = link_info->speed;
	/* Notify host with link interrupt */
	*(uint8_t *)dev->isr = 0x2;
	__atomic_store_n(dev->cb_intr_addr[0], (1UL << 59), __ATOMIC_RELAXED);

	return 0;
}

#define M(name, flags)                                                                             \
	int virtio_net_desc_manage_##name(uint16_t devid, uint16_t qp_count)                       \
	{                                                                                          \
		return virtio_net_desc_manage(devid, qp_count, (flags));                           \
	}

VIRTIO_NET_DESC_MANAGE_MODES
#undef M
