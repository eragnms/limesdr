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

void Modulation::filter()
{
        arma::vec c_novs_8 = {7.298941506695379e-04, 1.012674740974449e-04,
                              -6.400057619412672e-04, -1.317153962281510e-03,
                              -1.746403846176797e-03, -1.779735081575717e-03,
                              -1.344436388157660e-03, -4.702061727525083e-04,
                              7.036942342949051e-04, 1.945162523354514e-03,
                              2.969009330533594e-03, 3.493821608581103e-03,
                              3.305667395036481e-03, 2.315743647984128e-03,
                              5.988670154180114e-04, -1.597789006581647e-03,
                              -3.881349490139374e-03, -5.774285130329619e-03,
                              -6.801706426747524e-03, -6.590495820966173e-03,
                              -4.961811109567827e-03, -1.997650857781989e-03,
                              1.934989864517913e-03, 6.212536166683968e-03,
                              1.003709785712439e-02, 1.256384744877031e-02,
                              1.305542734154087e-02, 1.103840652031024e-02,
                              6.433838249830143e-03, -3.642180039147574e-04,
                              -8.481715163876228e-03, -1.663806141580324e-02,
                              -2.329586276880056e-02, -2.686956962230974e-02,
                              -2.596439149278374e-02, -1.960969633644766e-02,
                              -7.449508483547277e-03, 1.014277661163307e-02,
                              3.204819670450183e-02, 5.649294303335348e-02,
                              8.123807589704976e-02, 1.038446413397558e-01,
                              .219781559828021e-01, 1.337093385877148e-01,
                              1.013653067138499e-01, 1.337093385877147e-01,
                              1.219781559828018e-01, 1.038446413397556e-01,
                              8.123807589704955e-02, 5.649294303335328e-02,
                              3.204819670450163e-02, 1.014277661163293e-02,
                              -7.449508483547271e-03, -1.960969633644754e-02,
                              -2.596439149278415e-02, -2.686956962230987e-02,
                              -2.329586276880055e-02, -1.663806141580318e-02,
                              -8.481715163876122e-03, -3.642180039146815e-04,
                              6.433838249830209e-03, 1.103840652031031e-02,
                              1.305542734154090e-02, 1.256384744877030e-02,
                              1.003709785712436e-02, 6.212536166683925e-03,
                              1.934989864517869e-03, -1.997650857782017e-03,
                              -4.961811109567867e-03, -6.590495820966187e-03,
                              -6.801706426747521e-03, -5.774285130329611e-03,
                              -3.881349490139347e-03, -1.597789006581629e-03,
                              5.988670154180322e-04, 2.315743647984139e-03,
                              3.305667395036484e-03, 3.493821608581100e-03,
                              2.969009330533590e-03, 1.945162523354505e-03,
                              7.036942342948869e-04, -4.702061727525164e-04,
                              -1.344436388157660e-03, -1.779735081575721e-03,
                              -1.746403846176796e-03, -1.317153962281502e-03,
                              -6.400057619412621e-04, 1.012674740974558e-04,
                              7.298941506695379e-04};

        arma::vec c_novs_4 = {1.513371479872678e-03, -1.326995792729155e-03,
                              -3.621015144071930e-03, -2.787570888839776e-03,
                              1.459048252073076e-03,  6.155980343428911e-03,
                              6.854011301507485e-03, 1.241698211362190e-03,
                              -8.047637614861131e-03, -1.410274148828852e-02,
                              -1.028787998211416e-02, 4.012031706402771e-03,
                              2.081104174314789e-02, 2.706928309826024e-02,
                              1.333999910052060e-02, -1.758609219931321e-02,
                              -4.830192745207036e-02, -5.383488762224386e-02,
                              -1.544590221435313e-02, 6.644912393043899e-02,
                              1.684400224738754e-01, 2.529110039620129e-01,
                              2.101720695919658e-01, 2.529110039620124e-01,
                              1.684400224738750e-01, 6.644912393043857e-02,
                              -1.544590221435312e-02, -5.383488762224471e-02,
                              -4.830192745207033e-02, -1.758609219931299e-02,
                              1.333999910052074e-02, 2.706928309826031e-02,
                              2.081104174314783e-02, 4.012031706402680e-03,
                              -1.028787998211425e-02, -1.410274148828852e-02,
                              -8.047637614861073e-03, 1.241698211362233e-03,
                              6.854011301507491e-03, 6.155980343428902e-03,
                              1.459048252073038e-03, -2.787570888839776e-03,
                              -3.621015144071929e-03, -1.326995792729144e-03,
                              1.513371479872678e-03};

        arma::vec c_novs_2 = {3.264402329740409e-03, -7.810673340644812e-03,
                              3.147224971935993e-03, 1.478437433127159e-02,
                              -1.735907364996029e-02, -2.219136532461194e-02,
                              4.489024278187957e-02, 2.877490736520164e-02,
                              -1.041891740412316e-01, -3.331742393990334e-02,
                              3.633318118506540e-01, 4.533494933313385e-01,
                              3.633318118506531e-01, -3.331742393990331e-02,
                              -1.041891740412316e-01, 2.877490736520193e-02,
                              4.489024278187944e-02, -2.219136532461211e-02,
                              -1.735907364996017e-02, 1.478437433127161e-02,
                              3.147224971935912e-03, -7.810673340644809e-03,
                              3.264402329740409e-03};

        arma::vec coeffs;
        if (m_Novs == 8) {
                coeffs = c_novs_8;
        } else if (m_Novs == 4) {
                coeffs = c_novs_4;
        } else {
                coeffs = c_novs_2;
        }
        arma::cx_vec data = arma::conv_to<arma::cx_vec>::from(m_data);
        arma::vec i_data = arma::conv(arma::real(data), coeffs);
        arma::vec q_data = arma::conv(arma::imag(data), coeffs);
        arma::cx_vec filtered_data(data.n_rows);
        uint32_t length = data.n_rows;
        filtered_data = i_data.rows(0, length-1) + J*q_data.rows(0, length-1);
        m_data = arma::conv_to<std::vector<std::complex<float>>>::from(
                filtered_data);
}

void Modulation::scrap_samples(size_t no_to_scrap)
{
        arma::cx_vec data = arma::conv_to<arma::cx_vec>::from(m_data);
        arma::cx_vec tmp(data.n_rows-no_to_scrap);
        bool odd_length = (data.n_rows & 1);
        if (odd_length) {
                tmp.set_size(tmp.n_rows + 1);
                tmp.rows(0,tmp.n_rows-2) = data.rows(no_to_scrap,
                                                     data.n_rows-1);
                tmp(tmp.n_rows-1) = 0;
        } else {
                tmp = data.rows(no_to_scrap, data.n_rows-1);
        }
        m_data = arma::conv_to<std::vector<std::complex<float>>>::from(tmp);
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
