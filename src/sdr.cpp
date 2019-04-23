/**
 * \file sdr.cpp
 *
 * \brief SDR class
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "sdr.h"

SDR::SDR()
{}

void SDR::connect()
{

        SoapySDR::KwargsList results = SoapySDR::Device::enumerate();
        if (results.size() > 0) {
                std::cout << "Found Device!" << std::endl;
        } else {
                throw std::runtime_error("Found no device!");
        }
        connect_to_device(results, 0);
}

void SDR::connect(std::string device_serial)
{
        SoapySDR::KwargsList results = SoapySDR::Device::enumerate();
        if (results.size() > 0) {
                std::cout << "Found Device!" << std::endl;
        } else {
                throw std::runtime_error("Found no device!");
        }
        int32_t device_num = look_up_device_serial(results, device_serial);
        if (device_num != -1) {
                connect_to_device(results, device_num);
        } else {
                std::string err = "Could not find device: ";
                err += device_serial;
                throw std::runtime_error(err);
        }
}

void SDR::connect_to_device(SoapySDR::KwargsList results, int32_t device_num)
{
        m_device = SoapySDR::Device::make(results[device_num]);
        if (m_device == nullptr) {
                throw std::runtime_error("Could not open device!");
        }
        if (!m_device->hasHardwareTime()) {
                std::string err = "This device does not support";
                err +=  " timed streaming!";
                throw std::runtime_error(err);
        }
        if (is_bladerf()) {
                check_lib_bladerf_support();
        }
}

SoapySDR::Device *SDR::get_device()
{
        return m_device;
}

void SDR::configure(SDR_Device_Config dev_cfg)
{
        m_dev_cfg = dev_cfg;
        if (m_dev_cfg.clock_source != "") {
                m_device->setClockSource(m_dev_cfg.clock_source);
        }
        if (m_dev_cfg.time_source != "") {
                m_device->setTimeSource(m_dev_cfg.time_source);
        }
        m_device->setMasterClockRate(m_dev_cfg.f_clk);
        if (m_dev_cfg.rx_active) {
                configure_rx();
        }
        if (m_dev_cfg.tx_active) {
                configure_tx();
        }
}

void SDR::configure_tx()
{
        m_device->setSampleRate(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                                m_dev_cfg.sampling_rate_tx);
        double act_sample_rate = m_device->getSampleRate(
                SOAPY_SDR_TX,
                m_dev_cfg.channel_tx);
        std::cout << "Actual TX rate: "
                  << act_sample_rate
                  << " Msps" << std::endl;
        if (m_dev_cfg.tx_bw != -1) {
                m_device->setBandwidth(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                                       m_dev_cfg.tx_bw);
        }
        m_device->setGain(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                          m_dev_cfg.tx_gain);
        double gain = m_device->getGain(SOAPY_SDR_TX, m_dev_cfg.channel_tx);
        std::cout << "TX gain " << gain << std::endl;
        m_device->setAntenna(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                             m_dev_cfg.antenna_tx);
        m_device->setFrequency(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                               m_dev_cfg.tx_frequency);
        if (is_limesdr()) {
                bool tx_lo_locked = false;
                while (not tx_lo_locked) {
                        std::string tx_locked = m_device->readSensor(
                                SOAPY_SDR_TX,
                                m_dev_cfg.channel_tx,
                                "lo_locked");
                        if (tx_locked == "true") {
                                tx_lo_locked = true;
                        }
                        usleep(100);
                }
                std::cout << "sdr: TX LO lock detected on channel "
                          << std::to_string(m_dev_cfg.channel_tx)
                          << std::endl;
        }
        std::cout << "sdr: Actual TX frequency on channel "
                  << std::to_string(m_dev_cfg.channel_tx) << ": "
                  << std::to_string(m_device->getFrequency(
                                            SOAPY_SDR_TX,
                                            m_dev_cfg.channel_tx)/1e6)
                  << " [MHz]" << std::endl;
}

void SDR::configure_rx()
{
        m_device->setSampleRate(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                                m_dev_cfg.sampling_rate_rx);
        double act_sample_rate = m_device->getSampleRate(
                SOAPY_SDR_RX,
                m_dev_cfg.channel_rx);
        std::cout << "Actual RX rate: "
                  << act_sample_rate
                  << " Msps" << std::endl;
        if (m_dev_cfg.rx_bw != -1) {
                m_device->setBandwidth(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                                       m_dev_cfg.rx_bw);
        }
        m_device->setGainMode(SOAPY_SDR_RX, m_dev_cfg.channel_rx, false);
        m_device->setGain(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                          m_dev_cfg.rx_gain);
        bool gain_mode = m_device->getGainMode(SOAPY_SDR_RX, m_dev_cfg.channel_rx);
        std::cout << "Gain mode " << gain_mode << std::endl;
        m_device->setAntenna(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                             m_dev_cfg.antenna_rx);
        m_device->setFrequency(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                               m_dev_cfg.rx_frequency);

        if (is_limesdr()) {
                /*
                  bool rx_lo_locked = false;
                while (not rx_lo_locked) {
                        std::string rx_locked = m_device->readSensor(
                                SOAPY_SDR_RX,
                                m_dev_cfg.channel_rx,
                                "lo_locked");
                        if (rx_locked == "true") {
                                rx_lo_locked = true;
                        }
                        usleep(100);
                }
                std::cout << "sdr: RX LO lock detected on channel "
                          << std::to_string(m_dev_cfg.channel_rx)
                          << std::endl;
                */
        }
        std::cout << "sdr: Actual RX frequency on channel "
                  << std::to_string(m_dev_cfg.channel_rx) << ": "
                  << std::to_string(m_device->getFrequency(
                                            SOAPY_SDR_RX,
                                            m_dev_cfg.channel_rx)/1e6)
                  << " [MHz]" << std::endl;
}

