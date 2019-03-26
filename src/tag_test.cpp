/**
 * \file tag_test.cpp
 *
 * \brief Tag functionality
 *
 * The main functionality for the tag.
 *
 * \author Mats Gustafsson
 *
 * Copyright (C) 2019 by Wittra. All rights reserved.
 */

#include "tag_test.h"

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
                                              "Start tag",
                                              cmd, false);
                TCLAP::SwitchArg plot_switch("p","plot",
                                             "Plot data",
                                             cmd, false);
                cmd.parse(argc, argv);
                bool start_tag = start_switch.getValue();
                bool plot_data = plot_switch.getValue();
                if (start_tag) {
                        run_tag(plot_data);
                }
        }
        catch (TCLAP::ArgException &e) {
                std::cerr << "error: " << e.error()
                          << " for arg " << e.argId() << std::endl;
        }
        return EXIT_SUCCESS;
}

void run_tag(bool plot_data)
{
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
        //from INI
        if (LMS_Init(device) != 0)
                error();
        if (LMS_EnableChannel(device, LMS_CH_RX, 0, true) != 0)
                error();
        if (LMS_SetLOFrequency(device, LMS_CH_RX, 0, 800e6) != 0)
                error();
        //Set sample rate to 8 MHz, ask to use 2x oversampling in RF
        //This set sampling rate for all channels
        if (LMS_SetSampleRate(device, 8e6, 2) != 0)
                error();
        //Enable test signal generation
        //To receive data from RF, remove this line or change signal
        // to LMS_TESTSIG_NONE
        if (LMS_SetTestSignal(device, LMS_CH_RX, 0,
                              LMS_TESTSIG_NCODIV8,
                              0, 0) != 0)
                error();
        lms_stream_t streamId;
        streamId.channel = 0;
        streamId.fifoSize = 1024 * 1024;
        streamId.throughputVsLatency = 1.0; //optimize for max throughput
        streamId.isTx = false;
        streamId.dataFmt = lms_stream_t::LMS_FMT_I12;
        if (LMS_SetupStream(device, &streamId) != 0)
                error();
        const int sampleCnt = 5000;
        int16_t buffer[sampleCnt * 2];
        LMS_StartStream(&streamId);
        int samplesRead(0);
        auto t1 = std::chrono::high_resolution_clock::now();
        while (std::chrono::high_resolution_clock::now() - t1 < std::chrono::seconds(20))
        {
                samplesRead = LMS_RecvStream(&streamId, buffer, sampleCnt,
                                             NULL, 1000);
                //I and Q samples are interleaved in buffer: IQIQIQ...
                printf("Received %d samples\n", samplesRead);
                /*
                  INSERT CODE FOR PROCESSING RECEIVED SAMPLES
                */
        }

        if (plot_data) {
                arma::vec y_re(samplesRead);
                arma::vec y_im(samplesRead);
                for (int j = 0; j < samplesRead; ++j) {
                        y_re(j) = buffer[2 * j];
                        y_im(j) = buffer[2 * j + 1];
                }
                plot(y_re);
        }
        //stream is stopped but can be started again with LMS_StartStream()
        LMS_StopStream(&streamId);
        //stream is deallocated and can no longer be used
        LMS_DestroyStream(device, &streamId);
        LMS_Close(device);
}

/**
 * \fn plot
 * \brief Use gnuplot-cpp to plot data
 *
 * See the example.cc file that comes with the package for
 * examples on how to use the library.
 *
 */
void plot(std::vector<double> y, std::string title)
{
        Gnuplot g1("lines");
        g1.set_title(title);
        g1.plot_x(y);
        //usleep(1000000);
        wait_for_key();
}

void plot(std::vector<double> y)
{
        plot(y, "");
}

void plot(arma::vec y, std::string title)
{
        std::vector<double> y_p = arma::conv_to<std::vector<double>>::from(y);
        plot(y_p, title);
}

void plot(arma::vec y)
{
        std::vector<double> y_p = arma::conv_to<std::vector<double>>::from(y);
        plot(y_p, "");
}

void wait_for_key()
{
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__TOS_WIN__)
        std::cout << std::endl << "Press any key to continue..." << std::endl;

        FlushConsoleInputBuffer(GetStdHandle(STD_INPUT_HANDLE));
        _getch();
#elif defined(unix) || defined(__unix) || defined(__unix__) || defined(__APPLE__)
        std::cout << std::endl << "Press ENTER to continue..." << std::endl;
        std::cin.clear();
        std::cin.ignore(std::cin.rdbuf()->in_avail());
        std::cin.get();
#endif
        return;
}

int error()
{
        if (device != NULL)
                LMS_Close(device);
        exit(-1);
}
