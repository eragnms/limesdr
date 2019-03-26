/**
 * \file tag_test.h
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
#include <chrono>
#include <math.h>
#include <armadillo>
#include "lime/LimeSuite.h"
#include "gnuplot_i.hpp"

void run_tag(bool plot_data);
void plot(std::vector<double> y, std::string title);
void plot(std::vector<double> y);
void plot(arma::vec y, std::string title);
void plot(arma::vec y);
void wait_for_key();
int error();
