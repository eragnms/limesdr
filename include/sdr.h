/**
 * \file sdr.h
 *
 * \brief SDR class
 *
 * Class for the SDR
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
#include <SoapySDR/Modules.hpp>
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
        /**
         * \brief SDR constructor
         */
        SDR();
        /**
         * \brief Connect to an SDR
         *
         * Will connect to first attached SDR.
         */
        void connect();
        /**
         * \brief Connect to an SDR
         *
         * Will connect to an attached SDR with a specific serial
         * number.
         *
         * \param[in] device_serial serial number of the device to use
         */
        void connect(std::string device_serial);
        /**
         * \brief Configure the SDR
         *
         * \param[in] dev_cfg A struct carrying the configuration
         */
        void configure(SDR_Device_Config dev_cfg);
        /**
         * \brief Returns the device handle
         *
         * \return the device handle
         */
        SoapySDR::Device *get_device();
        /**
         * \brief Start the SDR
         *
         * setup and activate the streams.
         *
         * \return the timestamp at start
         */
        int64_t start();
        /**
         * \brief Returns the RX stream handle
         *
         * \return the RX stream handle
         */
        SoapySDR::Stream *get_rx_stream();

        /**
         * \brief Transmit data in the air
         *
         * \param[in] data the data to be transmitted
         * \param[in] no_of_samples the number of tx samples
         * \param[in] burst_time the timestamp of transmission [ns]
         * \return the number of transmitted samples
         */
        size_t write(std::vector<void *> data, size_t no_of_samples,
                            long long int burst_time);
        /**
         * \brief Read data from the air
         *
         * \param[in] data vector of pointers to data vectors, will
         * return the sampled data
         * \param[in] no_of_samples number of samples to read
         * \return number of read samples
         */
        int32_t read(size_t no_of_samples,
                     std::vector<std::complex<int16_t>> &buff_data);
        /**
         * \brief Close streams and disconnect device
         *
         */
        void close();
        /**
         * \brief Check if a limesdr is conected
         *
         * \return true if the first attached device is a LimeSDR
         */
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
         * \brief Modify the burst_time if burst_time is strange
         *
         * The burst_time will be considered strange if the current
         * harware has passed it in time. A number of burst periods
         * will then be added to the burst time and a new burst time
         * will be returned
         *
         * \param[in] burst_time the time to check
         * \return burst time to use
         */
        void check_burst_time(long long int burst_hw_ns);
        int64_t ix_to_hw_ns(int64_t ix);
        int64_t expected_ping_pos_ix(int64_t hw_time_of_sync);
        int64_t expected_pong_pos_ix(int64_t hw_time_of_sync);

private:
        std::string get_device_driver();
        void configure_tx();
        void configure_rx();
        void start_tx();
        int64_t start_rx();
        int32_t look_up_device_serial(SoapySDR::KwargsList result,
                                      std::string device_id);
        void connect_to_device(SoapySDR::KwargsList results,
                               int32_t device_num);
        void check_lib_bladerf_support();
        std::string get_modules_version(std::string libname);


        SDR_Device_Config m_dev_cfg;
        SoapySDR::Device *m_device;
        SoapySDR::Stream *m_tx_stream;
        SoapySDR::Stream *m_rx_stream;
        int64_t m_rx_start_hw_ticks;
        int64_t m_last_rx_timestamp;
        int64_t m_time_of_next_burst;
};
