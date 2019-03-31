/**
 * \file sdr.cpp
 *
 * \brief SDR class
 *
 * Class for the SDR
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
        m_device->setSampleRate(SOAPY_SDR_TX, dev_cfg.channel_tx,
                                m_dev_cfg.sampling_rate);
        double act_sample_rate = m_device->getSampleRate(SOAPY_SDR_TX,
                                                         dev_cfg.channel_tx);
        std::cout << "Actual TX rate: "
                  << act_sample_rate
                  << " Msps" << std::endl;
        if (dev_cfg.tx_bw != -1) {
                m_device->setBandwidth(SOAPY_SDR_TX, dev_cfg.channel_tx,
                                       m_dev_cfg.tx_bw);
        }
        m_device->setGain(SOAPY_SDR_TX, dev_cfg.channel_tx,
                          m_dev_cfg.tx_gain);
        m_device->setAntenna(SOAPY_SDR_TX, dev_cfg.channel_tx,
                             m_dev_cfg.antenna_tx);
        m_device->setFrequency(SOAPY_SDR_TX, dev_cfg.channel_tx,
                               m_dev_cfg.frequency);

        if (is_limesdr()) {
                bool tx_lo_locked = false;
                while (not tx_lo_locked) {
                        std::string tx_locked = m_device->readSensor(
                                SOAPY_SDR_TX,
                                dev_cfg.channel_tx,
                                "lo_locked");
                        if (tx_locked == "true") {
                                tx_lo_locked = true;
                        }
                        usleep(100);
                }
                std::cout << "sdr: TX LO lock detected on channel "
                          << std::to_string(dev_cfg.channel_tx)
                          << std::endl;
        }
        std::cout << "sdr: Actual TX frequency on channel "
                  << std::to_string(dev_cfg.channel_tx) << ": "
                  << std::to_string(m_device->getFrequency(
                                            SOAPY_SDR_TX,
                                            dev_cfg.channel_tx)/1e6)
                  << " [MHz]" << std::endl;
        SoapySDR::Stream *tx_stream;
        tx_stream = m_device->setupStream(
                SOAPY_SDR_TX,
                SOAPY_SDR_CF32,
                std::vector<size_t>{(size_t)dev_cfg.channel_tx});
        if (tx_stream == nullptr) {
                throw std::runtime_error("Unable to setup TX stream!");
        } else {
                std::cout << "sdr: TX stream has been successfully set up!"
                          << std::endl;
        }
}

void SDR::start()
{}

void SDR::write(std::vector<std::complex<float>> data)
{
        size_t a = data.size();
        a++;
}

std::vector<std::vector<std::complex<int16_t>>> SDR::read()
{
        std::vector<std::vector<std::complex<int16_t>>> a;
        return a;
}

bool SDR::is_limesdr()
{
        SoapySDR::KwargsList result = m_device-> enumerate();
        return (result[0]["driver"] == "lime");
}

bool SDR::is_bladerf()
{
        SoapySDR::KwargsList result = m_device-> enumerate();
        return (result[0]["driver"] == "bladerf");
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
