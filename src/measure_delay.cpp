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
// https://github.com/skylarkwireless/sklk-soapyiris/blob/master/tests/IrisFullDuplex.cpp

#include "measure_delay.h"

int main()
{
        Beacon beacon;
        beacon.open();
        beacon.configure();
        beacon.measure_tof();
        return EXIT_SUCCESS;
}

Beacon::Beacon()
{
        m_sample_rate = 10e+6;
        m_num_tx_samps = 200;
        m_num_rx_samps = 10000;
}

void Beacon::open()
{
        std::string args = "driver lime";
        m_device = SoapySDR::Device::make(args);
        if (m_device == nullptr)
        {
                std::cerr << "No device!" << std::endl;
        }
        if (!m_device->hasHardwareTime()) {
                std::cerr << "This device does not support timed streaming!"
                          << std::endl;
        }
}

void Beacon::configure()
{
        const double clock_rate = 0;
        if (clock_rate != 0) {
                m_device->setMasterClockRate(clock_rate);
        }
        const size_t rx_ch(0);
        const size_t tx_ch(0);
        m_device->setSampleRate(SOAPY_SDR_RX, rx_ch, m_sample_rate);
        m_device->setSampleRate(SOAPY_SDR_TX, tx_ch, m_sample_rate);
        double act_sample_rate = m_device->getSampleRate(SOAPY_SDR_RX, rx_ch);
        std::cout << "Actual RX rate: " << act_sample_rate << " Msps" << std::endl;
        act_sample_rate = m_device->getSampleRate(SOAPY_SDR_TX, tx_ch);
        std::cout << "Actual TX rate: " << act_sample_rate << " Msps" << std::endl;
        m_device->setAntenna(SOAPY_SDR_RX, rx_ch, "LNAL");
        m_device->setAntenna(SOAPY_SDR_TX, tx_ch, "BAND1");
        const double rx_gain(25);
        const double tx_gain(30);
        m_device->setGain(SOAPY_SDR_RX, rx_ch, rx_gain);
        m_device->setGain(SOAPY_SDR_TX, tx_ch, tx_gain);
        const double freq(1e+9);
        m_device->setFrequency(SOAPY_SDR_RX, rx_ch, freq);
        m_device->setFrequency(SOAPY_SDR_TX, tx_ch, freq);
        const double rx_bw(0);
        if (rx_bw != 0) {
                m_device->setBandwidth(SOAPY_SDR_RX, rx_ch, rx_bw);
        }
        const double tx_bw(0);
        if (tx_bw != 0) {
                m_device->setBandwidth(SOAPY_SDR_TX, tx_ch, tx_bw);
        }
}

void Beacon::configure_tx_stream()
{

}

