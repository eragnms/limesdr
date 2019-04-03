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
        void configure(DetectorType det_type,
                       std::vector<uint32_t> codes,
                       SDR_Device_Config dev_cfg);
        void detect();
        std::vector<std::complex<float>> get_corr_result();
private:
        void detect_cdma();
        void correlate_cdma(uint32_t code_nr);
        arma::vec correlate(arma::vec a, arma::vec b);
        arma::cx_vec correlate(arma::cx_vec a, arma::cx_vec b);

        arma::cx_vec m_data;
        DetectorType m_det_type;
        std::vector<uint32_t> m_codes;
        SDR_Device_Config m_dev_cfg;
        arma::cx_vec m_corr_result;

};
