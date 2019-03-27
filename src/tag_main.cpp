/**
 * \file tag_main.cpp
 *
 * \brief Tag functionality
 *
 * The main functionality for the tag.
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "tag_main.h"

SoapySDR::Device *device;

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
        const double frequency = 500e6;  //center frequency to 500 MHz
        const double sample_rate = 32e6;
        const double rx_gain(20);
        const double clock_rate(-1);
        const double rx_bw(-1);

        SoapySDR::KwargsList results = SoapySDR::Device::enumerate();
        if (results.size() > 0) {
                std::cout << "Found Device!" << std::endl;
        } else {
                throw std::runtime_error("Found no device!");
        }
        device = SoapySDR::Device::make();
        if (device == nullptr) {
                throw std::runtime_error("Could not open device!");
        }
        if (!device->hasHardwareTime()) {
                throw std::runtime_error("This device does not support timed streaming!");
        }
        if (clock_rate != -1) {
                device->setMasterClockRate(clock_rate);
        }
        const size_t rx_ch(0);
        device->setSampleRate(SOAPY_SDR_RX, rx_ch, sample_rate);
        double act_sample_rate = device->getSampleRate(SOAPY_SDR_RX, rx_ch);
        std::cout << "Actual RX rate: " << act_sample_rate << " Msps" << std::endl;
        device->setAntenna(SOAPY_SDR_RX, rx_ch, "LNAL");
        device->setGain(SOAPY_SDR_RX, rx_ch, rx_gain);
        device->setFrequency(SOAPY_SDR_RX, rx_ch, frequency);
        if (rx_bw != -1) {
                device->setBandwidth(SOAPY_SDR_RX, rx_ch, rx_bw);
        }
        std::vector<size_t> rx_channel;
        SoapySDR::Stream *rx_stream;
        rx_stream = device->setupStream(SOAPY_SDR_RX,
                                        SOAPY_SDR_CF32,
                                        rx_channel);
        uint32_t microseconds(1e+6);
        usleep(microseconds);

        //rx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
        int rx_flags = SOAPY_SDR_END_BURST;
        uint32_t receive_time(0);
        const int sampleCnt = 5000;
        device->activateStream(rx_stream, rx_flags, receive_time,
                               sampleCnt);
        //std::vector<std::complex<float>> buffer(sampleCnt);
        uint32_t numChans(1);
        const size_t numElems = device->getStreamMTU(rx_stream);
        const size_t elemSize(sampleCnt);
        std::vector<std::vector<char>> buffer(numChans, std::vector<char>(elemSize*numElems));
        std::vector<void *> rx_buffs(numChans);
        for (size_t i = 0; i < numChans; i++) rx_buffs[i] = buffer[i].data();
        int samplesRead(0);
        uint32_t timeout(1e3);
        long long int rx_timestamp(0);
        //auto t1 = std::chrono::high_resolution_clock::now();
        //while (std::chrono::high_resolution_clock::now() - t1 < std::chrono::seconds(2)) {

        for (size_t n=0; n<2; n++) {
                samplesRead = device->readStream(rx_stream,
                                                 rx_buffs.data(),
                                                 sampleCnt,
                                                 rx_flags,
                                                 rx_timestamp,
                                                 timeout);

                //I and Q samples are interleaved in buffer: IQIQIQ...
                printf("Received %d samples\n", samplesRead);
                /*
                  INSERT CODE FOR PROCESSING RECEIVED SAMPLES
                */
        }
        if (plot_data) {
        }
        /*if (plot_data) {
                arma::vec y_re(samplesRead);
                arma::vec y_im(samplesRead);
                for (int j = 0; j < samplesRead; ++j) {
                        y_re(j) = buffer[2 * j];
                        y_im(j) = buffer[2 * j + 1];
                }
                plot(y_re);
                }*/
        device->deactivateStream(rx_stream);
        device->closeStream(rx_stream);
        SoapySDR::Device::unmake(device);





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
