/* SPDX-License-Identifier: Marvell-Proprietary
 * Copyright (c) 2024 Marvell.
 */

#include <rte_ip.h>
#include <rte_vect.h>

#include "dao_virtio_netdev.h"
#include "virtio_dev_priv.h"

#include "spec/virtio_net.h"
#include "virtio_net_priv.h"

dao_virtio_net_deq_fn_t dao_virtio_net_deq_fns[VIRTIO_NET_DEQ_OFFLOAD_LAST << 1] = {
#define R(name, flags)[flags] = virtio_net_deq_##name,
	VIRTIO_NET_DEQ_FASTPATH_MODES
#undef R
};

#define TX_OFFLOAD_SHIFT 52

#define TX_IPV4_UDP_OFFLOAD (RTE_MBUF_F_TX_IP_CKSUM | RTE_MBUF_F_TX_IPV4 | RTE_MBUF_F_TX_UDP_CKSUM)
#define TX_IPV4_TCP_OFFLOAD (RTE_MBUF_F_TX_IP_CKSUM | RTE_MBUF_F_TX_IPV4 | RTE_MBUF_F_TX_TCP_CKSUM)

#define TX_IPV4_TCP_GSO_OFFLOAD                                                                    \
	(RTE_MBUF_F_TX_IP_CKSUM | RTE_MBUF_F_TX_IPV4 | RTE_MBUF_F_TX_TCP_CKSUM |                   \
	 RTE_MBUF_F_TX_TCP_SEG)

#define TX_IPV6_TCP_GSO_OFFLOAD                                                                    \
	(RTE_MBUF_F_TX_IP_CKSUM | RTE_MBUF_F_TX_IPV6 | RTE_MBUF_F_TX_TCP_CKSUM |                   \
	 RTE_MBUF_F_TX_TCP_SEG)

void
virtio_net_flush_deq(struct virtio_net_queue *q)
{
	uint16_t pend_sd_mbuf = q->pend_sd_mbuf, sd_desc_off, sd_mbuf_off = q->sd_mbuf_off;
	uint16_t last_off = q->last_off, q_sz = q->q_sz;
	uint32_t nb_avail;

	sd_desc_off = __atomic_load_n(&q->sd_desc_off, __ATOMIC_ACQUIRE);
	/* Return if no pending mbufs */
	if (unlikely(!pend_sd_mbuf || sd_desc_off == sd_mbuf_off))
		return;

	/* We are sure all DMAs are completed before reaching here */
	if (pend_sd_mbuf) {
		sd_mbuf_off = desc_off_add(q->sd_mbuf_off, pend_sd_mbuf, q_sz);
		pend_sd_mbuf = 0;
		q->sd_mbuf_off = sd_mbuf_off;
		q->pend_sd_mbuf = 0;
	}
	/* Check for available mbufs */
	nb_avail = desc_off_diff_no_wrap(sd_mbuf_off, last_off, q_sz);
	/* Return if no mbuf's available */
	if (unlikely(!nb_avail))
		return;

	rte_pktmbuf_free_bulk(&q->mbuf_arr[DESC_OFF(last_off)], nb_avail);
	last_off = desc_off_add(last_off, nb_avail, q_sz);
	nb_avail = sd_mbuf_off - last_off;
	if (nb_avail)
		rte_pktmbuf_free_bulk(&q->mbuf_arr[DESC_OFF(last_off)], nb_avail);
}

