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

/**
 * \struct SDR_Device_Config
 *
 * \brief SDR device configuration parameters
 *
 * Configuration parameters for the SDR. Set the parameters either in this
 * file, or change them in the file configuring the SDR.
 */
#include <SoapySDR/Logger.hpp>

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

        double frequency = 500e6; //!< Center frequency [Hz]
        double tx_gain = 60; //!< 60 dB is about 0 dBm
        double rx_gain = 20;
        double tx_bw = -1; //!< Not used if -1
        double rx_bw = -1; //!< Not used if -1

        double time_in_future = 1;
        double burst_period = 10e-3;
        double tx_burst_length = 0.1024e-3;
        size_t no_of_rx_samples = 1500;

		double f_clk = 40.0e6;
		short channel_tx = 0;
		short channel_rx = 0;
		uint16_t D_tx = 8;
		uint16_t D_rx = D_tx;
        double sampling_rate = f_clk / D_tx;
		std::string antenna_tx = "BAND1";
		std::string antenna_rx = "LNAL";
		double timeout = 2;

        bool tx_active = true;
        bool rx_active = true;
};
