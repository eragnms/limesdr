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
#include "analysis.h"
#include "detector.h"

void run_tag(bool plot_data);
void sigIntHandler(const int);
void list_device_info();
void check_return(int ret);
