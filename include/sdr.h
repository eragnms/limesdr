/**
 * \file sdr.h
 *
 * \brief SDR class
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */
#pragma once

#include <iostream>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Errors.hpp>
#include <SoapySDR/Time.hpp>
#include <unistd.h>
#include <armadillo>

#include "macros.h"
#include "sdr_config.h"

class SDR
{
public:
        SDR();
        void connect();
        void configure(SDR_Device_Config dev_cfg);
        void start();
        void write(std::vector<std::complex<float>> data);
        std::vector<std::vector<std::complex<int16_t>>> read();
private:
        SDR_Device_Config m_dev_cfg;
};
