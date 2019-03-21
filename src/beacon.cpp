/**
 * \file beacon.cpp
 *
 * \brief Beacon class
 *
 * Class for the beacon
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "beacon.h"
#include "gnuplot_i.hpp"

void Beacon::save_data()
{
        save(m_rx_data);
}

void Beacon::plot_data()
{
        //arma::cx_vec tmp = arma::conv_to<arma::cx_vec>::from(m_tx_pulse);
        //plot(arma::real(tmp), "tx pulse");
        /*  plot(arma::real(m_rx_data), "rx data real");
          plot(arma::imag(m_rx_data), "rx data imag");
          std::complex<double> m = arma::mean(m_rx_data);
          tmp = m_rx_data - m;
          std::cout << "rx mean: " << m << std::endl;
          plot(arma::abs(m_rx_data), "rx data norm");*/
        plot(m_rx_tx_corr, "correlation result");
}

Beacon::Beacon()
{
        m_tx_bw = 6.00e6;

        /*m_novs_tx = 1;
        m_num_tx_samps = 200 * m_tx_bw / 3.84e6;
        m_sample_rate_rx = 10e+6;
        m_sample_rate_tx = m_sample_rate_rx;*/

        // CDMA pulse settings
        m_novs_tx = 2;
        m_num_tx_samps = (size_t) (256);
        m_sample_rate_rx = 48.00e+6;
        m_sample_rate_tx = m_novs_tx * m_tx_bw;

        m_num_rx_samps = 200000;
        m_time_delta = 0;
}

void Beacon::generate_modulation()
{
        //m_tx_pulse = generate_cf32_pulse(m_num_tx_samps, 5, 0.3);
        //m_tx_pulse = generate_cf32_pulses_with_zeros(m_num_tx_samps, 5, 0.3);
        //m_tx_pulse = generate_ramp(m_num_tx_samps, 0.9);
        m_tx_pulse = generate_cdma_scr_code_pulse(m_num_tx_samps, 0.9, 2);
        m_tx_pulse_ref = generate_cdma_scr_code_pulse(m_num_tx_samps, 0.9, 2);
        //m_tx_pulse = generate_cdma_scr_code_pulse_const(m_num_tx_samps, 0.9);
        std::cout << "Pulse length: " << m_tx_pulse.size() << std::endl;
        m_tx_buffs.push_back(m_tx_pulse.data());
}

void Beacon::calculate_tof()
{
        //calculate_tof_sinc();
        calculate_tof_cdma();
}

