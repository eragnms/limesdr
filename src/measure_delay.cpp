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
        const size_t num_tx_samps(200);
        std::vector<double> tx_pulse = generate_cf32_pulse(num_tx_samps, 5,
                                                           0.3);
        //plot(tx_pulse);
        measure_delay();
        return EXIT_SUCCESS;
}

int measure_delay()
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
        double act_sample_rate =  device->getSampleRate(SOAPY_SDR_RX, rx_ch);
        std::cout << "Actal RX rate: " << act_sample_rate << " Msps" << std::endl;
        act_sample_rate =  device->getSampleRate(SOAPY_SDR_TX, tx_ch);
        std::cout << "Actal TX rate: " << act_sample_rate << " Msps" << std::endl;
        device->setAntenna(SOAPY_SDR_RX, rx_ch, "LNAL");
        device->setAntenna(SOAPY_SDR_TX, tx_ch, "BAND1");
        const double rx_gain(25);
        const double tx_gain(30);
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
        std::vector<double> tx_pulse = generate_cf32_pulse(num_tx_samps, 5,
                                                           0.3);
        void *tx_buffs[] = {tx_pulse.data()};
        // Transmit at 100 ms into the "future"
        uint32_t tx_time_0 = device->getHardwareTime() + 0.1e9;
        int tx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        uint32_t status = device->writeStream(tx_stream,
                                              tx_buffs,
                                              num_tx_samps,
                                              tx_flags, // compare with api!
                                              tx_time_0);
        if (status != num_tx_samps) {
                std::cerr << "Transmit failed!"
                          << std::endl;
                return EXIT_FAILURE;
        }

        // Receive slightly before transmit time
        uint32_t num_rx_samps(10000);
        std::vector<int16_t> rx_buffer(num_rx_samps, 0);
        const uint32_t rx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        uint32_t rate(10e6);
        double tx_rx_start_delta = (((double)num_rx_samps/rate) * 1e9 / 2);
        uint32_t receive_time = (uint32_t) (tx_time_0 - tx_rx_start_delta);
        device->activateStream(rx_stream, rx_flags, receive_time,
                               num_rx_samps);

        std::cout << "tx_time_0: " << tx_time_0 << std::endl;
        std::cout << "rx_start_shift: " << tx_rx_start_delta << std::endl;
        std::cout << "receive_time: " << receive_time << std::endl;

        uint32_t rx_time_0(0);
        size_t buffer_length(1024);
        std::vector<int16_t> rx_tmp_buff(buffer_length, 0);
        void *rx_buffs[] = {rx_tmp_buff.data()};
        size_t rx_buffer_index(0);
        while (true) {
                int rx_flags(0);
                uint32_t timeout(5e5);
                long long int rx_timestamp;
                int32_t status = device->readStream(rx_stream,
                                                    rx_buffs,
                                                    buffer_length,
                                                    rx_flags,
                                                    rx_timestamp,
                                                    timeout);
                if ((status > 0) && (rx_buffer_index == 0)) {
                        rx_time_0 = rx_timestamp;
                        MARK;
                }
                if (status > 0) {
                        /*rx_buffer.insert(rx_buffer.end(),
                                         rx_tmp_buff.begin(),
                                         rx_tmp_buff.begin()+status-1);*/
                        for (size_t n=0; n<(size_t)status; n++) {
                                rx_buffer[rx_buffer_index++];
                        }

                } else {
                        // All samples (num_rx_samps) read
                        break;
                }
        }
        std::cout << "Cleanup streams" << std::endl;
        device->deactivateStream(tx_stream);
        //device->closeStream(rx_stream);
        device->closeStream(tx_stream);

        if (rx_buffer.size() != num_rx_samps) {
                std::cerr << "Receive fail - not all samples captured" << std::endl;
                return EXIT_FAILURE;
        }
        if (rx_time_0 == 0) {
                std::cerr << "Receive fail - no valid timestamp" << std::endl;
                return EXIT_FAILURE;
        }
        arma::vec rx_data;
        rx_data.set_size(100);
        for (size_t n=0; n<100; n++){
                rx_data(n) = (double) rx_buffer[n];
        }
        // clear inital samples because transients
        double rx_mean = arma::mean(rx_data);
        size_t num_to_clear = (uint32_t) num_rx_samps / 100;
        for (size_t n=0; n<num_to_clear; n++) {
                rx_data(n) = rx_mean;
        }
        MARK;
        arma::vec tx_data;
        MARK;
        tx_data.set_size(10);
        MARK;
        for (size_t n=0; n<10; n++){
                tx_data(n) = (double) tx_pulse[n];
        }
        MARK;
        arma::vec tx_pulse_norm = normalize(tx_data);
        MARK;
        arma::vec rx_pulse_norm = normalize(rx_data);
        MARK;
        arma::uword rx_argmax_index = rx_data.index_max();
        arma::uword tx_argmax_index = tx_data.index_max();
        MARK;
        uint32_t tx_peak_time = (uint32_t) (tx_time_0 + (tx_argmax_index / rate) * 1e9);
        MARK;
        uint32_t rx_peak_time = (uint32_t) (rx_time_0 + (rx_argmax_index / rate) * 1e9);
        MARK;
        uint32_t time_delta = rx_peak_time - tx_peak_time;
        MARK;
        std::cout << "Time delta: " << time_delta << " us" << std::endl;

        return EXIT_SUCCESS;
}

/**
 * \fn Generate a sinc pulse
 */
std::vector<double> generate_cf32_pulse(size_t num_samps, uint32_t width,
                                         double scale_factor)
{
        arma::vec rel_time = arma::linspace(0, 2*width, num_samps) - width;
        arma::vec sinc_pulse = arma::sinc(rel_time);
        sinc_pulse = sinc_pulse * scale_factor;
        std::vector<double> pulse;
        pulse = arma::conv_to<std::vector<double>>::from(sinc_pulse);
        return pulse;
}

/**
 * \fn plot
 * \brief Use gnuplot-cpp to plot data
 *
 * See the example.cc file that comes with the package for
 * examples on how to use the library.
 *
 */
void plot(std::vector<double> y)
{
        Gnuplot g1("points");
        g1.reset_all();
        g1.set_title("Our data");
        g1.plot_x(y);
        wait_for_key();
}

void plot(arma::vec y)
{
        std::vector<double> y_p = arma::conv_to<std::vector<double>>::from(y);
        plot(y_p);
}

void wait_for_key ()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
        std::cout << std::endl << "Press any key to continue..." << std::endl;

        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        _getch();
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
        std::cout << std::endl << "Press ENTER to continue..." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::cin.rdbuf()->in_avail());
        std::cin.get();
#endif
        return;
}

void print_vec(const std::vector<int>& vec)
{
        for (auto x: vec) {
                std::cout << ' ' << x;
        }
        std::cout << std::endl;
}

arma::vec normalize(arma::vec samps) {
        //samps = samps - arma::mean(samps);
        //samps = arma::abs(samps);
        //samps = samps / arma::max(samps);
        return samps;
}
