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
        std::vector<size_t> rx_channel;
        std::vector<size_t> tx_channel;
        SoapySDR::Stream *rx_stream = device->setupStream(SOAPY_SDR_RX,
                                                            SOAPY_SDR_CF32,
                                                            rx_channel);
        SoapySDR::Stream *tx_stream = device->setupStream(SOAPY_SDR_TX,
                                                            SOAPY_SDR_CF32,
                                                            tx_channel);
        uint32_t microseconds(1e+6);
        usleep(microseconds);
        device->activateStream(tx_stream);
        const size_t num_tx_samps(200);
        std::vector<int16_t> tx_pulse = generate_cf32_pulse(num_tx_samps, 5,
                                                            0.3);
        void *buffs[] = {tx_pulse.data()};
        uint32_t tx_time_0 = device->getHardwareTime() + 0.1e9;
        int tx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        uint32_t status = device->writeStream(tx_stream,
                                              buffs,
                                              num_tx_samps,
                                              tx_flags, // compare with api!
                                              tx_time_0);

        if (status != num_tx_samps) {
                std::cerr << "Transmit failed!"
                          << std::endl;
                return EXIT_FAILURE;
        }

        //rx_buffs = np.array([], np.complex64)
        const uint32_t rx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        //receive_time = int(tx_time_0 - ((num_rx_samps)/rate) * 1e9 / 2)
        const uint32_t receive_time(0);
        const size_t num_rx_samps(10000);
        device->activateStream(rx_stream, rx_flags, receive_time,
                               num_rx_samps);
}

std::vector<int16_t> generate_cf32_pulse(size_t num_samps, uint32_t width,
                                         double scale_factor)
{
        arma::vec rel_time = arma::linspace(-width, width, num_samps);
        arma::vec sinc_pulse = arma::sinc(rel_time);
        sinc_pulse = sinc_pulse * scale_factor;
        std::vector<int16_t> pulse;
        pulse = arma::conv_to<std::vector<int16_t>>::from(sinc_pulse);
        return pulse;
}