static __rte_always_inline uint16_t
post_process_pkts(struct virtio_net_queue *q, struct rte_mbuf **d_mbufs, uint16_t *nb_mbufs,
		  const uint16_t flags)
{
	const uint64_t rearm_data = 0x100010000ULL | RTE_PKTMBUF_HEADROOM;
	struct rte_mbuf **mbuf_arr, *mbuf0, *mbuf1, *mbuf2, *mbuf3;
	uintptr_t desc_base = (uintptr_t)q->sd_desc_base;
	uint16_t last_off = DESC_OFF(q->last_off), off;
	uint64x2_t desc0, desc1, desc2, desc3, doff;
	const uint16_t vhdr_sz = q->virtio_hdr_sz;
	uint64x2_t mbuf01, mbuf23, hdr01, hdr23;
	uint32x4_t csum_off, tx_offload, olflags;
	uint32x4_t hdr0, hdr1, hdr2, hdr3;
	uint16_t total_mbufs = *nb_mbufs;
	uint16_t data_off = q->data_off;
	uint16_t q_sz = q->q_sz, segs;
	uint64x2_t flags01, flags23;
	uint32x4_t d0, d1, len_mask;
	struct virtio_net_hdr *hdr;
	uint64_t ol_flags, dflags;
	int count, i, num = 0;
	uint16_t l3_len = 0;

	doff = vdupq_n_u64(data_off);
	mbuf_arr = q->mbuf_arr;
	count = total_mbufs & ~(0x3u);
	for (i = 0; i < count; i += 4) {
		const uint8x16_t tbl = {
			0, 0, 0, 0, 0, TX_IPV4_UDP_OFFLOAD >> TX_OFFLOAD_SHIFT, 0, 0, 0, 0,
			0, 0, 0, 0, 0, TX_IPV4_TCP_OFFLOAD >> TX_OFFLOAD_SHIFT,
		};

		if (unlikely(last_off + 3 >= q_sz))
			break;

		desc0 = vld1q_u64(DESC_PTR_OFF(desc_base, last_off, 0));
		desc1 = vld1q_u64(DESC_PTR_OFF(desc_base, last_off + 1, 0));
		desc2 = vld1q_u64(DESC_PTR_OFF(desc_base, last_off + 2, 0));
		desc3 = vld1q_u64(DESC_PTR_OFF(desc_base, last_off + 3, 0));

		flags01 = vzip2q_u64(desc0, desc1);
		flags23 = vzip2q_u64(desc2, desc3);

		flags01 = vtrn2q_u32(flags01, flags23);
		const uint64x2_t xflags = {
			0x0001000000010000,
			0x0001000000010000,
		};
		flags01 = vandq_u64(flags01, xflags);
		flags01 = vceqzq_u64(flags01);
		/* If VRING_DESC_F_NEXT is set in any then process remaining mbufs in scalar way */
		if (unlikely(!vgetq_lane_u64(flags01, 0) || !vgetq_lane_u64(flags01, 1)))
			break;

		/* Prefetch next line */
		rte_prefetch0(DESC_PTR_OFF(desc_base, last_off + 16, 0));
		rte_prefetch0(mbuf_arr + last_off + 12);
		if (last_off + 11 < q_sz) {
			rte_prefetch0((uint8_t *)mbuf_arr[last_off + 8] + data_off);
			rte_prefetch0((uint8_t *)mbuf_arr[last_off + 9] + data_off);
			rte_prefetch0((uint8_t *)mbuf_arr[last_off + 10] + data_off);
			rte_prefetch0((uint8_t *)mbuf_arr[last_off + 11] + data_off);
		}

		if (!(flags & VIRTIO_NET_DEQ_OFFLOAD_CHECKSUM))
			goto skip_csum;

		mbuf01 = vld1q_u64((uint64_t *)&mbuf_arr[last_off]);
		mbuf23 = vld1q_u64((uint64_t *)&mbuf_arr[last_off + 2]);

		/* Move mbuf to data offset */
		hdr01 = vaddq_u64(mbuf01, doff);
		hdr23 = vaddq_u64(mbuf23, doff);

		/* Load virtio Net headers */
		hdr0 = vld1q_u32((void *)vgetq_lane_u64(hdr01, 0));
		hdr1 = vld1q_u32((void *)vgetq_lane_u64(hdr01, 1));
		hdr2 = vld1q_u32((void *)vgetq_lane_u64(hdr23, 0));
		hdr3 = vld1q_u32((void *)vgetq_lane_u64(hdr23, 1));

		/* If at least one buffer has gso type set, go to scalar processing */
		if (flags & VIRTIO_NET_DEQ_OFFLOAD_GSO) {
			if ((hdr0[0] & 0XFF00) || (hdr1[0] & 0XFF00) || (hdr2[0] & 0XFF00) ||
			    (hdr3[0] & 0XFF00))
				break;
		}
		/* Combine 4 packet headers into single 128 bit */
		d0 = vtrn1q_u32(hdr0, hdr1);
		d1 = vtrn1q_u32(hdr2, hdr3);

		/* Retrieve csum_offset values and get ol_flags based on csum offset
		 * For UDP, csum offset will be 6, and for TCP, it will be 0x10.
		 * Subtract csum offset with -1 for table lookup
		 */
		csum_off = vzip2q_u32(d0, d1);
		csum_off = vandq_u32(csum_off, vdupq_n_u32(0x0000FFFF));
		csum_off = vsubq_u32(csum_off, vdupq_n_u32(0x00000001));
		olflags = vqtbl1q_u8(tbl, csum_off);

		/* Extract csum start info from packets */
		d0 = vtrn2q_u32(hdr0, hdr1);
		d1 = vtrn2q_u32(hdr2, hdr3);
		tx_offload = vzip1q_u32(d0, d1);
		tx_offload = vshrq_n_u32(tx_offload, 16);

		/* l2_len = csum_start - 20. Update BIT0_BIT6 */
		tx_offload = vsubq_u32(tx_offload, vdupq_n_u32(20));
		len_mask = vcltq_u32(tx_offload, vdupq_n_u32(0x0000003F));
		tx_offload = vandq_u32(tx_offload, len_mask);

		/* Assuming IPv4 packets with 20 bytes header length.
		 * Update len value from BIT_7 to BIT_15
		 */
		tx_offload = vorrq_u32(tx_offload, vdupq_n_u32(0x00000A00));

		mbuf0 = (struct rte_mbuf *)vgetq_lane_u64(mbuf01, 0);
		mbuf1 = (struct rte_mbuf *)vgetq_lane_u64(mbuf01, 1);
		mbuf2 = (struct rte_mbuf *)vgetq_lane_u64(mbuf23, 0);
		mbuf3 = (struct rte_mbuf *)vgetq_lane_u64(mbuf23, 1);

		mbuf0->ol_flags = ((uint64_t)vgetq_lane_u32(olflags, 0)) << TX_OFFLOAD_SHIFT;
		mbuf1->ol_flags = ((uint64_t)vgetq_lane_u32(olflags, 1)) << TX_OFFLOAD_SHIFT;
		mbuf2->ol_flags = ((uint64_t)vgetq_lane_u32(olflags, 2)) << TX_OFFLOAD_SHIFT;
		mbuf3->ol_flags = ((uint64_t)vgetq_lane_u32(olflags, 3)) << TX_OFFLOAD_SHIFT;

		mbuf0->tx_offload = vgetq_lane_u32(tx_offload, 0);
		mbuf1->tx_offload = vgetq_lane_u32(tx_offload, 1);
		mbuf2->tx_offload = vgetq_lane_u32(tx_offload, 2);
		mbuf3->tx_offload = vgetq_lane_u32(tx_offload, 3);

	skip_csum:
		memcpy(d_mbufs + num, &mbuf_arr[last_off], 32);
		num += 4;
		last_off = (last_off + 4) & (q_sz - 1);
	}

	count = i;
	segs = 0;
	while (i < total_mbufs) {
		rte_prefetch0((uint8_t *)mbuf_arr[last_off + 1] + data_off);
		mbuf0 = mbuf_arr[last_off];

		dflags = (*DESC_PTR_OFF(desc_base, last_off, 8) >> 48) & VRING_DESC_F_NEXT;

		mbuf1 = mbuf0;
		off = last_off;

		/* Calculate additional segments required for mbuf-chain */
		while (unlikely(dflags)) {
			off = (off + 1) & (q_sz - 1);
			dflags = (*DESC_PTR_OFF(desc_base, off, 8) >> 48) & VRING_DESC_F_NEXT;
			segs++;
		}

		if (unlikely((i + segs >= total_mbufs)))
			break;

		/* Create mbuf chain from descriptors */
		while (unlikely(segs)) {
			/* Internal mbufs can also have chain based on descriptor length vs
			 * mbuf length variation.
			 */
			while (mbuf1->next)
				mbuf1 = mbuf1->next;

			last_off = (last_off + 1) & (q_sz - 1);
			mbuf2 = mbuf_arr[last_off];
			mbuf1->next = mbuf2;
			mbuf2->data_len += vhdr_sz;
			mbuf2->pkt_len += vhdr_sz;
			mbuf0->nb_segs += mbuf2->nb_segs;
			mbuf0->pkt_len += mbuf2->pkt_len;
			*((uint64_t *)&mbuf2->rearm_data) = rearm_data;
			mbuf1 = mbuf2;
			i++;
			segs--;
		}

		d_mbufs[num++] = mbuf0;

		if (flags & VIRTIO_NET_DEQ_OFFLOAD_CHECKSUM) {
			hdr = (struct virtio_net_hdr *)((uintptr_t)mbuf0 + data_off);
			ol_flags = 0;
			if (hdr->csum_start && hdr->csum_offset) {
				ol_flags = (hdr->csum_offset == 6) ? TX_IPV4_UDP_OFFLOAD :
								     TX_IPV4_TCP_OFFLOAD;
				l3_len = sizeof(struct rte_ipv4_hdr);
				if (flags & VIRTIO_NET_DEQ_OFFLOAD_GSO) {
					if (hdr->gso_type != VIRTIO_NET_HDR_GSO_NONE) {
						mbuf0->tso_segsz = hdr->gso_size;
						mbuf0->l4_len = hdr->hdr_len - hdr->csum_start;

						if (hdr->gso_type == VIRTIO_NET_HDR_GSO_TCPV4) {
							ol_flags = TX_IPV4_TCP_GSO_OFFLOAD;
						} else if (hdr->gso_type ==
							   VIRTIO_NET_HDR_GSO_TCPV6) {
							l3_len = sizeof(struct rte_ipv6_hdr);
							ol_flags = TX_IPV6_TCP_GSO_OFFLOAD;
						}
					}
				}
				mbuf0->l3_len = l3_len;
				mbuf0->l2_len = hdr->csum_start - l3_len;
				mbuf0->ol_flags |= ol_flags;
			}
		}
		last_off = (last_off + 1) & (q_sz - 1);
		i++;
		count = i;
	}
	/* Return consumed descriptor mbufs to update last_off,
	   And num will hold number of copied mbufs.
	 */
	*nb_mbufs = count;
	return num;
}

