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

SDR::SDR()
{}

void SDR::connect()
{}

void SDR::configure(SDR_Device_Config dev_cfg)
{
        m_dev_cfg = dev_cfg;
}

void SDR::start()
{}

void SDR::write(std::vector<std::complex<float>> data)
{
        size_t a = data.size();
        a++;
}

std::vector<std::vector<std::complex<int16_t>>> SDR::read()
{
        std::vector<std::vector<std::complex<int16_t>>> a;
        return a;
}
