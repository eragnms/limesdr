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
        CDMA /**< CDMA detector */
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
         * \brief Fetch data from the detector
         *
         * \return last data that was added to the detector
         */
        arma::cx_vec get_data();
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
         * \return index of (second) detected sync peak, -1 if sync failed
         */
        int64_t look_for_initial_sync();
        /**
         * \brief Look for PING bursts
         *
         * \return index of the first detected PING, -1 if sync failed
         */
        int64_t look_for_ping(int64_t expected_ix);
        /**
         * \brief Look for PONG bursts
         *
         * \return index of the first detected PONG, -1 if sync failed
         */
        int64_t look_for_pong(int64_t expected_ix);
        /**
         * \brief Get the correlation result
         *
         * \return a vector containing the correlation result
         */
        std::vector<float> get_corr_result();
        /**
         * \brief Check if initial sync index was found
         *
         * \return true if the index checked indicates that
         * initial sync has been found
         */
        bool found_initial_sync(int64_t ix);
        /**
         * \brief Check if ping index was found
         *
         * \return true if the index checked indicates that
         * a ping has been found
         */
        bool found_ping(int64_t ix);
        /**
         * \brief Check if pong index was found
         *
         * \return true if the index checked indicates that
         * a pong has been found
         */
        bool found_pong(int64_t ix);
private:
        arma::uvec detect_cdma_bursts();
        void correlate_cdma(uint32_t code_nr);
        arma::vec correlate(arma::vec ref, arma::vec rx_data);
        arma::vec correlate(arma::cx_vec ref);
        double calculate_threshold();
        arma::uvec find_peaks(double threshold);
        int64_t find_initial_sync_ix(arma::uvec peak_indexes);
        bool spacing_ok(int64_t burst_spacing);
        bool found_ok_index(int64_t ix);
        int64_t check_bursts_for_intial_sync_index(arma::uvec peak_indexes);
        int64_t check_bursts_for_ping_index(arma::uvec peak_indexes);
        int64_t reduce_buffer_data(int64_t expected_ix, int64_t guard);

        arma::cx_vec m_data;
        DetectorType m_det_type;
        std::vector<uint32_t> m_codes;
        SDR_Device_Config m_dev_cfg;
        arma::vec m_corr_result;
        arma::cx_vec m_reference;
        bool m_is_beacon;
};