static __rte_always_inline uint16_t
fetch_host_data(struct virtio_net_queue *q, struct dao_dma_vchan_state *dev2mem, uint16_t hint,
		const uint16_t flags)
{
	const uint64_t rearm_data = 0x100010000ULL | RTE_PKTMBUF_HEADROOM;
	struct rte_dma_sge *src = NULL, *dst = NULL;
	uintptr_t desc_base = (uintptr_t)q->sd_desc_base;
	struct rte_mbuf *mbuf0, *mbuf1, *mbuf2, *mbuf3;
	const uint16_t vhdr_sz = q->virtio_hdr_sz;
	uint64x2_t len01, len23, buf01, buf23;
	uint64x2_t desc0, desc1, desc2, desc3;
	uint16_t sd_desc_off, sd_mbuf_off;
	uint32_t nb_mbufs, count, nb_enq;
	uint32_t i = 0, slen, dlen, pend = 0;
	uint16_t data_off = q->data_off;
	uint16_t buf_len = q->buf_len;
	uint64x2_t flags01, flags23;
	uint64x2_t vdst[4], vsrc[4];
	struct rte_mbuf **mbuf_arr;
	uint64x2_t mbuf01, mbuf23;
	uint64x2_t xtmp0, xtmp1;
	uint64_t d_flags, avail;
	uint16_t q_sz = q->q_sz;
	struct rte_mbuf *mbuf;
	uint32x4_t xlen, ylen;
	uint16_t used = 0;
	int last_idx = 0;
	uint16_t off, mbuf_off;

	sd_mbuf_off = q->sd_mbuf_off;

	sd_desc_off = __atomic_load_n(&q->sd_desc_off, __ATOMIC_ACQUIRE);
	/* Return if already something is pending DMA or there are no descriptors to process */
	if (unlikely(sd_desc_off == sd_mbuf_off))
		return sd_mbuf_off;

	/* Start DMAs of mbuf's assuming other pending mbuf's are done */
	nb_mbufs = desc_off_diff(sd_desc_off, sd_mbuf_off, q_sz) - q->pend_sd_mbuf;
	if (!nb_mbufs)
		return sd_mbuf_off;

	nb_mbufs = RTE_MIN(nb_mbufs, hint);
	off = desc_off_add(sd_mbuf_off, q->pend_sd_mbuf, q_sz);
	off = DESC_OFF(off);

	rte_prefetch0(DESC_PTR_OFF(desc_base, off, 0));
	mbuf_arr = q->mbuf_arr;

	/* Flush to get minimum space */
	if (!dao_dma_flush(dev2mem, 1))
		return sd_mbuf_off;

	/* Start DMA of mbuf data */
	count = nb_mbufs & ~(0x3u);
	for (i = 0; i < count; ) {
		const uint64x2_t hoff = { 0, vhdr_sz };
		uint64x2_t doff = vdupq_n_u64(data_off);
		uint64x2_t f0, f1, f2, f3;
		const uint64x2_t rearm = {rearm_data + vhdr_sz, 0};
		const uint8x16_t shuf_msk = {
			0xFF, 0xFF,
			0xFF, 0xFF,
			8, 9,
			0xFF, 0xFF,
			8, 9,
			0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF,
		};
		const uint32x4_t vbuf_len = { buf_len, 0, buf_len, 0};
		const uint64x2_t xflags = {
			~(VIRT_PACKED_RING_DESC_F_USED),
			~(VIRT_PACKED_RING_DESC_F_USED),
		};
		const uint64x2_t xflags2 = {
			VIRT_PACKED_RING_DESC_F_AVAIL,
			VIRT_PACKED_RING_DESC_F_AVAIL,
		};

		if (unlikely(off + 3 >= q_sz))
			break;

		desc0 = vld1q_u64(DESC_PTR_OFF(desc_base, off, 0));
		desc1 = vld1q_u64(DESC_PTR_OFF(desc_base, off + 1, 0));
		desc2 = vld1q_u64(DESC_PTR_OFF(desc_base, off + 2, 0));
		desc3 = vld1q_u64(DESC_PTR_OFF(desc_base, off + 3, 0));

		buf01 = vzip1q_u64(desc0, desc1);
		buf23 = vzip1q_u64(desc2, desc3);

		/* Prefetch next line */
		rte_prefetch0(DESC_PTR_OFF(desc_base, off + 16, 0));
		rte_prefetch0(mbuf_arr + off + 24);

		/* Extract lengths */
		len01 = vzip2q_u64(desc0, desc1);
		len23 = vzip2q_u64(desc2, desc3);

		xlen = vcgtq_u32(len01, vbuf_len);
		ylen = vcgtq_u32(len23, vbuf_len);
		xlen = vuzp1q_u32(xlen, ylen);
		nb_enq = vgetq_lane_u32(xlen, 0) ? 2 : 1;
		nb_enq += vgetq_lane_u32(xlen, 1) ? 2 : 1;
		nb_enq += vgetq_lane_u32(xlen, 2) ? 2 : 1;
		nb_enq += vgetq_lane_u32(xlen, 3) ? 2 : 1;
		if (unlikely(nb_enq > 4))
			break;

		mbuf01 = vld1q_u64((uint64_t *)&mbuf_arr[off]);
		mbuf23 = vld1q_u64((uint64_t *)&mbuf_arr[off + 2]);

		if (flags & VIRTIO_NET_DEQ_OFFLOAD_NOINOR) {
			flags01 = vzip2q_u64(desc0, desc1);
			flags23 = vzip2q_u64(desc2, desc3);

			/* Prepare descriptor flags */
			flags01 = flags01 & xflags;
			flags23 = flags23 & xflags;

			/* Extract AVAIL bits and move to USED */
			xtmp0 = flags01 & xflags2;
			xtmp1 = flags23 & xflags2;
			xtmp0 = xtmp0 << 8;
			xtmp1 = xtmp1 << 8;
			/* Set USED field in flags */
			flags01 |= xtmp0;
			flags23 |= xtmp1;

			desc0 = vzip1q_u64(buf01, flags01);
			desc1 = vzip2q_u64(buf01, flags01);
			desc2 = vzip1q_u64(buf23, flags23);
			desc3 = vzip2q_u64(buf23, flags23);

			/* Write back descriptor and its flags */
			vst1q_u64(DESC_PTR_OFF(desc_base, off, 0), desc0);
			vst1q_u64(DESC_PTR_OFF(desc_base, off + 1, 0), desc1);
			vst1q_u64(DESC_PTR_OFF(desc_base, off + 2, 0), desc2);
			vst1q_u64(DESC_PTR_OFF(desc_base, off + 3, 0), desc3);
		}

		/* Take minimum of pktbuf and desc buf len */
		len01 = vminq_u32(len01, vbuf_len);
		len23 = vminq_u32(len23, vbuf_len);

		/* Prepare destination sg list */
		vsrc[0] = vzip1q_u64(buf01, len01);
		vsrc[1] = vzip2q_u64(buf01, len01);
		vsrc[2] = vzip1q_u64(buf23, len23);
		vsrc[3] = vzip2q_u64(buf23, len23);

		mbuf0 = (struct rte_mbuf *)vgetq_lane_u64(mbuf01, 0);
		mbuf1 = (struct rte_mbuf *)vgetq_lane_u64(mbuf01, 1);
		mbuf2 = (struct rte_mbuf *)vgetq_lane_u64(mbuf23, 0);
		mbuf3 = (struct rte_mbuf *)vgetq_lane_u64(mbuf23, 1);

		/* Move mbuf to data offset */
		mbuf01 = vaddq_u64(mbuf01, doff);
		mbuf23 = vaddq_u64(mbuf23, doff);

		/* Prepare destination sg list */
		vdst[0] = vzip1q_u64(mbuf01, len01);
		vdst[1] = vzip2q_u64(mbuf01, len01);
		vdst[2] = vzip1q_u64(mbuf23, len23);
		vdst[3] = vzip2q_u64(mbuf23, len23);

		desc0 = vsubq_u64(desc0, hoff);
		desc1 = vsubq_u64(desc1, hoff);
		desc2 = vsubq_u64(desc2, hoff);
		desc3 = vsubq_u64(desc3, hoff);

		/* Prepare rx_descriptor_fields1 with pkt_len and data_len */
		f0 = vqtbl1q_u8(desc0, shuf_msk);
		f1 = vqtbl1q_u8(desc1, shuf_msk);
		f2 = vqtbl1q_u8(desc2, shuf_msk);
		f3 = vqtbl1q_u8(desc3, shuf_msk);

		vst1q_u64((uint64_t *)&mbuf0->rx_descriptor_fields1, f0);
		vst1q_u64((uint64_t *)&mbuf1->rx_descriptor_fields1, f1);
		vst1q_u64((uint64_t *)&mbuf2->rx_descriptor_fields1, f2);
		vst1q_u64((uint64_t *)&mbuf3->rx_descriptor_fields1, f3);

		/* Store rearm data */
		vst1q_u64((uint64_t *)&mbuf0->rearm_data, rearm);
		vst1q_u64((uint64_t *)&mbuf1->rearm_data, rearm);
		vst1q_u64((uint64_t *)&mbuf2->rearm_data, rearm);
		vst1q_u64((uint64_t *)&mbuf3->rearm_data, rearm);

		/* Update mbuf length */
		mbuf0->next = NULL;
		mbuf1->next = NULL;
		mbuf2->next = NULL;
		mbuf3->next = NULL;

		/* Reset mbuf olflags */
		mbuf0->ol_flags = 0;
		mbuf1->ol_flags = 0;
		mbuf2->ol_flags = 0;
		mbuf3->ol_flags = 0;

		nb_enq = dao_dma_enq_x4(dev2mem, vsrc, vdst);
		i += nb_enq;
		used = i;
		off = (off + nb_enq) & (q_sz - 1);
		last_idx = dev2mem->tail;
		if (nb_enq != 4)
			break;
	}

	/* Flush to get minimum space */
	if (!dao_dma_flush(dev2mem, 1))
		goto exit;

	while (i < nb_mbufs) {
		mbuf = mbuf_arr[off];

		d_flags = *DESC_PTR_OFF(desc_base, off, 8);
		slen = d_flags & (RTE_BIT64(32) - 1);
		dlen = slen;

		if (flags & VIRTIO_NET_DEQ_OFFLOAD_NOINOR) {
			avail = !!(d_flags & VIRT_PACKED_RING_DESC_F_AVAIL);
			d_flags &= ~VIRT_PACKED_RING_DESC_F_AVAIL_USED;

			/* Set both AVAIL and USED bit same */
			*DESC_PTR_OFF(desc_base, off, 8) = avail << 55 | avail << 63 | d_flags;
		}

		/* Limit data to buffer length */
		if (unlikely(slen > buf_len)) {
			pend = slen - buf_len;
			dlen = buf_len;
		}

		src = dao_dma_sge_src(dev2mem);
		dst = dao_dma_sge_dst(dev2mem);
		src[0].addr = *DESC_PTR_OFF(desc_base, off, 0);
		src[0].length = slen;
		dst[0].addr = (((uintptr_t)mbuf) + data_off);
		dst[0].length = dlen;
		dev2mem->src_i++;
		dev2mem->dst_i++;

		/* Update mbuf length */
		*((uint64_t *)&mbuf->rearm_data) = rearm_data + vhdr_sz;
		mbuf->pkt_len = slen - vhdr_sz;
		mbuf->data_len = dlen - vhdr_sz;
		mbuf->next = NULL;
		mbuf->ol_flags = 0;
		mbuf0 = mbuf;
		while (unlikely(pend)) {
			/* allocate new mbuf and attach it */
			rte_mempool_get(q->mp, (void **)&mbuf1);
			*((uint64_t *)&mbuf1->rearm_data) = rearm_data;
			dlen = pend;
			if (unlikely(dlen > buf_len))
				dlen = buf_len;
			mbuf->next = mbuf1;
			mbuf = mbuf1;
			mbuf->data_len = dlen;
			mbuf->next = NULL;
			mbuf->ol_flags = 0;
			mbuf0->nb_segs++;
			/* Enqueue only destination pointers as source length is big */
			dao_dma_enq_dst_x1(dev2mem, (((uintptr_t)mbuf) + data_off), dlen);
			pend -= dlen;
		}

		i++;
		off = (off + 1) & (q_sz - 1);
		used = i;
		last_idx = dev2mem->tail;
		/* Flush on reaching max SG limit */
		if (!dao_dma_flush(dev2mem, mbuf0->nb_segs))
			goto exit;
	}

exit:
	if (likely(used)) {
		mbuf_off = desc_off_add(q->sd_mbuf_off, used + q->pend_sd_mbuf, q->q_sz);
		q->pend_sd_mbuf += used;
		dao_dma_update_cmpl_meta(dev2mem, &q->sd_mbuf_off, mbuf_off, &q->pend_sd_mbuf, used,
					 last_idx);
	}

	return sd_mbuf_off;
}

