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

std::vector<std::future<void>> my_futures;

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
                TCLAP::SwitchArg list_switch("l","list-devices",
                                             "List info on attached devices",
                                             cmd, false);
                cmd.parse(argc, argv);
                bool start_beacon = start_switch.getValue();
                bool list_dev_info = list_switch.getValue();
                if (list_dev_info) {
                        list_device_info();
                }
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

void list_device_info()
{
        SDR sdr;
        sdr.connect();
        sdr.list_hw_info();
}

void run_beacon()
{
        SDR_Device_Config dev_cfg;
        //std::string dev_serial = dev_cfg.serial_bladerf_x40;
        std::string dev_serial = dev_cfg.serial_bladerf_xA4;
        //std::string dev_serial = dev_cfg.serial_lime_3;

        SDR sdr;
        SoapySDR::setLogLevel(dev_cfg.log_level);
        sdr.connect(dev_serial);
        sdr.configure(dev_cfg);
        int64_t now_tick = sdr.start();
        int64_t tx_start_tick = calculate_tx_start_tick(now_tick);

        std::future<void> future;
        future = std::async(std::launch::async, &transmit_ping,
                            sdr,
                            tx_start_tick);
        my_futures.push_back(std::move(future));

        TimePoint time_last_spin = std::chrono::high_resolution_clock::now();
        int spin_index(0);
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not stop) {
                look_for_pong(tx_start_tick);
                time_last_spin = print_spin(time_last_spin, spin_index++);
                usleep(100);
        }

        size_t m = my_futures.size();
        for(size_t n = 0; n < m; n++) {
                auto e = std::move(my_futures.back());
                e.get();
                my_futures.pop_back();
        }

        sdr.close();
}

void look_for_pong(int64_t tx_start_tick)
{
        SDR_Device_Config dev_cfg;
        if (tx_start_tick) {}

}

void transmit_ping(SDR sdr, int64_t tx_start_tick)
{
        SDR_Device_Config dev_cfg;
        size_t buffer_size_tx = dev_cfg.tx_burst_length;
        size_t no_of_tx_samples = buffer_size_tx;

        int64_t tx_tick = tx_start_tick;
        double burst_period = dev_cfg.burst_period;
        int64_t ticks_per_burst_period = ticks_per_period(burst_period);

        double scale_factor(1.0);
        uint16_t Novs = dev_cfg.Novs_tx;
        double extra_samples_for_filter = dev_cfg.extra_samples_filter;
        size_t mod_length = dev_cfg.tx_burst_length_chip;
        mod_length = mod_length * (1 + extra_samples_for_filter);
        Modulation modulation(mod_length, scale_factor, Novs);
        modulation.generate_cdma(dev_cfg.ping_scr_code);
        modulation.filter();
        modulation.scrap_samples(mod_length * extra_samples_for_filter);
        std::vector<std::complex<float>> tx_buff_data = modulation.get_data();
        std::vector<void *> tx_buffs_data;
        tx_buffs_data.push_back(tx_buff_data.data());
        std::cout << "sample count per send call: "
                  << no_of_tx_samples << std::endl;
        while (not stop) {
                long long int burst_time = SoapySDR::ticksToTimeNs(
                        tx_tick,
                        dev_cfg.f_clk);
                sdr.check_burst_time(burst_time);
                sdr.write(tx_buffs_data, no_of_tx_samples, burst_time);
                tx_tick += ticks_per_burst_period;
        }
}

int64_t calculate_tx_start_tick(int64_t now_tick)
{
        SDR_Device_Config dev_cfg;
        int64_t tx_future = dev_cfg.time_in_future;
        tx_future += 2 * dev_cfg.burst_period;
        int64_t tx_start_tick;
        tx_start_tick = now_tick + SoapySDR::timeNsToTicks(
                (tx_future) * 1e9,
                dev_cfg.f_clk);
        return tx_start_tick;
}

int64_t ticks_per_period(double period)
{
        SDR_Device_Config dev_cfg;
        double sampling_rate = dev_cfg.sampling_rate_tx;
        int64_t ticks = (int64_t)(dev_cfg.D_tx * period * sampling_rate);
        return ticks;
}

TimePoint print_spin(TimePoint time_last_spin, int spin_index)
{
        TimePoint now = std::chrono::high_resolution_clock::now();
        if (time_last_spin + std::chrono::milliseconds(300) < now) {
                static const char spin[] = {"|/-\\"};
                printf("\b%c", spin[(spin_index++)%4]);
                fflush(stdout);
                time_last_spin = now;
        }
        return time_last_spin;
}
