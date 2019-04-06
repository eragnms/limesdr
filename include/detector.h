/**
 * \file detector.h
 *
 * \brief Detector class
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
#include "sdr_config.h"
#include "modulation.h"

/**
 * \brief enum
 *
 * Enum for picking detector
 */
enum DetectorType {
        CDMA
};


/**
 * \class Detector
 *
 * \brief Detector class
 *
 * This class supports data detection
 *
 */
class Detector
{
public:
        /**
         * \brief Detector constructor
         */
        Detector();
        /**
         * \brief Add data for detection
         *
         * \param[in] data the data to be analysed for detection
         */
        void add_data(std::vector<std::complex<float>> data);
        /**
         * \brief Add data for detection
         *
         * \param[in] data the data to be analysed for detection
         */
        void add_data(std::vector<std::complex<int16_t>> data);
        /**
         * \brief Set up parameters used in the detector
         *
         * \param[in] det_type defines what detector to use
         * \param[in] codes a vector of cdma scrambling code numbers
         * to search for
         * \param[in] dev_cfg configuration paramaters
         */
        void configure(DetectorType det_type,
                       std::vector<uint32_t> codes,
                       SDR_Device_Config dev_cfg);
        /**
         * \brief Look for initial sync
         *
         * \return index of detected sync peak, -1 if sync failed
         */
        int64_t initial_sync();
        /**
         * \brief Get the correlation result
         *
         * \return a vector containing the correlation result
         */
        std::vector<float> get_corr_result();
private:
        int64_t detect_cdma();
        void correlate_cdma(uint32_t code_nr);
        arma::vec correlate(arma::vec ref, arma::vec rx_data);
        arma::vec correlate(arma::cx_vec ref);
        double calculate_threshold();
        arma::uvec find_peaks(double threshold);
        int64_t find_sync_ix(arma::uvec peak_indexes);
        bool found_sync(int64_t ix);
        bool delta_ok(int64_t delta);

        arma::cx_vec m_data;
        DetectorType m_det_type;
        std::vector<uint32_t> m_codes;
        SDR_Device_Config m_dev_cfg;
        arma::vec m_corr_result;

};
