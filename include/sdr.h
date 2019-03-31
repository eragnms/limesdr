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

/**
 * \class SDR
 *
 * \brief A class for SDRs
 *
 * Connecting, configuring and running SDRs. The class uses
 * SoapySDR, so it should work for a number of different SDRs.
 *
 */
class SDR
{
public:
        SDR();
        /**
         * \brief Connect to the SDR
         *
         * Will connect to an attached SDR.
         */
        void connect();
        /**
         * \brief Configure the SDR
         *
         * \param[in] dev_cfg A struct carrying the configuration
         */
        void configure(SDR_Device_Config dev_cfg);
        /**
         * \brief Start the SDR
         *
         * setup and activate the streams.
         */
        void start();
        /**
         * \brief Transmit data in the air
         *
         * param[in] data the data to be transmitted
         */
        void write(std::vector<std::complex<float>> data);
        /**
         * \brief Read data from the air
         *
         * \return a vector of data read
         */
        std::vector<std::vector<std::complex<int16_t>>> read();
        bool is_limesdr();
private:
        SDR_Device_Config m_dev_cfg;
        SoapySDR::Device *m_device;

};
