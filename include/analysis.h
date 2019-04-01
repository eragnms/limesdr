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
        void add_data(std::vector<std::complex<float>> data);
        /**
         * \brief Plot data
         *
         * Plot the data that has been added with the
         * add_data method
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

        std::vector<std::complex<float>> m_data;



};
