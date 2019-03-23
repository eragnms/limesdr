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
        Beacon beacon;
        beacon.open();
        beacon.configure();
        beacon.configure_rx_stream();
        beacon.configure_tx_stream();
        beacon.generate_modulation();
        beacon.activate_tx_stream();
        beacon.activate_rx_stream(0.1e9);
        uint32_t tx_time(0);
        for (size_t m=0; m<20; m++) {
                std::cout << "Loop: " << m << std::endl;
                beacon.send_tx_pulse(tx_time);
                usleep(100);
        }
        beacon.close_rx_stream();
        beacon.close_tx_stream();
        beacon.close();
        if (plot_data) {
                beacon.plot_data();
        }
        //beacon.save_data();

}
