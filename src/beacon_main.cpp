/**
 * \file beacon_main.cpp
 *
 * \brief Beacon functionality
 *
 * The main functionality for the beacon.
 * Run from the commandline with -h for a list of options.
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "beacon_main.h"

static sig_atomic_t stop = false;
void sigIntHandler(const int)
{
        stop = true;
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
        SDR_Device_Config dev_cfg;
        std::string dev_serial = dev_cfg.serial_bladerf_v2;
        const double sampling_rate = dev_cfg.sampling_rate_tx;
        double burst_period = dev_cfg.burst_period;
        size_t buffer_size_tx = dev_cfg.tx_burst_length;
        size_t no_of_tx_samples = buffer_size_tx;

        double scale_factor(1.0);
        uint16_t Novs = dev_cfg.Novs;
        double extra_samples_for_filter = dev_cfg.extra_samples_filter;
        size_t mod_length = dev_cfg.tx_burst_length_chip;
        mod_length = mod_length * (1 + extra_samples_for_filter);
        Modulation modulation(mod_length, scale_factor, Novs);
        //const double tone_freq(16e3);
        //modulation.generate_sine(tone_freq, sampling_rate);
        modulation.generate_cdma(0);
        //modulation.filter();
        modulation.scrap_samples(mod_length * extra_samples_for_filter);
        std::vector<std::complex<float>> tx_buff_data = modulation.get_data();
        std::vector<void *> tx_buffs_data;
        tx_buffs_data.push_back(tx_buff_data.data());
        std::cout << "sample count per send call: "
                  << no_of_tx_samples << std::endl;

        //Analysis analyze;
        //analyze.add_data(tx_buff_data);
        //analyze.plot_real_data();

        SDR sdr;
        SoapySDR::setLogLevel(dev_cfg.log_level);
        sdr.connect(dev_serial);
        sdr.configure(dev_cfg);

        int64_t now_tick = sdr.start();
        int64_t tx_start_tick = now_tick;
        int64_t tx_future = dev_cfg.time_in_future;
        tx_future += 2 * dev_cfg.burst_period;
        tx_start_tick += SoapySDR::timeNsToTicks((tx_future) * 1e9,
                                                   dev_cfg.f_clk);
        int64_t tx_tick = tx_start_tick;
        int64_t tmp = dev_cfg.D_tx * burst_period * sampling_rate;
        int64_t no_of_ticks_per_bursts_period = tmp;

        auto timeLastSpin = std::chrono::high_resolution_clock::now();
        int spinIndex(0);
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not stop) {
                long long int burst_time = SoapySDR::ticksToTimeNs(
                        tx_tick,
                        dev_cfg.f_clk);
                sdr.check_burst_time(burst_time);
                sdr.write(tx_buffs_data, no_of_tx_samples, burst_time);
                tx_tick += no_of_ticks_per_bursts_period;

                const auto now = std::chrono::high_resolution_clock::now();
                if (timeLastSpin + std::chrono::milliseconds(300) < now) {
                        timeLastSpin = now;
                        static const char spin[] = {"|/-\\"};
                        printf("\b%c", spin[(spinIndex++)%4]);
                        fflush(stdout);
                }

        }
        sdr.close();
}
