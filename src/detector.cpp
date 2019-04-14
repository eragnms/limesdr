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
        arma::uvec found_bursts;
        if (m_det_type == CDMA) {
                found_bursts = detect_cdma_bursts();
        }
        index_of_sync = check_bursts_for_intial_sync_index(found_bursts);
        return index_of_sync;
}

int64_t Detector::look_for_pong(int64_t expected_ix)
{
        return look_for_ping(expected_ix);
}

int64_t Detector::look_for_ping(int64_t expected_ix)
{
        int64_t index_of_sync(-1);
        expected_ix += 0;
        int64_t adjust_ix = reduce_buffer_data(expected_ix);
        arma::uvec found_bursts;
        if (m_det_type == CDMA) {
                found_bursts = detect_cdma_bursts();
        }
        index_of_sync = check_bursts_for_ping_index(found_bursts);
        if (index_of_sync > 0) {
                index_of_sync += adjust_ix;
        }
        return index_of_sync;
}

int64_t Detector::reduce_buffer_data(int64_t expected_ix)
{
        size_t data_length = m_dev_cfg.tx_burst_length;
        data_length += 2 * m_dev_cfg.ping_burst_guard;
        int64_t start_pos = expected_ix - data_length / 2;
        uint64_t start_ix;
        if (start_pos < 0) {
                start_ix = 0;
        } else {
                start_ix = (uint64_t)start_pos;
        }
        uint64_t end_pos = (uint64_t)expected_ix + data_length / 2;
        uint64_t end_ix;
        if (end_pos >= m_data.n_rows) {
                end_ix = m_data.n_rows - 1;
        } else {
                end_ix = (uint64_t)end_pos;
        }
        data_length = end_ix - start_ix + 1;
        arma::cx_vec tmp = m_data;
        m_data.set_size(data_length);
        m_data = tmp.rows(start_ix, end_ix);
        return start_ix;
}

bool Detector::found_initial_sync(int64_t ix)
{
        return found_ok_index(ix);
}

bool Detector::found_pong(int64_t ix)
{
        return found_ping(ix);
}

bool Detector::found_ping(int64_t ix)
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

arma::uvec Detector::detect_cdma_bursts()
{
        arma::uvec peak_indexes;
        for (size_t n=0; n<m_codes.size(); n++) {
                correlate_cdma(m_codes[n]);
                double threshold = calculate_threshold();
                peak_indexes = find_peaks(threshold);
                if (peak_indexes.n_rows > 0) {
                        break;
                }
        }
        return peak_indexes;
}

int64_t Detector::check_bursts_for_intial_sync_index(arma::uvec peak_indexes)
{
        int64_t ix(-1);
        if (peak_indexes.n_rows > 0) {
                ix = find_initial_sync_ix(peak_indexes);
        }
        return ix;
}

int64_t Detector::check_bursts_for_ping_index(arma::uvec peak_indexes)
{
        int64_t ix(-1);
        if (peak_indexes.n_rows > 0) {
                ix = peak_indexes(0);
        }
        return ix;
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
        m_reference = arma::conv_to<arma::cx_vec>::from(tx_pulse);
        m_corr_result = correlate(m_reference);
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
        return arma::abs(arma::flipud(complex_corr));
}

double Detector::calculate_threshold()
{
        size_t length = m_reference.n_rows;
        size_t max_ix = m_corr_result.index_max();
        int64_t start_candidate = max_ix - length / 2;
        uint64_t start_ix;
        if (start_candidate < 0) {
                start_ix = 0;
        } else {
                start_ix = (uint64_t)(start_candidate);
        }
        uint64_t end_ix = (uint64_t)(max_ix + length / 2);
        if (end_ix >= m_corr_result.n_rows) {
                end_ix = m_corr_result.n_rows - 1;
        }
        arma::vec corr_data = m_corr_result.rows(start_ix, end_ix);
        double mean = arma::mean(corr_data);
        double standard_dev = arma::stddev(corr_data);
        double threshold = mean + m_dev_cfg.threshold_factor * standard_dev;
        return threshold;
}

arma::uvec Detector::find_peaks(double threshold)
{
        arma::uvec peaks = arma::find(m_corr_result > threshold);
        return arma::sort(peaks, "ascend");
}

int64_t Detector::find_initial_sync_ix(arma::uvec peak_indexes)
{
        int64_t sync_index(-1);
        size_t num_peaks = peak_indexes.n_rows;
        std::cout << "Num peaks: " << num_peaks << std::endl;
        std::cout << "index:amplitude " << std::flush;
        for (size_t n=0; n<num_peaks-1; n++) {
                std::cout << peak_indexes(n)
                          << ":"
                          << m_corr_result(peak_indexes(n))
                          << ", "
                          << std::flush;
                for (size_t m=n+1; m<num_peaks-1; m++) {
                        uint64_t spacing = peak_indexes(m) - peak_indexes(n);
                        if (spacing_ok(spacing)) {
                                sync_index = peak_indexes(m);
                                break;
                        }
                }
                if (sync_index != -1) {
                        break;
                }
        }
        std::cout << std::endl;
        return sync_index;
}

bool Detector::spacing_ok(int64_t burst_spacing)
{
        bool ok;
        int64_t burst_period =
                m_dev_cfg.burst_period * m_dev_cfg.sampling_rate_rx;
        int64_t max_diff = m_dev_cfg.max_sync_error;
        ok = burst_spacing <= (burst_period + max_diff);
        ok = ok && (burst_spacing >= (burst_period - max_diff));
        if (ok) {
                std::cout << "Spacing " << burst_spacing << std::endl;
        }
        return ok;
}
