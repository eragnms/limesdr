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

int64_t Detector::look_for_initial_sync()
{
        int64_t index_of_sync(-1);
        if (m_det_type == CDMA) {
                index_of_sync = detect_cdma();
        }
        return index_of_sync;
}

bool Detector::found_initial_sync(int64_t ix)
{
        return found_ok_index(ix);
}

bool Detector::found_ping(int64_t ix)
{
        return found_ok_index(ix);
}

bool Detector::found_pong(int64_t ix)
{
        return found_ok_index(ix);
}

bool Detector::found_ok_index(int64_t ix)
{
        return ix >= 0;
}

std::vector<float> Detector::get_corr_result()
{
        std::vector<float> data;
        data = arma::conv_to<std::vector<float>>::from(m_corr_result);
        return data;
}

int64_t Detector::detect_cdma()
{
        int64_t index_of_sync(-1);
        for (size_t n=0; n<m_codes.size(); n++) {
                correlate_cdma(m_codes[n]);
                double threshold = calculate_threshold();
                arma::uvec peak_indexes = find_peaks(threshold);
                int64_t ix(-1);
                if (peak_indexes.n_rows > 0) {
                        ix = find_sync_ix(peak_indexes);
                }
                if (found_sync(ix)) {
                        index_of_sync = ix;
                        break;
                }
        }
        return index_of_sync;
}

void Detector::correlate_cdma(uint32_t code_nr)
{
        double scale_factor(1.0);
        uint16_t Novs = m_dev_cfg.Novs_rx;
        double extra_samples_for_filter = m_dev_cfg.extra_samples_filter;
        size_t mod_length = m_dev_cfg.tx_burst_length_chip;
        mod_length = mod_length * (1 + extra_samples_for_filter);
        Modulation modulation(mod_length, scale_factor, Novs);
        modulation.generate_cdma(code_nr);
        modulation.filter();
        modulation.scrap_samples(mod_length * extra_samples_for_filter);
        std::vector<std::complex<float>> tx_pulse = modulation.get_data();
        arma::cx_vec reference = arma::conv_to<arma::cx_vec>::from(tx_pulse);
        m_corr_result = correlate(reference);
}

arma::vec Detector::correlate(arma::vec ref, arma::vec rx_data)
{
        return arma::conv(ref, arma::flipud(rx_data));
}

arma::vec Detector::correlate(arma::cx_vec ref)
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
        return arma::abs(complex_corr);
}

double Detector::calculate_threshold()
{
        double mean = arma::mean(m_corr_result);
        double standard_dev = arma::stddev(m_corr_result);
        double threshold = mean + m_dev_cfg.threshold_factor * standard_dev;
        return threshold;
}

arma::uvec Detector::find_peaks(double threshold)
{
        arma::uvec peaks = arma::find(m_corr_result > threshold);
        return peaks;
}

int64_t Detector::find_sync_ix(arma::uvec peak_indexes)
{
        int64_t sync_index(-1);
        for (size_t n=0; n<peak_indexes.n_rows-1; n++) {
                uint64_t delta = peak_indexes(n+1) - peak_indexes(n);
                if (delta_ok(delta)) {
                        sync_index = peak_indexes(n);
                        break;
                }
        }
        return sync_index;
}

bool Detector::delta_ok(int64_t delta)
{
        bool ok;
        int64_t burst_distance =
                m_dev_cfg.burst_period * m_dev_cfg.sampling_rate_rx;
        int64_t ok_error = m_dev_cfg.max_sync_error;
        ok = delta <= (burst_distance + ok_error);
        ok = ok && (delta >= (burst_distance - ok_error));
        return ok;
}

bool Detector::found_sync(int64_t ix)
{
        return ix >= 0;
}