void Beacon::measure_tof()
{
        const bool plot_data(false);


        std::vector<size_t> rx_channel;
        std::vector<size_t> tx_channel;
        auto tx_stream = m_device->setupStream(SOAPY_SDR_TX,
                                               SOAPY_SDR_CF32,
                                               tx_channel);
        auto rx_stream = m_device->setupStream(SOAPY_SDR_RX,
                                               SOAPY_SDR_CF32,
                                               rx_channel);
        uint32_t microseconds(1e+6);
        usleep(microseconds);
        m_device->activateStream(tx_stream);

        std::vector<double> tx_pulse = generate_cf32_pulse(m_num_tx_samps, 5,
                                                           0.3);
        void *tx_buffs[] = {tx_pulse.data()};
        // Transmit at 100 ms into the "future"
        uint32_t tx_time_0 = m_device->getHardwareTime() + 0.1e9;
        int tx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        uint32_t status = m_device->writeStream(tx_stream,
                                                tx_buffs,
                                                m_num_tx_samps,
                                                tx_flags, // compare with api!
                                                tx_time_0);
        if (status != m_num_tx_samps) {
                std::cerr << "Transmit failed!"
                          << std::endl;
        }

        // Receive slightly before transmit time
        arma::cx_vec rx_data(m_num_rx_samps);
        int rx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        double tx_rx_start_delta = (((double)m_num_rx_samps/m_sample_rate) * 1e9 / 2);
        uint32_t receive_time = (uint32_t) (tx_time_0 - tx_rx_start_delta);
        m_device->activateStream(rx_stream, rx_flags, receive_time,
                               m_num_rx_samps);

        uint32_t rx_time_0(0);
        size_t buffer_length(1024);
        std::vector<std::complex<float>> rx_buff(buffer_length);
        std::vector<void *> rx_buffs(1);
        size_t rx_buffer_index(0);
        uint32_t timeout(5e5);
        long long int rx_timestamp(0);

        while (true) {
                rx_buffs[0] = rx_buff.data();
                int32_t status = m_device->readStream(rx_stream,
                                                    rx_buffs.data(),
                                                    buffer_length,
                                                    rx_flags,
                                                    rx_timestamp,
                                                    timeout);
                if ((status > 0) && (rx_buffer_index == 0)) {
                        rx_time_0 = rx_timestamp;
                }
                if (status > 0) {
                        for (size_t n=0; n<(size_t)status; n++) {
                                rx_data(rx_buffer_index) = rx_buff[n];
                                rx_buffer_index++;
                        }

                } else {
                        // All samples (num_rx_samps) read
                        break;
                }
        }
        std::cout << rx_data(0) << std::endl;
        std::cout << "Cleanup streams" << std::endl;
        m_device->deactivateStream(rx_stream);
        m_device->deactivateStream(tx_stream);
        m_device->closeStream(rx_stream);
        m_device->closeStream(tx_stream);
        SoapySDR::Device::unmake(m_device);

        if (rx_buffer_index != m_num_rx_samps) {
                std::cerr << "Receive fail - not all samples captured"
                          << std::endl;
        }
        if (rx_time_0 == 0) {
                std::cerr << "Receive fail - no valid timestamp" << std::endl;
        }

        std::complex<double> rx_mean = arma::mean(rx_data);
        size_t num_to_clear = (uint32_t) m_num_rx_samps / 100;
        for (size_t n=0; n<num_to_clear; n++) {
                rx_data(n) = rx_mean;
        }

        arma::cx_vec tx_data;
        tx_data.set_size(m_num_tx_samps);
        for (size_t n=0; n<m_num_tx_samps; n++){
                tx_data(n) = (double) tx_pulse[n];
        }
        arma::vec tx_pulse_norm = normalize(tx_data);
        arma::vec rx_data_norm = normalize(rx_data);
        arma::uword tx_argmax_index = tx_pulse_norm.index_max();
        arma::vec rx_tx_corr = arma::conv(arma::flipud(tx_pulse_norm),
                                          rx_data_norm);
        arma::uword rx_corr_index = rx_tx_corr.index_max() - m_num_tx_samps / 2;
        int32_t tx_peak_time = peak_time(tx_time_0, tx_argmax_index, m_sample_rate);
        int32_t rx_peak_time = peak_time(rx_time_0, rx_corr_index, m_sample_rate);
        int32_t time_delta = (rx_peak_time - tx_peak_time) / 1e3;

        std::cout << "Time delta: " << time_delta << " us" << std::endl;
        std::cout << "rx_corr_index: " << rx_corr_index << std::endl;
        std::cout << "Num samples received: " << rx_buffer_index << std::endl;

        if (plot_data) {
                //plot(tx_pulse);
                //plot(tx_pulse_norm);
                plot(rx_tx_corr);
        }
}

int32_t peak_time(uint32_t ref_time, arma::uword argmax_ix, uint32_t rate)
{
        return (int32_t)(ref_time + ((double)argmax_ix / rate) * 1e9);
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
        Gnuplot g1("lines");
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

arma::vec normalize(arma::cx_vec samps) {
        samps = samps - arma::mean(samps);
        arma::vec samps_norm = arma::abs(samps);
        samps_norm = samps_norm / arma::max(samps_norm);
        return samps_norm;
}
