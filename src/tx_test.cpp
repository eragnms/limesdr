// https://discourse.myriadrf.org/t/tx-bursts-with-timestamps/4457

#include "tx_test.h"
SoapySDR::Device *device;
static sig_atomic_t stop = false;
void sigIntHandler(const int)
{
        stop = true;
}
int main()
{
        run_test();
        return EXIT_SUCCESS;
}
void run_test()
{
        const double frequency(500e6);
        double f_clk(37.8e6);
        uint16_t D_tx = 8;
        const double sampling_rate(f_clk / D_tx);
        const double tone_freq(8e3);
        const double f_ratio = tone_freq/sampling_rate;
        const double tx_gain(60);
        const double tx_bw(-1);
        std::string clock_source = "";
		std::string time_source = "";
        double T_timeout(2);
        double time_in_future(1);
        double burst_period(10e-3);
        double tx_burst_length(0.5e-3);
        SoapySDR::setLogLevel(SoapySDR::LogLevel::SOAPY_SDR_DEBUG);
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
        if (clock_source != "") {
                device->setClockSource(clock_source);
        }
        if (time_source != "") {
                device->setTimeSource(time_source);
        }
        device->setMasterClockRate(f_clk);
        const size_t tx_ch(0);
        const size_t rx_ch(0);
        device->setSampleRate(SOAPY_SDR_TX, tx_ch, sampling_rate);
        double act_sample_rate = device->getSampleRate(SOAPY_SDR_TX, tx_ch);
        std::cout << "Actual TX rate: " << act_sample_rate << " Msps" << std::endl;
        if (tx_bw != -1) {
                device->setBandwidth(SOAPY_SDR_TX, tx_ch, tx_bw);
        }
        device->setGain(SOAPY_SDR_TX, tx_ch, tx_gain);
        device->setAntenna(SOAPY_SDR_TX, tx_ch, "BAND1");
        device->setFrequency(SOAPY_SDR_TX, tx_ch, frequency);
        /*
        bool tx_lo_locked = false;
        while (!(stop || tx_lo_locked)) {
                std::string tx_locked = device->readSensor(SOAPY_SDR_TX,
                                                           tx_ch,
                                                           "lo_locked");
                if (tx_locked == "true") {
                        tx_lo_locked = true;
                }
                usleep(100);
        }
        */
        std::cout << "sdr: TX LO lock detected on channel "
                  << std::to_string(tx_ch) << std::endl;
        std::cout << "sdr: Actual TX frequency on channel "
                  << std::to_string(tx_ch) << ": "
                  << std::to_string(device->getFrequency(SOAPY_SDR_TX,
                                                         tx_ch)/1e6)
                  << " [MHz]" << std::endl;
        SoapySDR::Stream *tx_stream;
        tx_stream = device->setupStream(SOAPY_SDR_TX,
                                        SOAPY_SDR_CF32,
                                        std::vector<size_t>{(size_t)tx_ch});
        if (tx_stream == nullptr) {
                throw std::runtime_error("Unable to setup TX stream!");
        } else {
                std::cout << "sdr: TX stream has been successfully set up!"
                          << std::endl;
        }
        SoapySDR::Stream *rx_stream;
        rx_stream = device->setupStream(SOAPY_SDR_RX,
                                        "CF32",
                                        std::vector<size_t>{(size_t)rx_ch});
        if (rx_stream == nullptr) {
                throw std::runtime_error("Unable to setup RX stream!");
        } else {
                std::cout << "sdr: RX stream has been successfully set up!"
                          << std::endl;
        }
        int ret = device->activateStream(tx_stream);
        if (ret != 0) {
                std::string err = "sdr: Following problem occurred while";
                err += " activating TX stream: ";
                err += SoapySDR::errToStr(ret);
                throw std::runtime_error(err);
        } else {
                std::cout << "sdr: TX stream has been successfully activated!"
                          << std::endl;
        }
        int mtu_tx = device->getStreamMTU(tx_stream);
        int mtu_rx = device->getStreamMTU(rx_stream);
        std::cout << "sdr: mtu_tx="
                  << std::to_string(mtu_tx)
                  << " [Sa], mtu_rx="
                  << std::to_string(mtu_rx) + " [Sa]"
                  << std::endl;
        device->setHardwareTime(0);
        size_t buffer_size_tx = tx_burst_length * sampling_rate;
        size_t no_of_tx_samples = buffer_size_tx;
        int64_t tmp = D_tx * burst_period * sampling_rate;
        int64_t no_of_ticks_per_bursts_period = tmp;
        std::vector<std::complex<float>> tx_buff_data(no_of_tx_samples,
                                                      std::complex<float>(1.0f, 0.0f));
        for (size_t i = 0; i<no_of_tx_samples; i++) {
                const double pi = acos(-1);
                double w = 2*pi*i*f_ratio;
                tx_buff_data[i] = std::complex<float>(cos(w), sin(w));
        }
        std::vector<void *> tx_buffs_data;
        tx_buffs_data.push_back(tx_buff_data.data());

        std::cout << "sample count per send call: "
                  << no_of_tx_samples << std::endl;
        int64_t current_hardware_time = device->getHardwareTime();
        int64_t now_tick = SoapySDR::timeNsToTicks(current_hardware_time,
                                                   f_clk);
        int64_t rx_start_tick = now_tick;
        int64_t rx_future = time_in_future + burst_period;
        rx_start_tick += SoapySDR::timeNsToTicks(rx_future * 1e9, f_clk);
        int64_t tx_start_tick = now_tick;
        int64_t tx_future = time_in_future + 2 * burst_period;
        tx_start_tick += SoapySDR::timeNsToTicks((tx_future) * 1e9, f_clk);
        int64_t burst_time = SoapySDR::ticksToTimeNs(rx_start_tick, f_clk);
        int rx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST | SOAPY_SDR_ONE_PACKET;
        size_t no_of_requested_samples(100);
        ret = device->activateStream(rx_stream,
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
        int64_t tx_tick = tx_start_tick;
        std::cout << "Starting stream loop, press Ctrl+C to exit..."
                  << std::endl;
        signal(SIGINT, sigIntHandler);
        while (not stop) {
                long long int burst_time = SoapySDR::ticksToTimeNs(tx_tick,
                                                                   f_clk);
                current_hardware_time = device->getHardwareTime();
                std::cout << "burst_time: " << burst_time
                          << " current time:  " << current_hardware_time
                          << " diff: " << burst_time - current_hardware_time
                          << std::endl;
                int tx_flags = SOAPY_SDR_HAS_TIME | SOAPY_SDR_END_BURST | SOAPY_SDR_ONE_PACKET;
                uint32_t no_of_transmitted_samples;
                no_of_transmitted_samples = device->writeStream(
                        tx_stream,
                        tx_buffs_data.data(),
                        no_of_tx_samples,
                        tx_flags,
                        burst_time,
                        1e6*T_timeout);
                tx_tick += no_of_ticks_per_bursts_period;
                if (no_of_transmitted_samples != no_of_tx_samples) {
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
                        int stream_status = device->readStreamStatus(
                                tx_stream,
                                chan_mask,
                                tx_flags,
                                burst_time,
                                1e6*T_timeout);
                        std::string tx_verbose_msg = "Stream status: ";
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
                                tx_verbose_msg += "NO_ERROR";
                                break;
                        }
                        std::cout << tx_verbose_msg << std::endl;
                }
        }
        device->deactivateStream(tx_stream);
        device->closeStream(tx_stream);
        device->deactivateStream(rx_stream);
        device->closeStream(rx_stream);
        SoapySDR::Device::unmake(device);
}
