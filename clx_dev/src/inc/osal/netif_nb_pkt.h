/*
 * Copyright 2022 Clounix
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License, version 2, as
 * published by the Free Software Foundation (the "GPL").
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License version 2 (GPLv2) for more details.
 *
 * You should have received a copy of the GNU General Public License
 * version 2 (GPLv2) along with this source code.
 */

/* FILE NAME:  hal_nb_pkt_knl.h
 * PURPOSE:
 *      To provide Linux kernel for PDMA TX/RX control.
 *
 * NOTES:
 */

#ifndef HAL_NB_PKT_KNL_H
#define HAL_NB_PKT_KNL_H

#include <clx_error.h>
#include <netif_knl.h>

/* PKT definitions */
#define HAL_NB_PKT_CX_HDR_SZ               (20)
#define HAL_NB_PKT_PPH_HDR_SZ              (40)
#define HAL_NB_PKT_TX_MAX_LEN              (9216)



#endif /* end of HAL_NB_PKT_KNL_H */
