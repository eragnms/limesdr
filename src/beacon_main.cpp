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
// TODO: make this one atomic
long long int burst_time;
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
                TCLAP::ValueArg<uint32_t> device_arg("d", "device",
                                                     "1-BRF1, 2-BRF2, 3-L3",
                                                     false, 0,
                                                     "uint32_t");
                cmd.add(device_arg);
                cmd.parse(argc, argv);
                bool start_beacon = start_switch.getValue();
                bool list_dev_info = list_switch.getValue();
                uint32_t device = device_arg.getValue();
                if (list_dev_info) {
                        list_device_info();
                }
                if (start_beacon) {
                        run_beacon(device);
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

void run_beacon(uint32_t device)
{
        SDR_Device_Config dev_cfg;
        std::string dev_serial = "";
        switch(device) {
        case 1:
                dev_serial = dev_cfg.serial_bladerf_x40;
                break;
        case 2:
                dev_serial = dev_cfg.serial_bladerf_xA4;
                break;
        case 3:
                dev_serial = dev_cfg.serial_lime_3;
                break;
        default:
                dev_serial = dev_cfg.serial_bladerf_xA4;
        }
        if (dev_cfg.is_beacon) {
                dev_cfg.tx_frequency = dev_cfg.ping_frequency;
                dev_cfg.rx_frequency = dev_cfg.pong_frequency;
        } else {
                dev_cfg.tx_frequency = dev_cfg.pong_frequency;
                dev_cfg.rx_frequency = dev_cfg.ping_frequency;
        }

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

        long long int tx_start_time_hw_ns = SoapySDR::ticksToTimeNs(
                tx_start_tick,
                dev_cfg.f_clk);
        int64_t earliest_pong_tx_time_hw_ns =
                tx_start_time_hw_ns + dev_cfg.pong_delay * 1e9;
        int64_t last_pong_time_hw_ns = earliest_pong_tx_time_hw_ns;

        // TODO: Can we get rid of this?
        // A dummy read to get timestamps up to sync
        std::vector<std::complex<int16_t>> buff_data_dummy(100);
        sdr.read(100, buff_data_dummy);

        TimePoint time_last_spin = std::chrono::high_resolution_clock::now();
        int spin_index(0);
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not stop) {
                last_pong_time_hw_ns = look_for_pong(sdr,
                                                     last_pong_time_hw_ns);
                //calculate_tof(tx_start_time_hw_ns, last_pong_time_hw_ns);
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

void calculate_tof(int64_t tx_start_time_hw_ns, int64_t last_pong_time_hw_ns)
{
        /* TODO: this is just a place holder for now. Should probably be
         * something like mod((last-tx), burst_period)/2*c
         */
        int64_t tof = last_pong_time_hw_ns - tx_start_time_hw_ns;
        if (tof > 0) {}
}


int64_t look_for_pong(SDR sdr, int64_t last_pong_time_hw_ns)
{
        SDR_Device_Config dev_cfg;
        const size_t no_of_samples_pong =
                dev_cfg.no_of_rx_samples_pong;

        size_t num_pong_tries(0);
        size_t num_of_found_pongs(0);
        size_t num_of_missed_pongs(0);
        size_t tot_num_of_missed_pongs(0);
        int64_t sync_ix(-1);
        int64_t hw_time_of_sync(0);

        std::vector<std::complex<int16_t>> buff_data_pong(
                no_of_samples_pong);

        last_pong_time_hw_ns += 0;

        Detector detector;
        detector.configure(CDMA, {dev_cfg.pong_scr_code}, dev_cfg);
        int ret = sdr.read(no_of_samples_pong, buff_data_pong);
        if (return_ok(ret, no_of_samples_pong)) {
                std::cout << "Data read OK" << std::endl;
                num_pong_tries++;
                int64_t expected_pong_ix;
                MARK;
                expected_pong_ix = sdr.expected_pong_pos_ix(
                        burst_time + dev_cfg.pong_delay);
                MARK;
                detector.add_data(buff_data_pong);
                MARK;
                sync_ix = detector.look_for_pong(
                        expected_pong_ix);
                MARK;
                if (detector.found_pong(sync_ix)) {
                        hw_time_of_sync = sdr.ix_to_hw_time(
                                sync_ix);
                        num_of_found_pongs++;
                        std::cout << "Found PONG"
                                  << " expected "
                                  << expected_pong_ix
                                  << " sync ix "
                                  << sync_ix
                                  << " diff "
                                  << expected_pong_ix-sync_ix
                                  << " data_length "
                                  << buff_data_pong.size()
                                  << std::endl;
                } else {
                        num_of_missed_pongs++;
                        tot_num_of_missed_pongs++;
                }
        }  else {
                /*
                std::cout << "Failed read data "
                          << ret
                          << std::endl;
                */
        }
        return hw_time_of_sync;
}

bool return_ok(int ret, size_t expected_num_samples)
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
                std::cout << err << std::endl;
        }
        data_ok &= (ret == (int)expected_num_samples);
        return data_ok;
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
                burst_time = SoapySDR::ticksToTimeNs(
                        tx_tick,
                        dev_cfg.f_clk);
                //burst_time = sdr.check_burst_time(burst_time);
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
