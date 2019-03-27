/**
 * \file tag_main.cpp
 *
 * \brief Tag functionality
 *
 * The main functionality for the tag.
 * Based on
 * https://github.com/pothosware/SoapySDR/blob/master/apps/SoapyRateTest.cpp
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "tag_main.h"

SoapySDR::Device *device(nullptr);

static sig_atomic_t loopDone = false;
void sigIntHandler(const int)
{
    loopDone = true;
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
        const std::string &channelStr = "0";

        //const double frequency = 500e6;  //center frequency to 500 MHz
        const double sample_rate = 32e6;
        //const double rx_gain(20);
        //const double clock_rate(-1);
        //const double rx_bw(-1);

        int direction = SOAPY_SDR_RX;

        device = SoapySDR::Device::make();
        //build channels list, using KwargsFromString is a easy parsing hack
        std::vector<size_t> channels;
        for (const auto &pair : SoapySDR::KwargsFromString(channelStr)) {
            channels.push_back(std::stoi(pair.first));
        }
        if (channels.empty()) channels.push_back(0);
        //initialize the sample rate for all channels
        for (const auto &chan : channels) {
            device->setSampleRate(direction, chan, sample_rate);
        }

        //create the stream, use the native format
        double fullScale(0.0);
        const auto format = device->getNativeStreamFormat(direction, channels.front(), fullScale);
        const size_t elemSize = SoapySDR::formatToSize(format);
        auto stream = device->setupStream(direction, format, channels);

        std::cout << "Stream format: " << format << std::endl;
        std::cout << "Num channels: " << channels.size() << std::endl;
        std::cout << "Element size: " << elemSize << " bytes" << std::endl;
        std::cout << "Begin "  << (sample_rate/1e6) << " Msps" << std::endl;

        //allocate buffers for the stream read/write
        const size_t numChans = channels.size();
        const size_t numElems = device->getStreamMTU(stream);
        std::vector<std::vector<char>> buffMem(numChans, std::vector<char>(elemSize*numElems));
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

        std::cout << "Starting stream loop, press Ctrl+C to exit..." << std::endl;
        device->activateStream(stream);
        signal(SIGINT, sigIntHandler);

        size_t numElems2 = 5000;

        while (not loopDone) {
                int ret(0);
                int flags(0);
                long long timeNs(0);
                ret = device->readStream(stream, buffs.data(), numElems2, flags, timeNs);
                if (ret == SOAPY_SDR_TIMEOUT) continue;
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
                std::cout << ret << std::endl;
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

        if (plot_data) {
                arma::vec y_re(numElems2);
                //arma::vec y_im(numElems2);
                for (size_t j = 0; j < numElems2; ++j) {
                        y_re(j) = buffMem[0][j];
                        //y_im(j) = buffMem[1][2 * j + 1];
                }
                plot(y_re);
        }





}

/**
 * \fn plot
 * \brief Use gnuplot-cpp to plot data
 *
 * See the example.cc file that comes with the package for
 * examples on how to use the library.
 *
 */
void plot(std::vector<double> y, std::string title)
{
        Gnuplot g1("lines");
        g1.set_title(title);
        g1.plot_x(y);
        //usleep(1000000);
        wait_for_key();
}

void plot(std::vector<double> y)
{
        plot(y, "");
}

void plot(arma::vec y, std::string title)
{
        std::vector<double> y_p = arma::conv_to<std::vector<double>>::from(y);
        plot(y_p, title);
}

void plot(arma::vec y)
{
        std::vector<double> y_p = arma::conv_to<std::vector<double>>::from(y);
        plot(y_p, "");
}

void wait_for_key()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
        std::cout << std::endl << "Press any key to continue..." << std::endl;

        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        _getch();
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
        std::cout << std::endl << "Press ENTER to continue..." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::cin.rdbuf()->in_avail());
        std::cin.get();
#endif
        return;
}
