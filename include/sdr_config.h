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

struct SDR_Device_Config
{
		std::string args = "";
		std::string clock_source = "";
		std::string time_source = "";

        double frequency = 500e6;
        double tx_gain = 60;
        double tx_bw = -1;

        double time_in_future = 1;
        double burst_period = 10e-3;
        double tx_burst_length = 0.5e-3;

		double f_clk = 37.8e6;
		short channel_tx = 0;
		short channel_rx = 0;
		uint16_t D_tx = 8;
		uint16_t D_rx = D_tx;
		std::string antenna_tx = "BAND1";
		std::string antenna_rx = "LNAL";
		double timeout = 2;
};
