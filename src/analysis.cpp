/**
 * \file analysis.cpp
 *
 * \brief Analysis class
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "analysis.h"
#include "gnuplot_i.hpp"

Analysis::Analysis()
{}

void Analysis::add_data(std::vector<float> data)
{
        m_data.clear();
        m_data.set_size(data.size());
        m_data = arma::conv_to<arma::cx_vec>::from(data);
}

void Analysis::add_data(std::vector<std::complex<float>> data)
{
        m_data.clear();
        m_data.set_size(data.size());
        m_data = arma::conv_to<arma::cx_vec>::from(data);
}

void Analysis::add_data(std::vector<std::complex<int16_t>> data)
{
        m_data.clear();
        m_data.set_size(data.size());
        for (size_t n=0; n<data.size()-1; n++) {
                int16_t re = std::real(data[n]);
                int16_t im = std::imag(data[n]);
                m_data(n) = std::complex<double>((double)re, (double)im);
        }
}

void Analysis::add_data(arma::cx_vec data)
{
        m_data.clear();
        m_data.set_size(data.n_rows);
        m_data = data;
}

void Analysis::plot_real_data()
{
        plot(arma::real(m_data), "real part");
}

void Analysis::plot_imag_data()
{
        plot(arma::imag(m_data), "imag part");
}

void Analysis::plot_data()
{
        if (m_data.n_rows > 0) {
                plot(arma::abs(m_data), "absolute part");
        } else {
                std::cout << "Will not plot data as it is empty!"
                          << std::endl;
        }
}


void Analysis::save_data(std::string filename)
{
        std::string file;
        file = filename + "_re.arm";
        arma::vec re_data = arma::real(m_data);
        re_data.save(file, arma::raw_ascii);
        file = filename + "_im.arm";
        arma::vec im_data = arma::imag(m_data);
        im_data.save(file, arma::raw_ascii);
}

void Analysis::plot(std::vector<float> y, std::string title)
{
        Gnuplot g1("lines");
        g1.set_title(title);
        g1.plot_x(y);
        wait_for_key();
}

void Analysis::plot(std::vector<float> y)
{
        plot(y, "");
}

void Analysis::plot(arma::vec y, std::string title)
{
        std::vector<float> y_p = arma::conv_to<std::vector<float>>::from(y);
        plot(y_p, title);
}

void Analysis::plot(arma::vec y)
{
        plot(y);
}

void Analysis::wait_for_key()
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
