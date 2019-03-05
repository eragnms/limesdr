/**
 * \file measure_delay.cpp
 *
 * \brief Measure round trip delay through RF loopback/leakage
 *
 * Measures round trip delay through loopback from TX to RX. Should support
 * LimeSDR-USB, BladeRF,...
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */
// https://github.com/skylarkwireless/sklk-soapyiris/blob/master/tests/IrisFullDuplex.cpp

#include "measure_delay.h"


int main()
{
        std::string args = "driver lime";
        SoapySDR::Device *device = SoapySDR::Device::make(args);
        if (device == nullptr)
        {
                std::cerr << "No device!" << std::endl;
                return EXIT_FAILURE;
        }
        if (!device->hasHardwareTime()) {
                std::cerr << "This device does not support timed streaming!"
                          << std::endl;
                return EXIT_FAILURE;
        }
        const double clock_rate = 0;
        if (clock_rate != 0) {
                device->setMasterClockRate(clock_rate);
        }
        const size_t rx_ch(0);
        const size_t tx_ch(0);
        const double sample_rate(10e+6);
        device->setSampleRate(SOAPY_SDR_RX, rx_ch, sample_rate);
        device->setSampleRate(SOAPY_SDR_TX, tx_ch, sample_rate);
        const double rx_gain(20);
        const double tx_gain(20);
        device->setGain(SOAPY_SDR_RX, rx_ch, rx_gain);
        device->setGain(SOAPY_SDR_TX, tx_ch, tx_gain);
        const double freq(1e+9);
        device->setFrequency(SOAPY_SDR_RX, rx_ch, freq);
        device->setFrequency(SOAPY_SDR_TX, tx_ch, freq);
        const double rx_bw(0);
        if (rx_bw != 0) {
                device->setBandwidth(SOAPY_SDR_RX, rx_ch, rx_bw);
        }
        const double tx_bw(0);
        if (tx_bw != 0) {
                device->setBandwidth(SOAPY_SDR_TX, tx_ch, tx_bw);
        }
}
