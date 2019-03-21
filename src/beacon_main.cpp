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
        beacon.configure_streams();
        beacon.generate_modulation();
        beacon.activate_tx_stream();
        beacon.activate_rx_stream(0.1e9);
        unsigned int microseconds(100000);
        usleep(microseconds);
        uint32_t tx_time = 0.1e9;
        for (size_t n=0; n<1; n++) {
                beacon.send_tx_pulse(tx_time);
                //usleep(10);
        }
        beacon.read_rx_data();
        beacon.calculate_tof();
        beacon.close_streams();
        beacon.close();
        if (plot_data) {
                beacon.plot_data();
        }
        //beacon.save_data();

}
