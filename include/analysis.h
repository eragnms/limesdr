/**
 * \file analysis.h
 *
 * \brief Analysis class
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
 * \class Analysis
 *
 * \brief Analysis class
 *
 * This class supports data analysis of different forms.
 *
 */
class Analysis
{
public:
        /**
         * \brief Analysis constructor
         */
        Analysis();
        /**
         * \brief Add data for analysis
         *
         * \param[in] data the data to be analysed
         */
        void add_data(std::vector<float> data);
        /**
         * \brief Add data for analysis
         *
         * \param[in] data the data to be analysed
         */
        void add_data(std::vector<std::complex<float>> data);
        /**
         * \brief Add data for analysis
         *
         * \param[in] data the data to be analysed
         */
        void add_data(std::vector<std::complex<int16_t>> data);
        /**
         * \brief Add data for analysis
         *
         * \param[in] data the data to be analysed
         */
        void add_data(arma::cx_vec data);
        /**
         * \brief Plot real data
         *
         * Plot the real part of the data that has been added with
         * the add_data method
         */
        void plot_real_data();
        /**
         * \brief Plot imag data
         *
         * Plot the imaginary part of the data that has been added with
         * the add_data method
         */
        void plot_imag_data();
        /**
         * \brief Plot data
         *
         * Plot the absolute part of the data that has been added with
         * the add_data method
         */
        void plot_data();

        /**
         * \brief Save data to a file
         *
         * Save data to a file, that has been added with
         * the add_data method
         *
         * \param[in] filename filename, with no given extension
         */
        void save_data(std::string filename);
private:
        void plot(std::vector<float> y, std::string title);
        void plot(std::vector<float> y);
        void plot(arma::vec y, std::string title);
        void plot(arma::vec y);
        void wait_for_key();

        arma::cx_vec m_data;



};
