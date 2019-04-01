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

void Analysis::add_data(std::vector<std::complex<float>> data)
{
        m_data.clear();
        m_data = data;
}

void Analysis::plot_data()
{
        arma::cx_vec y = arma::conv_to<arma::cx_vec>::from(m_data);
        plot(arma::real(y), "m");
}

void Analysis::save_data(std::string filename)
{
        arma::cx_vec y = arma::conv_to<arma::cx_vec>::from(m_data);
        std::string file;
        file = filename + "_re.arm";
        arma::vec re_data = arma::real(y);
        re_data.save(file, arma::raw_ascii);
        file = filename + "_im.arm";
        arma::vec im_data = arma::imag(y);
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