static __rte_always_inline int
virtio_net_deq(struct virtio_net_queue *q, struct rte_mbuf **mbufs, uint16_t nb_mbufs,
	       const uint16_t flags)
{
	struct dao_dma_vchan_info *vchan_info = RTE_PER_LCORE(dao_dma_vchan_info);
	uint16_t dma_vchan = q->dma_vchan;
	struct dao_dma_vchan_state *dev2mem;
	uint16_t nb_avail, last_off;
	uint16_t sd_mbuf_off;
	uint16_t q_sz;
	int rc = 0;

	dev2mem = &vchan_info->dev2mem[dma_vchan];

	rte_prefetch0(&q->last_off);
	/* Update completed DMA ops */
	dao_dma_check_meta_compl(dev2mem, 0 /* No ATOMIC update */);

	/* Check shadow mbuf status and issue new DMA's for mbuf's */
	sd_mbuf_off = fetch_host_data(q, dev2mem, 128, flags);
	last_off = q->last_off;

	q_sz = q->q_sz;
	/* Check for available mbufs */
	nb_avail = desc_off_diff(sd_mbuf_off, last_off, q_sz);

	/* Return if no mbuf's available */
	if (unlikely(!nb_avail))
		goto exit;

	nb_mbufs = RTE_MIN(nb_mbufs, nb_avail);

	/* Post process packets and fill buffers */
	rc = post_process_pkts(q, mbufs, &nb_mbufs, flags);

	last_off = desc_off_add(last_off, nb_mbufs, q_sz);
	__atomic_store_n(&q->last_off, last_off, __ATOMIC_RELEASE);
exit:
	return rc;
}

#define R(name, flags)                                                                             \
	uint16_t virtio_net_deq_##name(void *q, struct rte_mbuf **mbufs, uint16_t nb_mbufs)        \
	{                                                                                          \
		return virtio_net_deq(q, mbufs, nb_mbufs, (flags));                                \
	}

VIRTIO_NET_DEQ_FASTPATH_MODES
#undef R
