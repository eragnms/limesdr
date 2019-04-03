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

#include "macros.h"

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
         *
         * \param[in] no_of_samples the number of samples to be generated
         * \param[in] scale_factor scales the amplitude of the generated data
         * \param[in] Novs over sampling factor, each sample will be repeated
         * (Novs-1) number of times, i.e. Novs=1 will have no impact on the
         * data
         */
        Modulation(size_t no_of_samples,
                   double scale_factor,
                   uint16_t Novs);
        /**
         * \brief Generate a sine
         *
         * \param[in] tone_freq the frequency of the sine
         * \param[in] sampling_rate the sampling rate of the tx transmission
         */
        void generate_sine(double tone_freq, double sampling_rate);
        /**
         * \brief Generate CDMA sequence
         *
         * Will generate a CDMA scrambling code, as specified in the 3GPP
         * specifications
         *
         * \param[in] code_nr the code number to generate
         */
        void generate_cdma(uint16_t code_nr);
        /**
         * \brief Get generated data
         *
         * \return the generated data
         */
        std::vector<std::complex<float>> get_data();
        /**
         * \brief Filter the generated pulse
         *
         * The filtering will use different coefficients based on
         * the oversampling factor Novs.
         */
        void filter();
        /**
         * \brief Scrap samples
         *
         * Throw away a number of samples in the beginning of the
         * modulated data vector
         *
         * \param[in] no_to_scrap the number of samples to throw away
         */
        void scrap_samples(size_t no_to_scrap);
private:
        void gen_scr_code(uint16_t code_nr, arma::cx_vec & Z);
        arma::cx_vec repvecN(arma::cx_vec vect);
        void shift_N(arma::vec & x, arma::vec & y, int32_t N_shifts);
        int8_t mod_2(double x);

        std::vector<std::complex<float>> m_data;
        arma::cx_vec m_data_arma;
        size_t m_no_of_samples;
        double m_scale_factor;
        uint16_t m_Novs;
};
