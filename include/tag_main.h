/**
 * \file tag_main.h
 *
 * \brief Tag functionality
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
#include <unistd.h>
#include <chrono>
#include <math.h>
#include <armadillo>
#include <signal.h>

#include "lime/LimeSuite.h"
#include "macros.h"
#include "sdr_config.h"
#include "sdr.h"
#include "analyser.h"
#include "detector.h"

/**
 * \brief enum
 *
 * Keeping track of the status machine in the tag
 *
 */
enum TagStateMachine {
        INITIAL_SYNC, /**< Trying to get initial synchronization */
        SEARCH_FOR_PING, /**< Trying to find ping messages */
        SEND_PONG /**< Send PONG burst */
};

void run_tag(bool plot_data, uint32_t device);
void sigIntHandler(const int);
void list_device_info();
bool return_ok(int ret, size_t expected_num_samples);
std::string state_to_string(TagStateMachine state);
bool time_for_initial_sync(size_t num_of_missed_pings,
                           SDR_Device_Config dev_cfg);
