/**
 * \file detector.cpp
 *
 * \brief Detector class
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "detector.h"

Detector::Detector()
{}

void Detector::configure(DetectorType det_type,
                         std::vector<uint32_t> codes,
                         SDR_Device_Config dev_cfg)
{
        m_det_type = det_type;
        m_codes = codes;
        m_dev_cfg = dev_cfg;

}

void Detector::add_data(std::vector<std::complex<float>> data)
{
        m_data.clear();
        m_data.set_size(data.size());
        m_data = arma::conv_to<arma::cx_vec>::from(data);
}

void Detector::add_data(std::vector<std::complex<int16_t>> data)
{
        m_data.clear();
        m_data.set_size(data.size());
        for (size_t n=0; n<data.size()-1; n++) {
                int16_t re = std::real(data[n]);
                int16_t im = std::imag(data[n]);
                m_data(n) = std::complex<double>((double)re, (double)im);
        }
}

void Detector::detect()
{
        if (m_det_type == CDMA) {
                detect_cdma();
        }
}

std::vector<std::complex<float>> Detector::get_corr_result()
{
        std::vector<std::complex<float>> data;
        for (size_t n=0; n<m_corr_result.n_rows; n++) {
                data.push_back((std::complex<float>) m_corr_result(n));
        }
        return data;
}

void Detector::detect_cdma()
{
        for (size_t n=0; n<m_codes.size(); n++) {
                correlate_cdma(m_codes[n]);

        }
}

void Detector::correlate_cdma(uint32_t code_nr)
{
        double scale_factor(1.0);
        uint16_t Novs = m_dev_cfg.Novs_rx;
        double extra_samples_for_filter = m_dev_cfg.extra_samples_filter;
        size_t mod_length = m_dev_cfg.tx_burst_length_chip;
        mod_length = mod_length * (1 + extra_samples_for_filter);
        Modulation modulation(mod_length, scale_factor, Novs);
        std::cout << "Code generated: " << code_nr << std::endl;
        modulation.generate_cdma(code_nr);
//        modulation.filter();
        modulation.scrap_samples(mod_length * extra_samples_for_filter);
        std::vector<std::complex<float>> tx_pulse = modulation.get_data();
        arma::cx_vec reference = arma::conv_to<arma::cx_vec>::from(tx_pulse);
        //m_corr_result = correlate_2(reference, m_data);
        arma::cx_vec a = {std::complex<double>(1,1),
                          std::complex<double>(2,2),
                          std::complex<double>(3,3)};
        arma::cx_vec b = {std::complex<double>(1,1),
                          std::complex<double>(2,2),
                          std::complex<double>(3,3),
                          std::complex<double>(4,4)};
        m_corr_result = correlate(reference);
}

arma::vec Detector::correlate(arma::vec ref, arma::vec rx_data)
{
        return arma::conv(ref, arma::flipud(rx_data));
}

arma::cx_vec Detector::correlate(arma::cx_vec ref)
{
        arma::vec c_re = correlate(arma::real(ref), arma::real(m_data));
        arma::vec c_im = correlate(arma::imag(ref), arma::imag(m_data));
        arma::vec c_mix_1 = correlate(arma::real(ref), arma::imag(m_data));
        arma::vec c_mix_2 = correlate(arma::imag(ref), arma::real(m_data));
        size_t length = c_re.n_rows;
        arma::cx_vec complex_corr(length);
        for (size_t n=0; n<length; n++)
        {
                complex_corr(n) = std::complex<double>(
                        c_re(n) + c_im(n),
                        -c_mix_1(n) + c_mix_2(n));
        }
        return complex_corr;
}
