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
         *
         * \return the timestamp at start
         */
        int64_t start();
        /**
         * \brief Transmit data in the air
         *
         * \param[in] data the data to be transmitted
         * \param[in] no_of_samples the number of tx samples
         * \param[in] burst_time the timestamp of transmission [ns]
         * \return the number of transmitted samples
         */
        uint32_t write(std::vector<void *> data, size_t no_of_samples,
                            long long int burst_time);
        /**
         * \brief Read data from the air
         *
         * \return a vector of data read
         */
        std::vector<std::vector<std::complex<int16_t>>> read();
        /**
         * \brief Check if a limesdr is conected
         *
         * \return true if the first attached device is a LimeSDR
         */
        /**
         * \brief Close streams and disconnect device
         *
         */
        void close();
        bool is_limesdr();
        /**
         * \brief Check if a bladerf is conected
         *
         * \return true if the first attached device is BladeRF
         */
        bool is_bladerf();
        /**
         * \brief List HW info for attached devices
         *
         * HW info will be printed on the screen.
         */
        void list_hw_info();
        /**
         * \brief Print a warning if burst_time is strange
         *
         */
        void check_burst_time(long long int burst_time);
private:
        std::string get_device_driver();

        SDR_Device_Config m_dev_cfg;
        SoapySDR::Device *m_device;
        SoapySDR::Stream *m_tx_stream;
        SoapySDR::Stream *m_rx_stream;
        int64_t m_tx_start_tick;
        int64_t m_rx_start_tick;
};
