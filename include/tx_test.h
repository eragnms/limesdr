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

void run_test();
void sigIntHandler(const int);
