/**
 * \file beacon_main.cpp
 *
 * \brief Beacon functionality
 *
 * The main functionality for the beacon.
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "beacon_main.h"

int main()
{
        Beacon beacon;
        size_t num_tofs(1);
        arma::vec tofs(num_tofs);
        for (size_t n=0; n<num_tofs; n++) {
                beacon.open();
                beacon.configure();
                beacon.configure_streams();
                beacon.generate_modulation();
                beacon.activate_streams();
                beacon.read_rx_data();
                beacon.calculate_tof();
                tofs(n) = beacon.get_tof();
                beacon.close_streams();
                beacon.close();
                beacon.plot_data();
        }
        std::cout << "Average TOF: " << arma::mean(tofs) << std::endl;
        std::cout << "Max TOF: " << arma::max(tofs) << std::endl;
        std::cout << "Min TOF: " << arma::min(tofs) << std::endl;
        //beacon.save_data();
        return EXIT_SUCCESS;
}
