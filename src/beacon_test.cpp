/**
 * \file beacon_test.cpp
 *
 * \brief Beacon functionality
 *
 * The main functionality for the beacon.
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "beacon_test.h"

//Device structure, should be initialize to NULL
lms_device_t* device = NULL;

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
        const double tone_freq = 2e6; //tone frequency
        const double f_ratio = tone_freq/sample_rate;
        //Find devices
        int n;
        lms_info_str_t list[8];
        if ((n = LMS_GetDeviceList(list)) < 0)
                error();
        std::cout << "Devices found: " << n << std::endl;
        if (n < 1)
                return;
        if (LMS_Open(&device, list[0], NULL))
                error();
        //Initialize device with default configuration
        //Do not use if you want to keep existing configuration
        //Use LMS_LoadConfig(device, "/path/to/file.ini") to load config
        // from INI
        if (LMS_Init(device)!=0)
                error();
        //Enable TX channel,Channels are numbered starting at 0
        if (LMS_EnableChannel(device, LMS_CH_TX, 0, true)!=0)
                error();
        if (LMS_SetSampleRate(device, sample_rate, 0)!=0)
                error();
        std::cout << "Sample rate: " << sample_rate/1e6 << " MHz"
                  << std::endl;
        if (LMS_SetLOFrequency(device,LMS_CH_TX, 0, frequency)!=0)
                error();
        std::cout << "Center frequency: " << frequency/1e6
                  << " MHz" << std::endl;
        //select TX1_1 antenna
        if (LMS_SetAntenna(device, LMS_CH_TX, 0, LMS_PATH_TX1)!=0)
                error();
        if (LMS_SetNormalizedGain(device, LMS_CH_TX, 0, 0.7) != 0)
                error();
        //calibrate Tx, continue on failure
        LMS_Calibrate(device, LMS_CH_TX, 0, sample_rate, 0);
        //Streaming Setup
        lms_stream_t tx_stream;
        tx_stream.channel = 0;
        tx_stream.fifoSize = 256*1024; //fifo size in samples
        tx_stream.throughputVsLatency = 0.5; //0 min latency, 1 max throughput
        tx_stream.dataFmt = lms_stream_t::LMS_FMT_F32; //floating point samps
        tx_stream.isTx = true;
        LMS_SetupStream(device, &tx_stream);
        //Initialize data buffers
        const int buffer_size = 1024*8;
        float tx_buffer[2*buffer_size]; //buffer to hold complex values
        //generate TX tone
        for (int i = 0; i <buffer_size; i++) {
                const double pi = acos(-1);
                double w = 2*pi*i*f_ratio;
                tx_buffer[2*i] = cos(w);
                tx_buffer[2*i+1] = sin(w);
        }
        std::cout << "Tx tone frequency: " << tone_freq/1e6 << " MHz"
                  << std::endl;
        const int send_cnt = int(buffer_size*f_ratio) / f_ratio;
        std::cout << "sample count per send call: " << send_cnt << std::endl;
        LMS_StartStream(&tx_stream);
        //Streaming
        auto t1 = std::chrono::high_resolution_clock::now();
        auto t2 = t1;
        while (std::chrono::high_resolution_clock::now() - t1<std::chrono::seconds(30))
        {
                int ret = LMS_SendStream(&tx_stream, tx_buffer, send_cnt,
                                         nullptr, 1000);
                if (ret != send_cnt)
                        std::cout << "error: samples sent: "
                                  << ret << "/" << send_cnt << std::endl;
                //Print data rate (once per second)
                if (std::chrono::high_resolution_clock::now() - t2>std::chrono::seconds(1))
                {
                        t2 = std::chrono::high_resolution_clock::now();
                        lms_stream_status_t status;
                        LMS_GetStreamStatus(&tx_stream, &status);
                        std::cout << "TX data rate: "
                             << status.linkRate / 1e6 << " MB/s\n";
                }
        }
        LMS_StopStream(&tx_stream);
        LMS_DestroyStream(device, &tx_stream);
        if (LMS_EnableChannel(device, LMS_CH_TX, 0, false)!=0)
                error();
        if (LMS_Close(device)==0)
                std::cout << "Closed" << std::endl;
}

int error()
{
        if (device != NULL)
                LMS_Close(device);
        exit(-1);
}
