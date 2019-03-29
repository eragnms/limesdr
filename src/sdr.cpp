/**
 * \file sdr.cpp
 *
 * \brief SDR class
 *
 * Class for the SDR
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "sdr.h"

SDR::SDR(SDR_Device_Config dev_cfg)
{
        m_dev_cfg = dev_cfg;
}