int64_t SDR::start()
{
        int64_t now_tick(-1);
        int mtu_tx(-1);
        int mtu_rx(-1);
        if (m_dev_cfg.rx_active) {
                now_tick = start_rx();
                mtu_rx = m_device->getStreamMTU(m_rx_stream);
        }
        if (m_dev_cfg.tx_active) {
                start_tx();
                mtu_tx = m_device->getStreamMTU(m_tx_stream);
        }
        std::cout << "sdr: mtu_tx="
                  << std::to_string(mtu_tx)
                  << " [Sa], mtu_rx="
                  << std::to_string(mtu_rx) + " [Sa]"
                  << std::endl;
        return now_tick;
}

void SDR::start_tx()
{
        SoapySDR::Kwargs args;
        if (m_dev_cfg.is_beacon) {
                args["beacon"] = 1;
        }
        m_tx_stream = m_device->setupStream(
                SOAPY_SDR_TX,
                SOAPY_SDR_CF32,
                std::vector<size_t>{(size_t)m_dev_cfg.channel_tx},
                args);
        if (m_tx_stream == nullptr) {
                throw std::runtime_error("Unable to setup TX stream!");
        } else {
                std::cout << "sdr: TX stream has been successfully set up!"
                          << std::endl;
        }
        int ret = m_device->activateStream(m_tx_stream);
        if (ret != 0) {
                std::string err = "sdr: Following problem occurred while";
                err += " activating TX stream: ";
                err += SoapySDR::errToStr(ret);
                throw std::runtime_error(err);
        } else {
                std::cout << "sdr: TX stream has been successfully activated!"
                          << std::endl;
        }
}

int64_t SDR::start_rx()
{
        SoapySDR::Kwargs args;
        if (m_dev_cfg.is_beacon) {
                args["beacon"] = 1;
        }
        m_rx_stream = m_device->setupStream(
                SOAPY_SDR_RX,
                SOAPY_SDR_CS16,
                std::vector<size_t>{(size_t)m_dev_cfg.channel_rx},
                args);
        if (m_rx_stream == nullptr) {
                throw std::runtime_error("Unable to setup RX stream!");
        } else {
                std::cout << "sdr: RX stream has been successfully set up!"
                          << std::endl;
        }
        int64_t now_hw_ticks(-1);
        m_device->setHardwareTime(0);
        int64_t now_hw_ns = m_device->getHardwareTime();
        now_hw_ticks = SoapySDR::timeNsToTicks(now_hw_ns,
                                                   m_dev_cfg.f_clk);
        m_rx_start_hw_ticks = now_hw_ticks;
        int64_t rx_future_rel_ns =
                (m_dev_cfg.time_in_future + m_dev_cfg.burst_period) * 1e9;
        m_rx_start_hw_ticks += SoapySDR::timeNsToTicks(rx_future_rel_ns,
                                                       m_dev_cfg.f_clk);
        int64_t burst_time = SoapySDR::ticksToTimeNs(m_rx_start_hw_ticks,
                                                     m_dev_cfg.f_clk);
        int rx_flags = SOAPY_SDR_HAS_TIME;
        rx_flags |= SOAPY_SDR_END_BURST;
        rx_flags |= SOAPY_SDR_ONE_PACKET;
        int ret = m_device->activateStream(m_rx_stream,
                                           rx_flags,
                                           burst_time);
        if (ret != 0) {
                std::string err = "sdr: Following problem occurred while";
                err += " activating RX stream: ";
                err += SoapySDR::errToStr(ret);
                throw std::runtime_error(err);
        } else {
                std::cout << "sdr: RX stream has been successfully activated!"
                          << std::endl;
        }
        return now_hw_ticks;
}

