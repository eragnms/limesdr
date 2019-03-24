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
        uint32_t num_rx_samps(200000);
        Beacon beacon(num_rx_samps);
        beacon.open();
        beacon.configure();
        beacon.configure_rx_stream();
        beacon.configure_tx_stream();
        beacon.generate_modulation();
        uint64_t time_before_tx_start(1e9);
        beacon.activate_rx_stream();
        beacon.activate_tx_stream(time_before_tx_start);
        uint32_t time_between_pulses(0.01e9); // [ns]
        uint32_t transmit_time(20); // [s]
        size_t num_pulses(transmit_time*1e9/time_between_pulses);
        std::cout << "Number of TX pulses: " << num_pulses << std::endl;
        for (size_t m=0; m<num_pulses; m++) {
        //while (true) {
                beacon.send_tx_pulse(time_between_pulses);
                //usleep(time_between_pulses/1e3); // [us]
        }
        std::cout << "cleaning up!" << std::endl;
        usleep(num_pulses * time_between_pulses / 1e3);
        beacon.close_rx_stream();
        beacon.close_tx_stream();
        beacon.close();
        if (plot_data) {
                beacon.plot_data();
        }
        //beacon.save_data();

}
