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
        //std::string dev_serial = dev_cfg.serial_bladerf_x40;
        std::string dev_serial = dev_cfg.serial_bladerf_xA4;
        //std::string dev_serial = dev_cfg.serial_lime_3;
        //dev_cfg.tx_active = false;
        dev_cfg.is_beacon = false;
        const size_t no_of_samples_initial_sync =
                dev_cfg.no_of_rx_samples_initial_sync;
        const size_t no_of_samples_ping =
                dev_cfg.no_of_rx_samples_ping;

        std::cout << "No of samples to read in initial sync: "
                  << no_of_samples_initial_sync << std::endl;

        SDR sdr;
        SoapySDR::setLogLevel(dev_cfg.log_level);
        sdr.connect(dev_serial);
        sdr.configure(dev_cfg);
        sdr.start();

        std::vector<std::complex<int16_t>> buff_data_initial(
                no_of_samples_initial_sync);
        std::vector<std::complex<int16_t>> buff_data_ping(
                no_of_samples_ping);
        Detector detector;
        detector.configure(CDMA, {dev_cfg.ping_scr_code}, dev_cfg);

        size_t buffer_size_tx = dev_cfg.tx_burst_length;
        size_t no_of_tx_samples = buffer_size_tx;
        double scale_factor(1.0);
        uint16_t Novs = dev_cfg.Novs_tx;
        double extra_samples_for_filter = dev_cfg.extra_samples_filter;
        size_t mod_length = dev_cfg.tx_burst_length_chip;
        mod_length = mod_length * (1 + extra_samples_for_filter);
        Modulation modulation(mod_length, scale_factor, Novs);
        modulation.generate_cdma(dev_cfg.pong_scr_code);
        modulation.filter();
        modulation.scrap_samples(mod_length * extra_samples_for_filter);
        std::vector<std::complex<float>> tx_buff_data = modulation.get_data();
        std::vector<void *> tx_buffs_data;
        tx_buffs_data.push_back(tx_buff_data.data());
        std::cout << "sample count per send call: "
                  << no_of_tx_samples << std::endl;

        size_t num_of_found_pings(0);
        size_t num_of_missed_pings(0);
        size_t tot_num_of_missed_pings(0);
        size_t num_ping_tries(0);
        int64_t sync_ix(-1);
        int64_t hw_time_of_sync(0);
        TagStateMachine current_state(INITIAL_SYNC);
        std::cout << "**********************" << std::endl;
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        std::cout << "Looking for inital sync" << std::endl;
        signal(SIGINT, sigIntHandler);
        size_t num_syncs(0);
        //int64_t old_hw_time(0);
        //size_t num_packets(0);
        while (not stop) {
                switch(current_state) {
                case INITIAL_SYNC: {
                        int ret = sdr.read(no_of_samples_initial_sync,
                                           buff_data_initial);
                        if (return_ok(ret)) {
                                //num_packets++;
                                detector.add_data(buff_data_initial);
                                sync_ix = detector.look_for_initial_sync();
                                /*if (num_packets > 10) {
                                        stop = true;
                                        }*/
                                if (detector.found_initial_sync(sync_ix)) {
                                        num_syncs++;
                                        hw_time_of_sync = sdr.ix_to_hw_time(
                                                sync_ix);
                                        std::cout << "Found inital sync"
                                                  << " at "
                                                  << hw_time_of_sync
                                                  << std::endl;
                                        current_state = SEARCH_FOR_PING;
                                        //stop = true;
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
                                        current_state = SEND_PONG;
                                } else {
                                        num_of_missed_pings++;
                                        tot_num_of_missed_pings++;
                                }
                                if (num_of_missed_pings > dev_cfg.num_of_ping_tries) {
                                        num_of_missed_pings = 0;
                                        std::cout << "Faild PING detect"
                                                  << std::endl;
                                        std::cout << "Starting initial sync"
                                                  << std::endl;
                                        current_state = INITIAL_SYNC;
                                }
                                if (num_of_found_pings == 10) {
                                        //stop = true;
                                }
                        }
                        break;
                }
                case SEND_PONG: {
                        std::cout << "Sending PONG" << std::endl;
                        const double fs_tx = dev_cfg.sampling_rate_tx;
                        double pong_delay = dev_cfg.pong_delay + dev_cfg.pong_delay_processing;
                        int64_t tmp = dev_cfg.D_tx * pong_delay * fs_tx;
                        int64_t ticks_before_pong = tmp;
                        int64_t tx_tick = SoapySDR::timeNsToTicks(
                                hw_time_of_sync, dev_cfg.f_clk) + ticks_before_pong;
                        long long int burst_time = SoapySDR::ticksToTimeNs(
                                tx_tick,
                                dev_cfg.f_clk);
                        sdr.check_burst_time(burst_time);
                        sdr.write(tx_buffs_data, no_of_tx_samples, burst_time);
                        current_state = SEARCH_FOR_PING;
                        break;
                }
                default:
                        throw std::runtime_error("Unknown state tag!");
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
                //analysis.plot_imag_data();
                //analysis.save_data("initial_buff_20ms");
                analysis.add_data(buff_data_ping);
                //analysis.plot_imag_data();
                //analysis.save_data("ping_buff_10ms");
                std::vector<float> corr;
                corr = detector.get_corr_result();
                analysis.add_data(corr);
                analysis.plot_data();
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
        case SEARCH_FOR_PING:
                return "SEARCHING_FOR_PING";
        case SEND_PONG:
                return "SEND_PONG";
        }
        return "UNKNOWN STATE";
}
