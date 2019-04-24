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

static sig_atomic_t g_stop = false;
// TODO: make this one atomic
long long int g_burst_hw_ns = -1;
void sigIntHandler(const int)
{
        g_stop = true;
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
                TCLAP::SwitchArg plot_switch("p","plot",
                                             "Plot data",
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
                bool plot_data = plot_switch.getValue();
                bool list_dev_info = list_switch.getValue();
                uint32_t device = device_arg.getValue();
                if (list_dev_info) {
                        list_device_info();
                }
                if (start_beacon) {
                        run_beacon(plot_data, device);
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

void run_beacon(bool plot_data, uint32_t device)
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
        int64_t now_hw_ticks = sdr.start();
        int64_t tx_start_hw_ticks = calculate_tx_start_tick(now_hw_ticks);

        std::future<void> future;
        future = std::async(std::launch::async, &transmit_ping,
                            sdr,
                            tx_start_hw_ticks);
        my_futures.push_back(std::move(future));

        // TODO: Can we get rid of this?
        // A dummy read to get timestamps up to sync
        std::vector<std::complex<int16_t>> buff_data_dummy(100);
        sdr.read(100, buff_data_dummy);

        Detector detector;
        detector.configure(CDMA, {dev_cfg.pong_scr_code}, dev_cfg);

        TimePoint time_last_spin = std::chrono::high_resolution_clock::now();
        int spin_index(0);
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not g_stop) {
                int64_t pong_time_hw_ns;
                pong_time_hw_ns = look_for_pong(sdr, detector);
                if (pong_time_hw_ns != -1) {
                        g_stop = true;
                }
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

        if (plot_data) {
                Analyser analyser;
                std::vector<float> corr;
                corr = detector.get_corr_result();
                analyser.add_data(corr);
                analyser.plot_data();
                arma::cx_vec raw_data = detector.get_raw_data();
                analyser.add_data(raw_data);
                analyser.save_data("raw_beacon_data");
        }
}

void calculate_tof(int64_t tx_start_time_hw_ns, int64_t last_pong_time_hw_ns)
{
        /* TODO: this is just a place holder for now. Should probably be
         * something like mod((last-tx), burst_period)/2*c
         */
        int64_t tof = last_pong_time_hw_ns - tx_start_time_hw_ns;
        if (tof > 0) {}
}

int64_t look_for_pong(SDR sdr, Detector &detector)
{
        SDR_Device_Config dev_cfg;
        const size_t no_of_samples_pong =
                dev_cfg.no_of_rx_samples_pong;
        size_t num_pong_tries(0);
        size_t num_of_found_pongs(0);
        size_t num_of_missed_pongs(0);
        size_t tot_num_of_missed_pongs(0);
        int64_t sync_ix(-1);
        int64_t sync_hw_ns(-1);
        std::vector<std::complex<int16_t>> buff_data_pong(
                no_of_samples_pong);
        long long int last_burst_hw_ns = g_burst_hw_ns;
        int ret = sdr.read(no_of_samples_pong, buff_data_pong);
        if (return_ok(ret, no_of_samples_pong)) {
                num_pong_tries++;
                int64_t expected_pong_ix;
                int64_t exp_pong_hw_ns =
                        last_burst_hw_ns + dev_cfg.pong_delay * 1e9;
                expected_pong_ix = sdr.find_exp_pong_pos_ix(exp_pong_hw_ns);
                detector.add_data(buff_data_pong);
                sync_ix = detector.look_for_pong(
                        expected_pong_ix);
                if (detector.found_pong(sync_ix)) {
                        sync_hw_ns = sdr.ix_to_hw_ns(sync_ix);
                        num_of_found_pongs++;
                        std::cout << "*** Found PONG"
                                  << " expected "
                                  << expected_pong_ix
                                  << " sync ix "
                                  << sync_ix
                                  << " diff "
                                  << expected_pong_ix-sync_ix
                                  << " data_length "
                                  << buff_data_pong.size()
                                  << " expected pong time "
                                  << exp_pong_hw_ns
                                  << " last burst time "
                                  << last_burst_hw_ns
                                  << std::endl;
                } else {
                        num_of_missed_pongs++;
                        tot_num_of_missed_pongs++;
                }
        }
        return sync_hw_ns;
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

void transmit_ping(SDR sdr, int64_t tx_start_hw_ticks)
{
        SDR_Device_Config dev_cfg;
        size_t buffer_size_tx = dev_cfg.tx_burst_length;
        size_t no_of_tx_samples = buffer_size_tx;
        int64_t tx_hw_ticks = tx_start_hw_ticks;
        int64_t burst_period_rel_ticks = ticks_per_period(
                dev_cfg.burst_period);
        double scale_factor(1.0);
        uint16_t Novs = dev_cfg.Novs_tx;
        double extra_samples_for_filter = dev_cfg.extra_samples_filter;
        size_t mod_length = dev_cfg.tx_burst_length_chip;
        mod_length = mod_length * (1 + extra_samples_for_filter);
        Modulator modulator(mod_length, scale_factor, Novs);
        modulator.generate_cdma(dev_cfg.ping_scr_code);
        modulator.filter();
        modulator.scrap_samples(mod_length * extra_samples_for_filter);
        std::vector<std::complex<float>> tx_buff_data = modulator.get_data();
        std::vector<void *> tx_buffs_data;
        tx_buffs_data.push_back(tx_buff_data.data());
        std::cout << "sample count per send call: "
                  << no_of_tx_samples << std::endl;
        while (not g_stop) {
                g_burst_hw_ns = SoapySDR::ticksToTimeNs(
                        tx_hw_ticks,
                        dev_cfg.f_clk);
                sdr.check_burst_time(g_burst_hw_ns);
                sdr.write(tx_buffs_data, no_of_tx_samples, g_burst_hw_ns);
                tx_hw_ticks += burst_period_rel_ticks;
        }
}

int64_t calculate_tx_start_tick(int64_t now_hw_ticks)
{
        SDR_Device_Config dev_cfg;
        int64_t tx_future_rel_ns =
                (dev_cfg.time_in_future + 2 * dev_cfg.burst_period);
        tx_future_rel_ns *= 1e9;
        int64_t tx_start_hw_ticks;
        tx_start_hw_ticks =
                now_hw_ticks + SoapySDR::timeNsToTicks(tx_future_rel_ns,
                                                       dev_cfg.f_clk);
        return tx_start_hw_ticks;
}

int64_t ticks_per_period(double period)
{
        SDR_Device_Config dev_cfg;
        double sampling_rate = dev_cfg.sampling_rate_tx;
        int64_t rel_ticks = (int64_t)(dev_cfg.D_tx * period * sampling_rate);
        return rel_ticks;
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
