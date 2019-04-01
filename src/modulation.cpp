/**
 * \file modulation.cpp
 *
 * \brief Modulation class
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "modulation.h"


Modulation::Modulation(size_t no_of_samples,
                       double scale_factor,
                       uint16_t Novs)
{
        m_no_of_samples = no_of_samples;
        m_scale_factor = scale_factor;
        m_Novs = Novs;
}

void Modulation::generate_sine(double tone_freq,
                               double sampling_rate)
{
        m_data.clear();
        double f_ratio = tone_freq/sampling_rate;
        for (size_t i = 0; i < m_no_of_samples; i++) {
                const double pi = acos(-1);
                double w = 2*pi*i*f_ratio;
                m_data.push_back(std::complex<float>(cos(w)*m_scale_factor,
                                                     sin(w)*m_scale_factor));
        }
}

void Modulation::generate_cdma(uint16_t code_nr)
{
        m_data.clear();
        arma::cx_vec scr_code(m_no_of_samples);
        gen_scr_code(code_nr, scr_code);
        arma::cx_vec scr_code_ovs = repvecN(scr_code);
        for (size_t n=0; n<scr_code_ovs.n_rows; n++) {
                std::complex<double> tmp = scr_code_ovs(n);
                tmp *= (double)m_scale_factor;
                m_data.push_back((std::complex<float>)tmp);
        }

}

std::vector<std::complex<float>> Modulation::get_data()
{
        return m_data;
}

void Modulation::gen_scr_code(uint16_t code_nr, arma::cx_vec & Z)
{
        arma::vec x = arma::zeros<arma::vec>(18);
        x(0) = 1;
        arma::vec y = arma::ones<arma::vec>(18);
        shift_N(x,y,code_nr);

        y = arma::ones(18);

        arma::vec I = arma::zeros<arma::vec>(m_no_of_samples);
        arma::vec Q = arma::zeros<arma::vec>(m_no_of_samples);
        for (size_t i=0; i<m_no_of_samples; i++) {
                int8_t tmp_I = mod_2(x(0) + y(0));
                I(i) = 1-2*tmp_I;
                int8_t tmp_Q_x = mod_2(x(4) + x(6) + x(15));
                int8_t tmp_Q_y = mod_2(y(5)+ y(6) + sum(y.rows(8,15)));
                Q(i) = 1-2*mod_2(tmp_Q_x + tmp_Q_y);
                shift_N(x,y,1);
        }

        for (size_t n=0; n<m_no_of_samples; n++) {
                Z(n) = 1/sqrt(2) * std::complex<double>(I(n), Q(n));
        }
}

arma::cx_vec Modulation::repvecN(arma::cx_vec vect)
{
        uint32_t length = vect.n_rows;
        arma::cx_vec tmp = arma::zeros<arma::cx_vec>(vect.n_rows * m_Novs);
        for (uint32_t n=0; n<length ; n++ ) {
                for (uint16_t m=0; m<m_Novs; m++ ) {
                        tmp(n*m_Novs+m) = vect(n);
                }
        }
        return tmp;
}

void Modulation::shift_N(arma::vec & x, arma::vec & y, int32_t N_shifts)
{
        for (int32_t i=0; i<N_shifts; i++) {
                int8_t x_tmp = mod_2(x(0) + x(7));
                int8_t y_tmp = mod_2(y(0) + y(5) + y(7) + y(10));
                x.rows(0,16) = x.rows(1,17);
                x(17) = x_tmp;
                y.rows(0,16) = y.rows(1,17);
                y(17) = y_tmp;
        }
}

int8_t Modulation::mod_2(double x)
{
        return (int8_t) floor(2*(x/2-floor(x/2)));
}
