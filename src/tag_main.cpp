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
                cmd.parse(argc, argv);
                bool start_tag = start_switch.getValue();
                bool plot_data = plot_switch.getValue();
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

void run_tag(bool plot_data)
{
        SDR_Device_Config dev_cfg;
        dev_cfg.sampling_rate = 50e3;

        SDR sdr;
        SoapySDR::setLogLevel(dev_cfg.log_level);
        sdr.connect();
        sdr.configure(dev_cfg);

        sdr.start();

        const size_t numElems = dev_cfg.no_of_rx_samples;
        std::cout << "Num elems: " << numElems << std::endl;
        std::vector<std::vector<std::complex<int16_t>>> buffMem(
                numChans,
                std::vector<std::complex<int16_t >>(elemSize*numElems));
        std::vector<void *> buffs(numChans);
        for (size_t i = 0; i < numChans; i++) buffs[i] = buffMem[i].data();

        //state collected in this loop
        unsigned int overflows(0);
        unsigned int underflows(0);
        unsigned long long totalSamples(0);
        const auto startTime = std::chrono::high_resolution_clock::now();
        auto timeLastPrint = std::chrono::high_resolution_clock::now();
        auto timeLastSpin = std::chrono::high_resolution_clock::now();
        auto timeLastStatus = std::chrono::high_resolution_clock::now();
        int spinIndex(0);

        size_t numElems2 = numElems;

        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not stop) {
                int ret(0);
                int flags(0);
                long long timeNs(0);
                ret = device->readStream(stream, buffs.data(), numElems2,
                                         flags, timeNs);
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
                        std::cerr << "Unexpected stream error " << SoapySDR::errToStr(ret) << std::endl;
                        break;
                }
                totalSamples += ret;
                const auto now = std::chrono::high_resolution_clock::now();
                if (timeLastSpin + std::chrono::milliseconds(300) < now) {
                        timeLastSpin = now;
                        static const char spin[] = {"|/-\\"};
                        printf("\b%c", spin[(spinIndex++)%4]);
                        fflush(stdout);
                }
                //occasionally read out the stream status (non blocking)
                if (timeLastStatus + std::chrono::seconds(1) < now) {
                        timeLastStatus = now;
                        while (true) {
                                size_t chanMask; int flags; long long timeNs;
                                ret = device->readStreamStatus(stream, chanMask, flags, timeNs, 0);
                                if (ret == SOAPY_SDR_OVERFLOW) overflows++;
                                else if (ret == SOAPY_SDR_UNDERFLOW) underflows++;
                                else if (ret == SOAPY_SDR_TIME_ERROR) {}
                                else break;
                        }
                }
                if (timeLastPrint + std::chrono::seconds(5) < now) {
                        timeLastPrint = now;
                        const auto timePassed = std::chrono::duration_cast<std::chrono::microseconds>(now - startTime);
                        const auto sampleRate = double(totalSamples)/timePassed.count();
                        printf("\b%g Msps\t%g MBps", sampleRate, sampleRate*numChans*elemSize);
                        if (overflows != 0) printf("\tOverflows %u", overflows);
                        if (underflows != 0) printf("\tUnderflows %u", underflows);
                        printf("\n ");
                }
        }
        device->deactivateStream(stream);
        //cleanup stream and device
        device->closeStream(stream);
        SoapySDR::Device::unmake(device);

        if (plot_data) {}

        /*if (plot_data) {
                arma::vec y_re(numElems2);
                //arma::vec y_im(numElems2);
                for (size_t j = 0; j < numElems2; ++j) {
                        y_re(j) = (double)std::imag(buffMem[0][j]);
                        //y_re(j) = buffer[2 * j];
                        //y_im(j) = buffMem[1][2 * j + 1];
                }
                plot(y_re);*/
}
