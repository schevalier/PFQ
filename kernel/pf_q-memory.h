/***************************************************************
 *                                                
 * (C) 2011-13 Nicola Bonelli <nicola.bonelli@cnit.it>   
 * 	           Loris Gazzarrini <loris.gazzarrini@iet.unipi.it>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * The full GNU General Public License is included in this distribution in
 * the file called "COPYING".
 *
 ****************************************************************/

#ifndef _PFQ_MEMORY_H_
#define _PFQ_MEMORY_H_

#include <linux/skbuff.h>

/* per-cpu data... */

struct local_data 
{
        unsigned long           eligible_mask;
        unsigned long           sock_mask [Q_MAX_ID];
        int                     sock_cnt;
        int 			        flowctrl;
        struct pfq_queue_skb    prefetch_queue;
        struct sk_buff_head     recycle_list;     
};


extern int recycle_len;

extern struct local_data __percpu    * cpu_data;


struct sk_buff * __pfq_alloc_skb(unsigned int size, gfp_t priority, int fclone, int node);
struct sk_buff * pfq_dev_alloc_skb(unsigned int length);
struct sk_buff * __pfq_netdev_alloc_skb(struct net_device *dev, unsigned int length, gfp_t gfp);


static inline void
pfq_kfree_skb_list(struct sk_buff *skb, struct sk_buff_head *list)
{
#ifdef PFQ_USE_SKB_RECYCLE

        if (likely(skb_queue_len(list) <= recycle_len))
        {
                __skb_queue_head(list, skb);
                return;
        }

#endif        
	__kfree_skb(skb);     
}


static inline void
pfq_kfree_skb(struct sk_buff *skb)
{
#ifdef PFQ_USE_SKB_RECYCLE
        struct local_data * local_data = __this_cpu_ptr(cpu_data);
        struct sk_buff_head * list = &local_data->recycle_list;
	    return pfq_kfree_skb_list(skb, list);
#else
        return pfq_kfree_skb_list(skb, NULL);
#endif
}


static inline 
struct sk_buff *
pfq_skb_recycle(struct sk_buff *skb)
{
        skb_recycle(skb);
        return skb;
}


static inline 
struct sk_buff *
pfq_netdev_alloc_skb(struct net_device *dev, unsigned int length)
{
        return __pfq_netdev_alloc_skb(dev, length, GFP_ATOMIC);
}


static inline
struct sk_buff *
__pfq_netdev_alloc_skb_ip_align(struct net_device *dev, unsigned int length, gfp_t gfp)
{
        struct sk_buff *skb = __pfq_netdev_alloc_skb(dev, length + NET_IP_ALIGN, gfp);
        if (NET_IP_ALIGN && likely(skb)) 
                skb_reserve(skb, NET_IP_ALIGN);
        return skb;
}


static inline
struct sk_buff *
pfq_netdev_alloc_skb_ip_align(struct net_device *dev, unsigned int length)
{
        return __pfq_netdev_alloc_skb_ip_align(dev, length, GFP_ATOMIC);
}


static inline
struct sk_buff *
pfq_alloc_skb(unsigned int size, gfp_t priority)
{
#ifdef PFQ_USE_SKB_RECYCLE
        struct local_data * local_data = __this_cpu_ptr(cpu_data);
        struct sk_buff *skb;

        skb = __skb_dequeue(&local_data->recycle_list);
        if (skb) 
                return pfq_skb_recycle(skb);
#endif
        return alloc_skb(size, priority);
}


static inline
struct sk_buff *
pfq_alloc_skb_fclone(unsigned int size, gfp_t priority)
{
        return __pfq_alloc_skb(size, priority, 1, NUMA_NO_NODE);
}


#endif /* _PFQ_MEMORY_H_ */
