/**
 * \file sdr_config.h
 *
 * \brief SDR config
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */
#pragma once
#include <SoapySDR/Logger.hpp>

/**
 * \struct SDR_Device_Config
 *
 * \brief SDR device configuration parameters
 *
 * Configuration parameters for the SDR. Set the parameters either in this
 * file, or change them in the file configuring the SDR.
 */
struct SDR_Device_Config
{
        //SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_FATAL;
        //SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_CRITICAL;
        //SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_ERROR;
        //SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_WARNING;
        //SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_NOTICE;
        SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_INFO;
        //SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_DEBUG;
        //SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_TRACE;
        //SoapySDRLogLevel log_level = SoapySDR::LogLevel::SOAPY_SDR_SSI;
        std::string args = ""; //!< Not used if ""
        std::string clock_source = ""; //!< Not used if ""
        std::string time_source = ""; //!< Not used if ""

        std::string serial_bladerf_x40 = "a662f87f08f131e8dc3f4700c5d555e7"; //!< Old BladeRF
        std::string serial_bladerf_xA4 = "9fe83c7bbc004b49b71a0adfd941ae6d"; //!< New BladeRF
        std::string serial_lime_1 = "0009072C02881717";
        std::string serial_lime_2 = "00090726074D2435";
        std::string serial_lime_3 = "0009072C02870A19";

        double frequency = 800e6; //!< Center frequency [Hz]
        double tx_gain = 50; //!< 60 dB is about 0 dBm
        double rx_gain = 20;
        double tx_bw = -1; //!< Not used if -1
        double rx_bw = -1; //!< Not used if -1

        uint16_t Novs_tx = 2; //!< No of oversampling [2,4,8]
        uint16_t Novs_rx = 2; //!< No of oversampling [2,4,8]
        double time_in_future = 1;
        double burst_period = 10e-3; //!< Time between PINGs [s]
        size_t tx_burst_length_chip = 512; //!< PING length
        double tx_burst_length = tx_burst_length_chip * Novs_tx;
        double extra_samples_filter = 1/8;

        double f_clk = 122.88e6; //!< SDR system clock
        short channel_tx = 0;
        short channel_rx = 0;
        uint16_t D_tx = 32 / Novs_tx; //!< Should be 8
        uint16_t D_rx = 32 / Novs_rx; //!< Should be 8
        double sampling_rate_tx = f_clk / D_tx;
        double sampling_rate_rx = f_clk / D_rx;
        std::string antenna_tx = "BAND1";
        std::string antenna_rx = "LNAL";
        double timeout = 2; //!< Read and write stream timeout

        size_t no_of_rx_samples_initial_sync =
                2 * sampling_rate_rx * burst_period; //!< Read buffer size
        size_t no_of_rx_samples_ping =
                (size_t)(1 * sampling_rate_rx * burst_period); //!< Read buffer size

        uint32_t ping_scr_code = 2;
        uint32_t pong_scr_code = 12;

        int64_t max_sync_error = 5;
        uint32_t threshold_factor = 16;
        size_t num_of_ping_tries = 1000; //!< Number of tries before initial sync
        int64_t ping_burst_guard = 2; //!< Guard samples around expected PING

        double pong_delay = 5e-3; //!< In tag, time from ping rx to pong tx

        bool tx_active = true;
        bool rx_active = true;

        bool is_beacon = true;
};
