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
 * Do the ...
 *
 */
class Analysis
{
public:
        /**
         * \brief Analysis constructor
         */
        Analysis();
        void add_data(std::vector<std::complex<float>> data);
        void plot_data();
        void save_data(std::string filename);
private:
        void plot(std::vector<float> y, std::string title);
        void plot(std::vector<float> y);
        void plot(arma::vec y, std::string title);
        void plot(arma::vec y);
        void wait_for_key();

        std::vector<std::complex<float>> m_data;



};
