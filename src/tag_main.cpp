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

        unsigned int overflows(0);
        unsigned int underflows(0);
        unsigned long long totalSamples(0);
        auto timeLastSpin = std::chrono::high_resolution_clock::now();
        int spinIndex(0);

        std::vector<std::complex<int16_t>> buff_data(no_of_samples);

        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not stop) {
                int ret(0);
                ret = sdr.read(no_of_samples, buff_data);
                if (ret == SOAPY_SDR_TIMEOUT) {
                        continue;
                }
                if (ret == SOAPY_SDR_OVERFLOW) {
                        overflows++;
                        continue;
                }
                if (ret == SOAPY_SDR_UNDERFLOW) {
                        underflows++;
                        continue;
                }
                if (ret < 0) {
                        std::string err = "Unexpected stream error ";
                        err += SoapySDR::errToStr(ret);
                        throw std::runtime_error(err);
                }
                totalSamples += ret;
                const auto now = std::chrono::high_resolution_clock::now();
                if (timeLastSpin + std::chrono::milliseconds(300) < now) {
                        timeLastSpin = now;
                        static const char spin[] = {"|/-\\"};
                        printf("\b%c", spin[(spinIndex++)%4]);
                        fflush(stdout);
                }
        }
        sdr.close();
        if (plot_data) {
                Analysis analysis;
                analysis.add_data(buff_data);
                analysis.plot_imag_data();
                analysis.save_data("cdma");
                Detector detector;
                detector.configure(CDMA, {2}, dev_cfg);
                detector.add_data(buff_data);
                detector.detect();
                std::vector<std::complex<float>> corr;
                corr = detector.get_corr_result();
                std::cout << "corr l " << corr.size() << std::endl;
                analysis.add_data(corr);
                analysis.plot_data();
        }
}