SoapySDR::Stream *SDR::get_rx_stream()
{
        return m_rx_stream;
}


size_t SDR::write(std::vector<void *> data, size_t no_of_samples,
                    long long int burst_time)
{
        int tx_flags = SOAPY_SDR_HAS_TIME;
        tx_flags |= SOAPY_SDR_END_BURST | SOAPY_SDR_ONE_PACKET;
        size_t no_of_transmitted_samples = m_device->writeStream(
                m_tx_stream,
                data.data(),
                no_of_samples,
                tx_flags,
                burst_time,
                1e6*m_dev_cfg.timeout);
        if (no_of_transmitted_samples != no_of_samples) {
                std::string tx_verbose_msg = "Transmit failed: ";
                switch (no_of_transmitted_samples) {
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
                default:
                        tx_verbose_msg += "Num of transmitted samps: ";
                        tx_verbose_msg += std::to_string(no_of_transmitted_samples);
                }
                std::cout << tx_verbose_msg << std::endl;
        } else {
                size_t chan_mask = 0;
                int stream_status = m_device->readStreamStatus(
                        m_tx_stream,
                        chan_mask,
                        tx_flags,
                        burst_time,
                        1e6*m_dev_cfg.timeout);
                std::string tx_verbose_msg = "";
                switch(stream_status) {
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
                default:
                        // No ERROR
                        break;
                }
                if (tx_verbose_msg != "") {
                        std::cout << "Stream status: "
                                  << tx_verbose_msg
                                  << std::endl;
                }
        }


        return no_of_transmitted_samples;

}

int32_t SDR::read(size_t no_of_samples,
                  std::vector<std::complex<int16_t>> &buff_data)
{
        int32_t no_of_received_samples(0);
        //int flags(0);
        int flags = SOAPY_SDR_HAS_TIME;
        flags |= SOAPY_SDR_END_BURST;
        //flags |= SOAPY_SDR_ONE_PACKET;
        long long int time_ns(0);
        buff_data.resize(no_of_samples);
        std::vector<void *> buffs_data;
        buffs_data.push_back(buff_data.data());
        no_of_received_samples = m_device->readStream(m_rx_stream,
                                                      buffs_data.data(),
                                                      no_of_samples,
                                                      flags,
                                                      time_ns);
        m_last_rx_timestamp = (int64_t)time_ns;
        return no_of_received_samples;
}

int64_t SDR::ix_to_hw_time(int64_t ix)
{
        int64_t hw_time;
        int64_t fs = m_dev_cfg.sampling_rate_rx;
        int64_t ix_ns = (ix * 1e9) / fs;
        /*std::cout << "ix "
                  << ix
                  << " ix_ns "
                  << ix_ns
                  << std::endl;*/
        hw_time = m_last_rx_timestamp + ix_ns;
        return hw_time;
}

int64_t SDR::expected_pong_pos_ix(int64_t hw_time_of_sync)
{
        int64_t expected_pong_pos_ix = expected_ping_pos_ix(hw_time_of_sync);
        expected_pong_pos_ix += m_dev_cfg.pong_pos_comp;
        if (expected_pong_pos_ix >= (int64_t)m_dev_cfg.no_of_rx_samples_pong) {
                expected_pong_pos_ix -= m_dev_cfg.no_of_rx_samples_pong;
        }
        return expected_pong_pos_ix;
}

