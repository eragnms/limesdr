/**
 * \file modulation.h
 *
 * \brief Modulation class
 *
 * Class for the differet modulations used in the project
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */
#pragma once

#include <vector>
#include <complex>
#include <armadillo>

/**
 * \class Modulation
 *
 * \brief Modulation class
 *
 * Generating the modulation and returning vectors of modulated data
 *
 */
class Modulation
{
public:
        /**
         * \brief Modulation constructor
         */
        Modulation(size_t no_of_samples,
                   double scale_factor,
                   uint16_t Novs);
        void generate_sine(double tone_freq, double sampling_rate);
        void generate_cdma(uint16_t code_nr);
        std::vector<std::complex<float>> get_data();
private:
        void gen_scr_code(uint16_t code_nr, arma::cx_vec & Z);
        arma::cx_vec repvecN(arma::cx_vec vect);
        void shift_N(arma::vec & x, arma::vec & y, int32_t N_shifts);
        int8_t mod_2(double x);

        std::vector<std::complex<float>> m_data;
        size_t m_no_of_samples;
        double m_scale_factor;
        uint16_t m_Novs;
};
