/**
 * \file measure_delay.h
 *
 * \brief Measure round trip delay through RF loopback/leakage
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

std::vector<int16_t> generate_cf32_pulse(size_t num_samps, uint32_t width,
                                         double scale_factor);
