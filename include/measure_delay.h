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

#include "gnuplot_i.hpp"

int measure_delay();
std::vector<double> generate_cf32_pulse(size_t num_samps, uint32_t width,
                                        double scale_factor);
void plot(std::vector<double> y);
void plot(arma::vec y);
void wait_for_key();
void print_vec(const std::vector<int>& vec);
arma::vec normalize(arma::vec samps);
