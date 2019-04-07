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
        //std::string dev_serial = dev_cfg.serial_bladerf_v2;
        std::string dev_serial = dev_cfg.serial_lime_3;
        dev_cfg.tx_active = false;
        const size_t no_of_samples = dev_cfg.no_of_rx_samples;
        dev_cfg.tx_active = false;

        std::cout << "No of samples to read: "
                  << no_of_samples << std::endl;

        SDR sdr;
        SoapySDR::setLogLevel(dev_cfg.log_level);
        sdr.connect(dev_serial);
        sdr.configure(dev_cfg);
        sdr.start();

        auto timeLastSpin = std::chrono::high_resolution_clock::now();
        int spinIndex(0);

        std::vector<std::complex<int16_t>> buff_data(no_of_samples);
        Detector detector;
        detector.configure(CDMA, {dev_cfg.ping_scr_code}, dev_cfg);

        bool found_sync(false);
        int64_t sync_ix(-1);
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not stop && not found_sync) {
                int ret = sdr.read(no_of_samples, buff_data);
                if (return_ok(ret)) {
                        detector.add_data(buff_data);
                        sync_ix = detector.look_for_initial_sync();
                        if (detector.found_initial_sync(sync_ix)) {
                                found_sync = true;
                        }
                }
                const auto now = std::chrono::high_resolution_clock::now();
                if (timeLastSpin + std::chrono::milliseconds(300) < now) {
                        timeLastSpin = now;
                        static const char spin[] = {"|/-\\"};
                        printf("\b%c", spin[(spinIndex++)%4]);
                        fflush(stdout);
                }
        }
        sdr.close();
        std::cout << "Found initial sync peak at: "
                  << sync_ix
                  << std::endl;
        if (plot_data) {
                Analysis analysis;
                analysis.add_data(buff_data);
                analysis.plot_imag_data();
                //analysis.save_data("cdma");
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
