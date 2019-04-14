/**
 * \file beacon_main.h
 *
 * \brief Beacon functionality
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */
#pragma once

#include <tclap/CmdLine.h>
#include <iostream>
#include <SoapySDR/Device.hpp>
#include <SoapySDR/Formats.hpp>
#include <SoapySDR/Errors.hpp>
#include <SoapySDR/Time.hpp>
#include <SoapySDR/Logger.hpp>
#include <unistd.h>
#include <chrono>
#include <math.h>
#include <signal.h>
#include <future>

#include "sdr_config.h"
#include "sdr.h"
#include "modulation.h"
#include "analysis.h"
#include "detector.h"

typedef std::chrono::_V2::system_clock::time_point TimePoint;

void run_beacon();
void sigIntHandler(const int);
void list_device_info();
void transmit_ping(SDR sdr, int64_t tx_start_tick);
TimePoint print_spin(TimePoint time_last_spin, int spin_index);
int64_t calculate_tx_start_tick(int64_t now_tick);
int64_t ticks_per_period(double period);
int64_t look_for_pong(SDR sdr, int64_t tx_start_tick);
bool return_ok(int ret);