int64_t SDR::expected_ping_pos_ix(int64_t hw_time_of_sync)
{
        int64_t exp_hw_time = hw_time_of_sync;
        int64_t burst_period_ns = m_dev_cfg.burst_period * 1e9;
        /* If the diff is too large there is something spoky
         * with the timestamps, and we skip this and try to
         * sample some new data, by setting the stamps equal.
         */
        uint64_t diff = std::abs(exp_hw_time - m_last_rx_timestamp);
        if (diff > 2e9) {
                std::cout << "Warning: strange timestamps,"
                          << " last timestamp on rx buffer: "
                          << m_last_rx_timestamp
                          << " expected timestamp: "
                          << exp_hw_time
                          << std::endl;
                exp_hw_time = m_last_rx_timestamp;
        }
        while (exp_hw_time < m_last_rx_timestamp) {
                exp_hw_time += burst_period_ns;
        }
        int64_t rx_burst_length = m_last_rx_timestamp + burst_period_ns;
        while (exp_hw_time > rx_burst_length) {
                exp_hw_time -= burst_period_ns;
        }
        int64_t fs = m_dev_cfg.sampling_rate_rx;
        int64_t ix = ((exp_hw_time - m_last_rx_timestamp) * fs) / 1e9;
        return ix;
}

void SDR::close()
{
        if (m_dev_cfg.tx_active) {
                m_device->deactivateStream(m_tx_stream);
                m_device->closeStream(m_tx_stream);
        }
        if (m_dev_cfg.rx_active) {
                m_device->deactivateStream(m_rx_stream);
                m_device->closeStream(m_rx_stream);
        }
        SoapySDR::Device::unmake(m_device);
}

int64_t SDR::check_burst_time(long long int burst_time)
{
        int64_t current_hardware_time = m_device->getHardwareTime();
        if (burst_time - current_hardware_time < 0) {
                std::cout << "burst_time: " << burst_time
                          << " current time:  " << current_hardware_time
                          << " diff: " << burst_time - current_hardware_time
                          << std::endl;
                /*
                double burst_period = m_dev_cfg.burst_period;
                //double sampling_rate = m_dev_cfg.sampling_rate_tx;
                //int64_t ticks_per_burst_period = (int64_t)(
                //        m_dev_cfg.D_tx * burst_period * sampling_rate);
                while (burst_time < current_hardware_time) {
                long long tx_tick = SoapySDR::timeNsToTicks(
                                burst_time,
                                m_dev_cfg.f_clk);
                                tx_tick += 10*ticks_per_burst_period;
                                burst_time = SoapySDR::ticksToTimeNs(tx_tick,
                                m_dev_cfg.f_clk);
                        burst_time += 100*burst_period;
                        current_hardware_time = m_device->getHardwareTime();
                        std::cout << burst_time-current_hardware_time << std::endl;

                }
                */
        }
        return burst_time;
}

bool SDR::is_limesdr()
{
        return (get_device_driver() == "lime");
}

bool SDR::is_bladerf()
{

        return (get_device_driver() == "bladerf");
}

void SDR::list_hw_info()
{
        SoapySDR::KwargsList result = m_device->enumerate();
        for (size_t n=0; n<result.size(); n++){
                std::cout << "*******************" << std::endl;
                SoapySDR::Kwargs::iterator it;
                for (it=result[n].begin(); it!=result[n].end(); ++it)
                        std::cout << it->first << " => "
                                  << it->second << std::endl;
        }
}

void SDR::check_lib_bladerf_support()
{
        std::string libname = "libbladeRFSupport";
        std::string version = get_modules_version(libname);
        std::size_t found = version.find(m_dev_cfg.req_soapybladerf_version);
        std::cout << "Lib BladeRF SOAPY support version: "
                  << version << std::endl;
        if (found == std::string::npos) {
                std::string err = "Need the Wittra SoapySDR";
                err += " plugin for BladeRF,";
                err += " version ";
                err += m_dev_cfg.req_soapybladerf_version;
                throw std::runtime_error(err);
        }
}

std::string SDR::get_modules_version(std::string libname)
{
        std::vector<std::string> modules = SoapySDR::listModules();
        std::string version = "";
        for (size_t n=0; n<modules.size(); n++){
                std::size_t found = modules[n].find(libname);
                if (found != std::string::npos) {
                        version = SoapySDR::getModuleVersion(modules[n]);
                }
        }
        return version;
}

std::string SDR::get_device_driver()
{
        SoapySDR::KwargsList result = m_device-> enumerate();
        return result[0]["driver"];
}

int32_t SDR::look_up_device_serial(SoapySDR::KwargsList result,
                                   std::string device_id)
{
        int32_t device_num(-1);
        for (size_t n=0; n<result.size(); n++) {
                if (result[n]["serial"] == device_id) {
                        device_num = n;
                }
        }
        return device_num;
}