void Beacon::open()
{
        //std::string args = "driver bladerf";
        //std::string args = "driver lime";
        //m_device = SoapySDR::Device::make(args);
        m_device = SoapySDR::Device::make();
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
        m_device->setSampleRate(SOAPY_SDR_RX, rx_ch, m_sample_rate_rx);
        m_device->setSampleRate(SOAPY_SDR_TX, tx_ch, m_sample_rate_tx);
        double act_sample_rate = m_device->getSampleRate(SOAPY_SDR_RX, rx_ch);
        std::cout << "Actual RX rate: " << act_sample_rate << " Msps" << std::endl;
        act_sample_rate = m_device->getSampleRate(SOAPY_SDR_TX, tx_ch);
        std::cout << "Actual TX rate: " << act_sample_rate << " Msps" << std::endl;
        m_device->setAntenna(SOAPY_SDR_RX, rx_ch, "LNAL");
        m_device->setAntenna(SOAPY_SDR_TX, tx_ch, "BAND1");
        const double rx_gain(20);
        const double tx_gain(40);
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

void Beacon::configure_streams()
{
        std::vector<size_t> rx_channel;
        std::vector<size_t> tx_channel;

        m_tx_stream = m_device->setupStream(SOAPY_SDR_TX,
                                            SOAPY_SDR_CF32,
                                            tx_channel);
        m_rx_stream = m_device->setupStream(SOAPY_SDR_RX,
                                            SOAPY_SDR_CF32,
                                            rx_channel);
        uint32_t microseconds(1e+6);
        usleep(microseconds);
}

void Beacon::activate_tx_stream()
{
        m_device->activateStream(m_tx_stream);
        m_tx_time_0 = m_device->getHardwareTime();
}

void Beacon::send_tx_pulse(uint32_t tx_delta_time)
{
        uint32_t tx_time_0 = m_tx_time_0 + tx_delta_time;
        //uint32_t tx_time_0 = m_device->getHardwareTime() + tx_delta_time;
        int tx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        uint32_t status = m_device->writeStream(m_tx_stream,
                                                m_tx_buffs.data(),
                                                m_num_tx_samps*m_novs_tx,
                                                tx_flags, // compare with api!
                                                tx_time_0);
        if (status != (m_num_tx_samps*m_novs_tx)) {
                std::cerr << "Transmit failed!"
                          << std::endl;
        }
}

void Beacon::activate_rx_stream(uint32_t tx_start_time)
{
        m_rx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        double start_delta = (((double)m_num_rx_samps/m_sample_rate_rx)*1e9/2);
        uint32_t receive_time = (uint32_t) (m_tx_time_0 + tx_start_time - start_delta);
        std::cout << "TX time: " << m_tx_time_0 << std::endl;
        std::cout << "RX time: " << receive_time << std::endl;
        std::cout << "Delta: " << start_delta << std::endl;
        m_device->activateStream(m_rx_stream, m_rx_flags, receive_time,
                                 m_num_rx_samps);
}

void Beacon::read_rx_data()
{
        m_rx_time_0 = 0;
        size_t buffer_length(1024);
        std::vector<std::complex<float>> rx_buff(buffer_length);
        std::vector<void *> rx_buffs(1);
        m_rx_buffer_index = 0;
        uint32_t timeout(5e5);
        long long int rx_timestamp(0);
        m_rx_data.set_size(m_num_rx_samps);
        while (true) {
                rx_buffs[0] = rx_buff.data();
                int32_t status = m_device->readStream(m_rx_stream,
                                                      rx_buffs.data(),
                                                      buffer_length,
                                                      m_rx_flags,
                                                      rx_timestamp,
                                                      timeout);
                if ((status > 0) && (m_rx_buffer_index == 0)) {
                        m_rx_time_0 = rx_timestamp;
                }
                if (status > 0) {
                        for (size_t n=0; n<(size_t)status; n++) {
                                m_rx_data(m_rx_buffer_index) = rx_buff[n];
                                m_rx_buffer_index++;
                        }

                } else {
                        // All samples (num_rx_samps) read
                        break;
                }
        }
        if (m_rx_buffer_index != m_num_rx_samps) {
                std::cerr << "Receive fail - not all samples captured"
                          << std::endl;
        }
        if (m_rx_time_0 == 0) {
                std::cerr << "Receive fail - no valid timestamp" << std::endl;
        }
}

void Beacon::close_streams()
{
        std::cout << "Cleanup streams" << std::endl;
        m_device->deactivateStream(m_rx_stream);
        m_device->deactivateStream(m_tx_stream);
        m_device->closeStream(m_rx_stream);
        m_device->closeStream(m_tx_stream);
}

void Beacon::close()
{
        SoapySDR::Device::unmake(m_device);
}

void Beacon::calculate_tof_sinc()
{
        std::complex<double> rx_mean = arma::mean(m_rx_data);
        size_t num_to_clear = (uint32_t) m_num_rx_samps / 100;
        for (size_t n=0; n<num_to_clear; n++) {
                m_rx_data(n) = rx_mean;
        }

        arma::cx_vec tx_data;
        tx_data.set_size(m_num_tx_samps);
        for (size_t n=0; n<m_num_tx_samps; n++){
                tx_data(n) = m_tx_pulse[n];
        }
        arma::vec tx_pulse_norm = normalize(tx_data);
        arma::vec rx_data_norm = normalize(m_rx_data);
        arma::uword tx_argmax_index = tx_pulse_norm.index_max();
        m_rx_tx_corr = correlate(tx_pulse_norm, rx_data_norm);
        arma::uword rx_corr_index = m_rx_tx_corr.index_max()-m_num_tx_samps/2;
        int32_t tx_peak_time = peak_time(m_tx_time_0, tx_argmax_index,
                                         m_sample_rate_tx);
        int32_t rx_peak_time = peak_time(m_rx_time_0, rx_corr_index,
                                         m_sample_rate_rx);
        m_time_delta = (rx_peak_time - tx_peak_time) / 1e3;

        std::cout << "Time delta: " << m_time_delta << " us" << std::endl;
        std::cout << "rx_corr_index: " << rx_corr_index << std::endl;
        std::cout << "Num samples received: "
                  << m_rx_buffer_index
                  << std::endl;
}

void Beacon::calculate_tof_cdma()
{
        std::complex<double> rx_mean = arma::mean(m_rx_data);
        size_t num_to_clear = (uint32_t) m_num_rx_samps / 100;
        for (size_t n=0; n<num_to_clear; n++) {
                m_rx_data(n) = rx_mean;
        }

        arma::cx_vec tx_data;
        tx_data.set_size(m_num_tx_samps*m_novs_tx);
        for (size_t n=0; n<tx_data.n_rows; n++){
                tx_data(n) = m_tx_pulse_ref[n];
        }

        arma::cx_vec tx_data_ref = repvecN(
          (uint16_t)(m_sample_rate_rx/m_sample_rate_tx),
          tx_data);
        //arma::cx_vec tx_data_ref = repvecN(4, tx_data);

        std::cout << "TX ref length: " << tx_data_ref.n_rows << std::endl;
        std::cout << "RX data length: " << m_rx_data.n_rows << std::endl;

        m_rx_tx_corr_complex = correlate(m_rx_data, tx_data_ref);
        m_rx_tx_corr = arma::abs(m_rx_tx_corr_complex);
        arma::uword rx_corr_index = m_rx_tx_corr.index_max();

        std::cout << "rx_corr_index: " << rx_corr_index << std::endl;
        std::cout << "Num samples received: "
                  << m_rx_buffer_index
                  << std::endl;

        double tmp1(0);
        double tmp2(0);
        for (int16_t n=-6; n<=6; n++) {
                tmp1 = tmp1 + (rx_corr_index + n)*m_rx_tx_corr(rx_corr_index+n);
                tmp2 = tmp2 + m_rx_tx_corr(rx_corr_index+n);
        }
        double cog = tmp1/tmp2;
        std::cout << "CoG: " << cog << std::endl;

        m_time_delta = (int32_t)cog;
}

arma::vec Beacon::correlate(arma::vec a, arma::vec b)
{
        return arma::conv(arma::flipud(a), b);
}

arma::cx_vec Beacon::correlate(arma::cx_vec a, arma::cx_vec b)
{
        arma::vec real_corr = correlate(arma::real(a), arma::real(b));
        arma::vec imag_corr = correlate(arma::imag(a), arma::imag(b));
        arma::cx_vec complex_corr(a.n_rows);
        for (size_t n=0; n<a.n_rows; n++)
        {
                complex_corr(n) = std::complex<double>(real_corr(n),
                                                       imag_corr(n));
        }
        return complex_corr;
}


int32_t Beacon::get_tof()
{
        return m_time_delta;
}

int32_t Beacon::peak_time(uint32_t ref_time, arma::uword argmax_ix,
                          double sample_rate)
{
        return (int32_t)(ref_time + ((double)argmax_ix / sample_rate)*1e9);
}

/**
 * \fn Generate a sinc pulse
 */
std::vector<std::complex<float>> Beacon::generate_cf32_pulse(
        size_t num_samps,
        uint32_t width,
        double scale_factor)
{
        arma::vec rel_time = arma::linspace(0, 2*width, num_samps) - width;
        arma::vec sinc_pulse = arma::sinc(rel_time);
        sinc_pulse = sinc_pulse * scale_factor;
        std::vector<std::complex<float>> pulse;
        for (size_t n=0; n<num_samps; n++) {
                pulse.push_back(std::complex<float>((float)sinc_pulse(n), 0));
        }
        return pulse;
}

std::vector<std::complex<float>> Beacon::generate_cf32_pulses_with_zeros(
        size_t num_samps,
        uint32_t width,
        double scale_factor)
{
        size_t n_samples = (size_t) num_samps/3;
        arma::vec rel_time = arma::linspace(0, 2*width, n_samples) - width;
        arma::vec sinc_pulse = arma::sinc(rel_time);
        sinc_pulse = sinc_pulse * scale_factor;
        std::vector<std::complex<float>> pulse;
        for (size_t n=0; n<n_samples; n++) {
                pulse.push_back(std::complex<float>((float)sinc_pulse(n), 0));
        }
        for (size_t n=0; n<n_samples; n++) {
                pulse.push_back(std::complex<float>(0, 0));
        }
        for (size_t n=0; n<n_samples; n++) {
                pulse.push_back(std::complex<float>((float)sinc_pulse(n), 0));
        }
        return pulse;
}

std::vector<std::complex<float>> Beacon::generate_ramp(size_t num_samps,
                                                       double scale_factor)
{
        float start_value(0);
        float end_value(256);
        size_t num_pre_0(10);
        size_t num_post_0(10);
        size_t num_ramp_samps = num_samps-num_pre_0-num_post_0;
        float delta = (end_value - start_value) / num_ramp_samps;
        std::vector<std::complex<float>> ramp;
        float value(start_value);
        for (size_t n=0; n<num_pre_0; n++) {
                ramp.push_back(std::complex<float>(0, 0));
        }
        for (size_t n=0; n<num_ramp_samps; n++) {
                ramp.push_back(std::complex<float>(value*scale_factor/256,0));
                value += delta;
        }
        for (size_t n=0; n<num_post_0; n++) {
                ramp.push_back(std::complex<float>(0, 0));
        }
        return ramp;
}

std::vector<std::complex<float>> Beacon::generate_cdma_scr_code_pulse(
        size_t num_samps,
        double scale_factor,
        uint16_t code_nr)
{
        arma::cx_vec scr_code(num_samps);
        gen_scr_code(code_nr, scr_code, num_samps);
        arma::cx_vec scr_code_ovs = repvecN(m_novs_tx, scr_code);
        std::vector<std::complex<float>> v;
        for (size_t n=0; n<scr_code_ovs.n_rows; n++) {
                v.push_back((std::complex<float>)scr_code_ovs(n)*(float)scale_factor);
        }
        return v;
}

std::vector<std::complex<float>> Beacon::generate_cdma_scr_code_pulse_const(
        size_t num_samps,
        double scale_factor)
{
        arma::cx_vec scr_code(1);
        scr_code(0) = std::complex<double>(1, 1);
        arma::cx_vec scr_code_ovs = repvecN(num_samps, scr_code);
        std::vector<std::complex<float>> v;
        for (size_t n=0; n<scr_code_ovs.n_rows; n++) {
                v.push_back((std::complex<float>)scr_code_ovs(n)*(float)scale_factor);
        }
        return v;
}


void Beacon::gen_scr_code(uint16_t code_nr, arma::cx_vec & Z, size_t num_samps)
{
        arma::vec x = arma::zeros<arma::vec>(18);
        x(0) = 1;
        arma::vec y = arma::ones<arma::vec>(18);
        shift_N(x,y,code_nr);

        y = arma::ones(18);

        arma::vec I = arma::zeros<arma::vec>(num_samps);
        arma::vec Q = arma::zeros<arma::vec>(num_samps);
        for (size_t i=0; i<num_samps; i++) {
                int8_t tmp_I = mod_2(x(0) + y(0));
                I(i) = 1-2*tmp_I;
                int8_t tmp_Q_x = mod_2(x(4) + x(6) + x(15));
                int8_t tmp_Q_y = mod_2(y(5)+ y(6) + sum(y.rows(8,15)));
                Q(i) = 1-2*mod_2(tmp_Q_x + tmp_Q_y);
                shift_N(x,y,1);
        }

        for (size_t n=0; n<num_samps; n++) {
                Z(n) = 1/sqrt(2) * std::complex<double>(I(n), Q(n));
        }
}

arma::cx_vec Beacon::repvecN(uint16_t Novs, arma::cx_vec vect)
{
        uint32_t length = vect.n_rows;
        arma::cx_vec tmp = arma::zeros<arma::cx_vec>(vect.n_rows * Novs);
        for (uint32_t n=0; n<length ; n++ ) {
                for (uint16_t m=0; m<Novs; m++ ) {
                        tmp(n*Novs+m) = vect(n);
                }
        }
        return tmp;
}

int Beacon::shift_N(arma::vec & x, arma::vec & y, int32_t N_shifts)
{
        for (int32_t i=0; i<N_shifts; i++) {
                int8_t x_tmp = mod_2(x(0) + x(7));
                int8_t y_tmp = mod_2(y(0) + y(5) + y(7) + y(10));
                x.rows(0,16) = x.rows(1,17);
                x(17) = x_tmp;
                y.rows(0,16) = y.rows(1,17);
                y(17) = y_tmp;
        }

        return 0;

}

int8_t Beacon::mod_2(double x)
{
        return (int8_t) floor(2*(x/2-floor(x/2)));
}

arma::vec Beacon::normalize(arma::cx_vec samps)
{
        samps = samps - arma::mean(samps);
        arma::vec samps_norm = arma::abs(samps);
        samps_norm = samps_norm / arma::max(samps_norm);
        return samps_norm;
}

/**
 * \fn plot
 * \brief Use gnuplot-cpp to plot data
 *
 * See the example.cc file that comes with the package for
 * examples on how to use the library.
 *
 */
void Beacon::plot(std::vector<double> y, std::string title)
{
        Gnuplot g1("lines");
        //g1.reset_all();
        g1.set_title(title);
        g1.plot_x(y);
        wait_for_key();
}

void Beacon::plot(std::vector<double> y)
{
        plot(y, "");
}

void Beacon::plot(arma::vec y, std::string title)
{
        std::vector<double> y_p = arma::conv_to<std::vector<double>>::from(y);
        plot(y_p, title);
}

void Beacon::plot(arma::vec y)
{
        plot(y);
}

void Beacon::wait_for_key()
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

void Beacon::print_vec(const std::vector<int>& vec)
{
        for (auto x: vec) {
                std::cout << ' ' << x;
        }
        std::cout << std::endl;
}

void Beacon::save(arma::vec data, std::string filename)
{
        std::string file;
        file = filename + ".arm";
        data.save(file, arma::raw_ascii);
}

void Beacon::save_with_header(arma::vec data, std::string filename)
{
        std::string file;
        file = filename + ".arm";
        data.save(file, arma::arma_ascii);
}

void Beacon::save(arma::vec data)
{
        save(data, "data");
}

void Beacon::save(arma::cx_vec data)
{
        arma::vec i_data(data.n_rows);
        arma::vec q_data(data.n_rows);
        i_data = arma::real(data);
        q_data = arma::imag(data);
        save(i_data, "i_data");
        save(q_data, "q_data");
}
