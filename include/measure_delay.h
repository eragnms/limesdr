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

#define J (std::complex<double>(0,1))

#define STRINGIFY(x) #x
#define TOSTRING(x) STRINGIFY(x)
/**
 * \def TRACE
 *
 * Used for debugging. Simply creates an "I am here!" string.
 *
 * \ingroup group_Macros
 */
#define TRACE "Reached: " __FILE__ ":" TOSTRING(__LINE__)
/**
 * \def MARK
 *
 * Used for debugging. Simply prints an "I am here!" statement to cout.
 * See: http://www.decompile.com/cpp/faq/file_and_line_error_string.htm
 *
 * \ingroup group_Macros
 */
#define MARK std::cout << TRACE << std::endl;

class Beacon
{
public:
        Beacon();
        void open();
        void configure();
        void configure_streams();
        void generate_modulation();
        void activate_streams();
        void read_rx_data();
        void close_streams();
        void calculate_tof();
        void close();
        int32_t get_tof();
        void plot_data();
        void calibrate();
        void save_data();

private:
        int32_t peak_time(uint32_t ref_time, arma::uword argmax_ix,
                          double sample_rate);
        std::vector<std::complex<float>> generate_cf32_pulse(
                size_t num_samps,
                uint32_t width,
                double scale_factor);
        std::vector<double> generate_cdma_scr_code_pulse(size_t num_samps);
        void gen_scr_code(uint16_t code_nr, arma::cx_vec & Z,
                          size_t num_samps);
        int shift_N(arma::vec & x, arma::vec & y, int32_t N_shifts);
        int8_t mod_2(double x);
        void plot(std::vector<double> y, std::string title);
        void plot(std::vector<double> y);
        void plot(arma::vec y, std::string title);
        void plot(arma::vec y);
        void wait_for_key();
        void print_vec(const std::vector<int>& vec);
        arma::vec normalize(arma::cx_vec samps);
        arma::cx_vec repvecN(uint16_t Novs, arma::cx_vec vect);
        arma::vec correlate(arma::vec a, arma::vec b);
        arma::cx_vec correlate(arma::cx_vec a, arma::cx_vec b);
        void calculate_tof_sinc();
        void calculate_tof_cdma();
        std::vector<std::complex<float>> generate_ramp(size_t num_samps);
        void save(arma::vec data, std::string filename);
        void save_with_header(arma::vec data, std::string filename);
        void save(arma::vec data);
        void save(arma::cx_vec data);

        SoapySDR::Device *m_device;
        double m_sample_rate_rx;
        double m_sample_rate_tx;
        size_t m_num_tx_samps;
        size_t m_num_rx_samps;
        SoapySDR::Stream *m_tx_stream;
        SoapySDR::Stream *m_rx_stream;
        std::vector<std::complex<float>> m_tx_pulse;
        std::vector<void *> m_tx_buffs;
        uint32_t m_tx_time_0;
        int m_rx_flags;
        arma::cx_vec m_rx_data;
        uint32_t m_rx_time_0;
        size_t m_rx_buffer_index;
        int32_t m_time_delta;
        arma::vec m_rx_tx_corr;
        arma::cx_vec m_rx_tx_corr_complex;
        uint32_t m_tx_bw;
        uint32_t m_novs_tx;
        uint32_t m_novs_rx;
};
