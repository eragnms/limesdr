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

private:
        int32_t peak_time(uint32_t ref_time, arma::uword argmax_ix);
        std::vector<double> generate_cf32_pulse(size_t num_samps,
                                                uint32_t width,
                                                double scale_factor);
        void plot(std::vector<double> y);
        void plot(arma::vec y);
        void wait_for_key();
        void print_vec(const std::vector<int>& vec);
        arma::vec normalize(arma::cx_vec samps);

        SoapySDR::Device *m_device;
        double m_sample_rate;
        size_t m_num_tx_samps;
        size_t m_num_rx_samps;
        SoapySDR::Stream *m_tx_stream;
        SoapySDR::Stream *m_rx_stream;
        std::vector<double> m_tx_pulse;
        std::vector<void *> m_tx_buffs;
        uint32_t m_tx_time_0;
        int m_rx_flags;
        arma::cx_vec m_rx_data;
        uint32_t m_rx_time_0;
        size_t m_rx_buffer_index;
        bool m_plot_data;
        int32_t m_time_delta;
};
