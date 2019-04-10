/**
 * \file tag_main.cpp
 *
 * \brief Tag functionality
 *
 * The main functionality for the tag.
 * Run from the commandline with -h for a list of options.
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "tag_main.h"

SoapySDR::Device *device(nullptr);

static sig_atomic_t stop = false;
void sigIntHandler(const int)
{
    stop = true;
}

int main(int argc, char** argv)
{
        try {
                bool enable_version_and_help(true);
                TCLAP::CmdLine cmd("Tag main application",
                                   ' ',
                                   PACKAGE_STRING,
                                   enable_version_and_help);
                TCLAP::SwitchArg start_switch("s","start",
                                              "Start tag",
                                              cmd, false);
                TCLAP::SwitchArg plot_switch("p","plot",
                                             "Plot data",
                                             cmd, false);
                TCLAP::SwitchArg list_switch("l","list-devices",
                                             "List info on attached devices",
                                             cmd, false);
                cmd.parse(argc, argv);
                bool start_tag = start_switch.getValue();
                bool plot_data = plot_switch.getValue();
                bool list_dev_info = list_switch.getValue();
                if (list_dev_info) {
                        list_device_info();
                }
                if (start_tag) {
                        run_tag(plot_data);
                }

        }
        catch (TCLAP::ArgException &e) {
                std::cerr << "error: " << e.error()
                          << " for arg " << e.argId() << std::endl;
        }
        return EXIT_SUCCESS;
}

void list_device_info()
{
        SDR sdr;
        sdr.connect();
        sdr.list_hw_info();
}

void run_tag(bool plot_data)
{
        SDR_Device_Config dev_cfg;
        std::string dev_serial = dev_cfg.serial_bladerf_x40;
        //std::string dev_serial = dev_cfg.serial_bladerf_xA4;
        //std::string dev_serial = dev_cfg.serial_lime_3;
        dev_cfg.tx_active = false;
        const size_t no_of_samples_initial_sync =
                dev_cfg.no_of_rx_samples_initial_sync;
        const size_t no_of_samples_ping =
                dev_cfg.no_of_rx_samples_ping;
        dev_cfg.tx_active = false;

        std::cout << "No of samples to read in initial sync: "
                  << no_of_samples_initial_sync << std::endl;

        SDR sdr;
        SoapySDR::setLogLevel(dev_cfg.log_level);
        sdr.connect(dev_serial);
        sdr.configure(dev_cfg);
        sdr.start();

        //auto time_last_spin = std::chrono::high_resolution_clock::now();
        auto time_status_info = std::chrono::high_resolution_clock::now();
        //int spinIndex(0);

        std::vector<std::complex<int16_t>> buff_data_initial(
                no_of_samples_initial_sync);
        std::vector<std::complex<int16_t>> buff_data_ping(
                no_of_samples_ping);
        Detector detector;
        detector.configure(CDMA, {dev_cfg.ping_scr_code}, dev_cfg);

        size_t num_of_found_pings(0);
        size_t num_of_missed_pings(0);
        size_t tot_num_of_missed_pings(0);
        size_t num_ping_tries(0);
        int64_t sync_ix(-1);
        int64_t hw_time_of_sync(-1);
        TagStateMachine current_state(INITIAL_SYNC);
        std::cout << "**********************" << std::endl;
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        std::cout << "Looking for inital sync" << std::endl;
        signal(SIGINT, sigIntHandler);
        size_t num_syncs(0);
        //int64_t old_hw_time(0);
        while (not stop) {
                switch(current_state) {
                case INITIAL_SYNC: {
                        int ret = sdr.read(no_of_samples_initial_sync,
                                           buff_data_initial);
                        if (return_ok(ret)) {
                                detector.add_data(buff_data_initial);
                                sync_ix = detector.look_for_initial_sync();

                                stop = true;
                                std::cout << "no of samples to fetch "
                                          << no_of_samples_initial_sync
                                          << " ret " << ret << std::endl;

                                if (detector.found_initial_sync(sync_ix)) {
                                        num_syncs++;
                                        hw_time_of_sync = sdr.ix_to_hw_time(
                                                sync_ix);
                                        std::cout << std::endl
                                                  << "Found inital sync"
                                                  << std::endl;
                                        current_state = SEARCH_FOR_PING;
                                }
                        }
                        break;
                }
                case SEARCH_FOR_PING: {
                        int ret = sdr.read(no_of_samples_ping,
                                           buff_data_ping);
                        if (return_ok(ret)) {
                                num_ping_tries++;
                                int64_t expected_ping_ix;
                                expected_ping_ix = sdr.expected_ping_pos_ix(
                                        hw_time_of_sync);
                                detector.add_data(buff_data_ping);
                                sync_ix = detector.look_for_ping(
                                        expected_ping_ix);
                                if (detector.found_ping(sync_ix)) {
                                        hw_time_of_sync = sdr.ix_to_hw_time(
                                                sync_ix);
                                        num_of_found_pings++;
                                        num_of_missed_pings = 0;
                                        std::cout << "Found PING"
                                                  << " expected "
                                                  << expected_ping_ix
                                                  << " sync ix "
                                                  << sync_ix
                                                  << " diff "
                                                  << expected_ping_ix-sync_ix
                                                  << " data_length "
                                                  << buff_data_ping.size()
                                                  << std::endl;
                                } else {
                                        num_of_missed_pings++;
                                        tot_num_of_missed_pings++;
                                }
                                if (num_of_missed_pings > dev_cfg.num_of_ping_tries) {
                                        num_of_missed_pings = 0;
                                        std::cout << std::endl
                                                  << "Faild PING detect"
                                                  << std::endl;
                                        std::cout << "Starting initial sync"
                                                  << std::endl;
                                        current_state = INITIAL_SYNC;
                                }
                        }
                        break;
                }
                default:
                        throw std::runtime_error("Unknown state tag!");
                }
                const auto now = std::chrono::high_resolution_clock::now();
                /*
                if (time_last_spin + std::chrono::milliseconds(300) < now) {
                        time_last_spin = now;
                        static const char spin[] = {"|/-\\"};
                        printf("\b%c", spin[(spinIndex++)%4]);
                        fflush(stdout);
                }
                */

                if (time_status_info + std::chrono::seconds(1) < now) {
                        time_status_info = now;
/*
                        std::string state_info = "In state: ";
                        state_info += state_to_string(current_state);
                        std::cout << std::endl << state_info << std::endl;
                        std::cout << "Num of found PINGS: "
                                  << num_of_found_pings
                                  << " Num of tries "
                                  << num_ping_tries
                                  << std::endl;
*/
                }

        }
        sdr.close();
        std::cout << "Number of found PINGS: "
                  << num_of_found_pings
                  << " Number of missed PINGS: "
                  << tot_num_of_missed_pings
                  << std::endl;
        if (plot_data) {
                Analysis analysis;
                analysis.add_data(buff_data_initial);
                analysis.plot_imag_data();
                //analysis.save_data("initial_buff_20ms");
                analysis.add_data(buff_data_ping);
                //analysis.plot_imag_data();
                //analysis.save_data("ping_buff_10ms");
                std::vector<float> corr;
                corr = detector.get_corr_result();
                analysis.add_data(corr);
                //analysis.plot_data();
        }
}

bool return_ok(int ret)
{
        bool data_ok(true);
        if (ret == SOAPY_SDR_TIMEOUT) {
                std::cout << "Timeout!" << std::endl;
        }
        if (ret == SOAPY_SDR_OVERFLOW) {
                std::cout << "Overflow!" << std::endl;
        }
        if (ret == SOAPY_SDR_UNDERFLOW) {
                std::cout << "Underflow!" << std::endl;
        }
        if (ret < 0) {
                std::string err = "Unexpected stream error ";
                err += SoapySDR::errToStr(ret);
                throw std::runtime_error(err);
        }
        return data_ok;
}

std::string state_to_string(TagStateMachine state)
{
        switch(state) {
        case INITIAL_SYNC:
                return "INITIAL_SYNC";
        case WAIT_FOR_PING:
                return "WAITING_FOR_PING";
        case SEARCH_FOR_PING:
                return "SEARCHING_FOR_PING";
        }
        return "UNKNOWN STATE";
}
