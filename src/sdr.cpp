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
        m_device = SoapySDR::Device::make();
        if (m_device == nullptr) {
                throw std::runtime_error("Could not open device!");
        }
        if (!m_device->hasHardwareTime()) {
                std::string err = "This device does not support timed";
                err +=  " streaming!";
                throw std::runtime_error(err);
        }
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
        m_device->setSampleRate(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                                m_dev_cfg.sampling_rate);
        double act_sample_rate = m_device->getSampleRate(
                SOAPY_SDR_TX,
                m_dev_cfg.channel_tx);
        std::cout << "Actual TX rate: "
                  << act_sample_rate
                  << " Msps" << std::endl;
        m_device->setSampleRate(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                                m_dev_cfg.sampling_rate);
        act_sample_rate = m_device->getSampleRate(
                SOAPY_SDR_RX,
                m_dev_cfg.channel_rx);
        std::cout << "Actual RX rate: "
                  << act_sample_rate
                  << " Msps" << std::endl;

        if (m_dev_cfg.tx_bw != -1) {
                m_device->setBandwidth(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                                       m_dev_cfg.tx_bw);
        }
        if (m_dev_cfg.rx_bw != -1) {
                m_device->setBandwidth(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                                       m_dev_cfg.rx_bw);
        }

        m_device->setGain(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                          m_dev_cfg.tx_gain);
        m_device->setGain(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                          m_dev_cfg.rx_gain);
        m_device->setAntenna(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                             m_dev_cfg.antenna_tx);
        m_device->setAntenna(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                             m_dev_cfg.antenna_rx);
        m_device->setFrequency(SOAPY_SDR_TX, m_dev_cfg.channel_tx,
                               m_dev_cfg.frequency);
        m_device->setFrequency(SOAPY_SDR_RX, m_dev_cfg.channel_rx,
                               m_dev_cfg.frequency);

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
        }
        std::cout << "sdr: Actual TX frequency on channel "
                  << std::to_string(m_dev_cfg.channel_tx) << ": "
                  << std::to_string(m_device->getFrequency(
                                            SOAPY_SDR_TX,
                                            m_dev_cfg.channel_tx)/1e6)
                  << " [MHz]" << std::endl;
        std::cout << "sdr: Actual RX frequency on channel "
                  << std::to_string(m_dev_cfg.channel_rx) << ": "
                  << std::to_string(m_device->getFrequency(
                                            SOAPY_SDR_RX,
                                            m_dev_cfg.channel_rx)/1e6)
                  << " [MHz]" << std::endl;
}

int64_t SDR::start()
{
        m_tx_stream = m_device->setupStream(
                SOAPY_SDR_TX,
                SOAPY_SDR_CF32,
                std::vector<size_t>{(size_t)m_dev_cfg.channel_tx});
        if (m_tx_stream == nullptr) {
                throw std::runtime_error("Unable to setup TX stream!");
        } else {
                std::cout << "sdr: TX stream has been successfully set up!"
                          << std::endl;
        }
        m_rx_stream = m_device->setupStream(
                SOAPY_SDR_RX,
                SOAPY_SDR_CF32,
                std::vector<size_t>{(size_t)m_dev_cfg.channel_rx});
        if (m_rx_stream == nullptr) {
                throw std::runtime_error("Unable to setup RX stream!");
        } else {
                std::cout << "sdr: RX stream has been successfully set up!"
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
        m_device->setHardwareTime(0);
        int64_t current_hardware_time = m_device->getHardwareTime();
        int64_t now_tick = SoapySDR::timeNsToTicks(current_hardware_time,
                                                   m_dev_cfg.f_clk);
        m_rx_start_tick = now_tick;
        int64_t rx_future = m_dev_cfg.time_in_future + m_dev_cfg.burst_period;
        m_rx_start_tick += SoapySDR::timeNsToTicks(rx_future * 1e9,
                                                   m_dev_cfg.f_clk);
        int64_t burst_time = SoapySDR::ticksToTimeNs(m_rx_start_tick,
                                                     m_dev_cfg.f_clk);

        int rx_flags = SOAPY_SDR_HAS_TIME;
        rx_flags |= SOAPY_SDR_END_BURST;
        rx_flags |= SOAPY_SDR_ONE_PACKET;
        size_t no_of_requested_samples(100);
        ret = m_device->activateStream(m_rx_stream,
                                       rx_flags,
                                       burst_time,
                                       no_of_requested_samples);
        if (ret != 0) {
                std::string err = "sdr: Following problem occurred while";
                err += " activating RX stream: ";
                err += SoapySDR::errToStr(ret);
                throw std::runtime_error(err);
        } else {
                std::cout << "sdr: RX stream has been successfully activated!"
                          << std::endl;
        }
        int mtu_tx = m_device->getStreamMTU(m_tx_stream);
        int mtu_rx = m_device->getStreamMTU(m_rx_stream);
        std::cout << "sdr: mtu_tx="
                  << std::to_string(mtu_tx)
                  << " [Sa], mtu_rx="
                  << std::to_string(mtu_rx) + " [Sa]"
                  << std::endl;
        return now_tick;
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

int32_t SDR::read(std::vector<void *> &data, size_t no_of_samples)
{
        int32_t no_of_received_samples(0);
        int flags(0);
        long long time_ns(0);
        no_of_received_samples = m_device->readStream(m_rx_stream,
                                                      data.data(),
                                                      no_of_samples,
                                                      flags,
                                                      time_ns);
        return no_of_received_samples;
}

void SDR::close()
{
        m_device->deactivateStream(m_tx_stream);
        m_device->closeStream(m_tx_stream);
        m_device->deactivateStream(m_rx_stream);
        m_device->closeStream(m_rx_stream);
        SoapySDR::Device::unmake(m_device);

}

void SDR::check_burst_time(long long int burst_time)
{
        int64_t current_hardware_time = m_device->getHardwareTime();
        if (burst_time - current_hardware_time < 0) {
                std::cout << "burst_time: " << burst_time
                          << " current time:  " << current_hardware_time
                          << " diff: " << burst_time - current_hardware_time
                          << std::endl;
        }
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
        SoapySDR::KwargsList result = m_device-> enumerate();
        for (size_t n=0; n<result.size(); n++){
                SoapySDR::Kwargs::iterator it;
                for (it=result[n].begin(); it!=result[n].end(); ++it)
                        std::cout << it->first << " => "
                                  << it->second << std::endl;
        }
}

std::string SDR::get_device_driver()
{
        SoapySDR::KwargsList result = m_device-> enumerate();
        return result[0]["driver"];
}
