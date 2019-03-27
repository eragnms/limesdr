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

SoapySDR::Device *device;

static sig_atomic_t loopDone = false;
void sigIntHandler(const int)
{
        loopDone = true;
}

int main(int argc, char** argv)
{
        try {
                bool enable_version_and_help(true);
                TCLAP::CmdLine cmd("Beacon main application",
                                   ' ',
                                   PACKAGE_STRING,
                                   enable_version_and_help);
                TCLAP::SwitchArg start_switch("s","start",
                                              "Start beacon",
                                              cmd, false);
                cmd.parse(argc, argv);
                bool start_beacon = start_switch.getValue();
                if (start_beacon) {
                        run_beacon();
                }
        }
        catch (TCLAP::ArgException &e) {
                std::cerr << "error: " << e.error()
                          << " for arg " << e.argId() << std::endl;
        }
        return EXIT_SUCCESS;
}

void run_beacon()
{
        const double frequency = 500e6;  //center frequency to 500 MHz
        const double sample_rate = 5e6;    //sample rate to 5 MHz
        const double tone_freq = 2e4; //tone frequency
        const double f_ratio = tone_freq/sample_rate;
        const double tx_gain(40);
        const double clock_rate(-1);
        const double tx_bw(-1);

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
        const size_t tx_ch(0);
        device->setSampleRate(SOAPY_SDR_TX, tx_ch, sample_rate);
        double act_sample_rate = device->getSampleRate(SOAPY_SDR_TX, tx_ch);
        std::cout << "Actual TX rate: " << act_sample_rate << " Msps" << std::endl;
        device->setAntenna(SOAPY_SDR_TX, tx_ch, "BAND1");
        device->setGain(SOAPY_SDR_TX, tx_ch, tx_gain);
        device->setFrequency(SOAPY_SDR_TX, tx_ch, frequency);
        if (tx_bw != -1) {
                device->setBandwidth(SOAPY_SDR_TX, tx_ch, tx_bw);
        }
        std::vector<size_t> tx_channel;
        SoapySDR::Stream *tx_stream;
        tx_stream = device->setupStream(SOAPY_SDR_TX,
                                        SOAPY_SDR_CF32,
                                        tx_channel);
        uint32_t microseconds(1e+6);
        usleep(microseconds);

        const int buffer_size = 256*8;
        std::vector<float> tx_buff_data(2*buffer_size);
        for (int i = 0; i <buffer_size; i++) {
                const double pi = acos(-1);
                double w = 2*pi*i*f_ratio;
                tx_buff_data[2*i] = cos(w);
                tx_buff_data[2*i+1] = sin(w);
        }
        std::vector<void *> tx_buffs_data;
        tx_buffs_data.push_back(tx_buff_data.data());

        device->activateStream(tx_stream);
        const int send_cnt = int(buffer_size*f_ratio) / f_ratio;
        std::cout << "sample count per send call: " << send_cnt << std::endl;

        // Start rx to get timestmap running
        std::vector<size_t> rx_channel;
        SoapySDR::Stream *rx_stream;
        rx_stream = device->setupStream(SOAPY_SDR_RX,
                                        SOAPY_SDR_CF32,
                                        rx_channel);
        device->activateStream(rx_stream);

        long long tx_time_0 = device->getHardwareTime() + 0.1e9;

        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = t1;
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not loopDone) {
                //int tx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST | SOAPY_SDR_ONE_PACKET;
                int tx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST;
                //tx_flags = 0;
                uint32_t status;
                status = device->writeStream(tx_stream,
                                             tx_buffs_data.data(),
                                             send_cnt,
                                             tx_flags,
                                             tx_time_0,
                                             1e6*4);
                tx_time_0 += 0.1e9;
                usleep(100000);
                std::cout << "**********" << std::endl;
                std::cout << tx_time_0 << std::endl;
                std::cout << device->getHardwareTime() << std::endl;
                if (status != send_cnt) {
                        std::string tx_verbose_msg = "Transmit failed: ";
                        switch (status) {
                        case SOAPY_SDR_TIMEOUT:
                                tx_verbose_msg += "SOAPY_SDR_TIMEOUT";
                                break;
                        case SOAPY_SDR_STREAM_ERROR:
                                tx_verbose_msg += "SOAPY_SDR_STREAM_ERROR";
                                break;
                        case SOAPY_SDR_CORRUPTION:
                                tx_verbose_msg += "SOAPY_SDR_CORRUPTION";
                                break;
                        case SOAPY_SDR_OVERFLOW:
                                tx_verbose_msg += "SOAPY_SDR_OVERFLOW";
                                break;
                        case SOAPY_SDR_NOT_SUPPORTED:
                                tx_verbose_msg += "SOAPY_SDR_NOT_SUPPORTED";
                                break;
                        case SOAPY_SDR_END_BURST:
                                tx_verbose_msg += "SOAPY_SDR_END_BURST";
                                break;
                        case SOAPY_SDR_TIME_ERROR:
                                tx_verbose_msg += "SOAPY_SDR_TIME_ERROR";
                                break;
                        case SOAPY_SDR_UNDERFLOW:
                                tx_verbose_msg += "SOAPY_SDR_UNDERFLOW";
                                break;
                        }
                        throw std::runtime_error(tx_verbose_msg);
                }
                if (std::chrono::high_resolution_clock::now() - t2>std::chrono::seconds(1)) {
                        t2 = std::chrono::high_resolution_clock::now();
                        size_t chan_mask = 0;
                        int stream_status = device->readStreamStatus(
                                tx_stream,
                                chan_mask,
                                tx_flags,
                                tx_time_0,
                                1e6*2);
                        std::string verbose_msg = "Stream status: ";
                        switch(stream_status) {
                        case SOAPY_SDR_TIMEOUT:
                                verbose_msg += "SOAPY_SDR_TIMEOUT";
                                break;
                        case SOAPY_SDR_STREAM_ERROR:
                                verbose_msg += "SOAPY_SDR_STREAM_ERROR";
                                break;
                        case SOAPY_SDR_CORRUPTION:
                                verbose_msg += "SOAPY_SDR_CORRUPTION";
                                break;
                        case SOAPY_SDR_OVERFLOW:
                                verbose_msg += "SOAPY_SDR_OVERFLOW";
                                break;
                        case SOAPY_SDR_NOT_SUPPORTED:
                                verbose_msg += "SOAPY_SDR_NOT_SUPPORTED";
                                break;
                        case SOAPY_SDR_END_BURST:
                                verbose_msg += "SOAPY_SDR_END_BURST";
                                break;
                        case SOAPY_SDR_TIME_ERROR:
                                verbose_msg += "SOAPY_SDR_TIME_ERROR";
                                break;
                        case SOAPY_SDR_UNDERFLOW:
                                verbose_msg += "SOAPY_SDR_UNDERFLOW";
                                break;
                        default:
                                verbose_msg += "NO_ERROR";
                                break;
                        }

                        std::cout << verbose_msg << std::endl;

                }
        }
        device->deactivateStream(tx_stream);
        device->closeStream(tx_stream);
        device->deactivateStream(rx_stream);
        device->closeStream(rx_stream);
        SoapySDR::Device::unmake(device);





}
