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

#include "tag_main.h"

int main(int argc, char** argv)
{
        try {
                bool enable_version_and_help(true);
                TCLAP::CmdLine cmd("Beacon main application",
                                   ' ',
                                   PACKAGE_STRING,
                                   enable_version_and_help);
                TCLAP::SwitchArg plot_switch("p","plot-data",
                                             "Plot data",
                                             cmd, false);
                cmd.parse(argc, argv);
                bool plot_data = plot_switch.getValue();
                run_beacon(plot_data);
        }
        catch (TCLAP::ArgException &e) {
                std::cerr << "error: " << e.error()
                          << " for arg " << e.argId() << std::endl;
        }
        return EXIT_SUCCESS;
}

void run_beacon(bool plot_data)
{
        uint32_t num_rx_samps(4.8e6);
        Beacon beacon(num_rx_samps);
        beacon.open();
        beacon.configure();
        beacon.configure_rx_stream();
        beacon.generate_modulation();
        //uint64_t time_before_tx_start(1e9);
        beacon.activate_rx_stream();
        beacon.read_rx_data();
        beacon.close_rx_stream();
        beacon.close();
        std::cout << "Analyzing received data..." << std::endl;
        beacon.calculate_tof();
        std::cout << "Analyze done!" << std::endl;
        if (plot_data) {
                beacon.plot_data();
        }
        //beacon.save_data();
}
