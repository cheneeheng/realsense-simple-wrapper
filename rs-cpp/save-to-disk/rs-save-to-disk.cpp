// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015-2017 Intel Corporation. All Rights Reserved.

#include <librealsense2/rs.hpp> // Include RealSense Cross Platform API
#include <librealsense2-net/rs_net.hpp>
#include <librealsense2/h/rs_pipeline.h>

#include <sys/types.h>
#include <sys/stat.h>

#include <cstdlib>
#include <fstream>  // File IO
#include <iostream> // Terminal IO
#include <sstream>  // Stringstreams

// 3rd party header for writing png files
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

/**
 * @brief Save raw frame data into a binary file.
 *
 * Taken from : https://github.com/IntelRealSense/librealsense/issues/1485
 *
 * @param filename
 * @param frame
 * @return true
 * @return false
 */
bool save_raw_frame_data(const std::string &filename, rs2::frame frame);

/**
 * @brief Saves the metadata from rs2::frame into a csv file.
 *
 * @param frm An instance of rs2::frame .
 * @param filename name of file to save to.
 */
void metadata_to_csv(const rs2::frame &frm, const std::string &filename);

/**
 * @brief Class that contains the arguments for the rs2wrapper class.
 *
 */
class rs2args
{
    int _argc;
    char **_argv;

public:
    /**
     * @brief Construct a new rs2args object
     *
     * @param argc Number of arguments.
     * @param argv Arguments in an array of char*.
     */
    rs2args(int argc, char *argv[])
    {
        _argc = argc;
        _argv = argv;
    }
    /**
     * @brief FPS of the realsense device.
     *
     * @return int
     */
    int fps()
    {
        return atoi(_argv[1]);
    }
    /**
     * @brief Height of the realsense frames.
     *
     * @return int
     */
    int height()
    {
        return atoi(_argv[2]);
    }
    /**
     * @brief Width of the realsense frames.
     *
     * @return int
     */
    int width()
    {
        return atoi(_argv[3]);
    }
    /**
     * @brief Path to save the frames.
     *
     * @return const char*
     */
    const char *save_path()
    {
        return _argv[4];
    }
    /**
     * @brief IP address of the remote realsense device (optional).
     *
     * @return std::string
     */
    std::string ip()
    {
        return (std::string)_argv[5];
    }
    /**
     * @brief Checks whether the realsense device is connected over the network.
     *
     * @return true
     * @return false
     */
    bool network()
    {
        if (_argc == 6)
            return true;
        else
            return false;
    }
    /**
     * @brief Prints out the arguments from 'rs2args' class that is used for the 'rs2wrapper' class.
     *
     */
    void info()
    {
        std::cout << "========================================" << std::endl;
        std::cout << ">>>>> rs2args <<<<<" << std::endl;
        std::cout << "========================================" << std::endl;
        std::cout << "FPS        : " << fps() << std::endl;
        std::cout << "Height     : " << height() << std::endl;
        std::cout << "Width      : " << width() << std::endl;
        std::cout << "Save Path  : " << save_path() << std::endl;
        std::cout << "IP Address : " << ip() << std::endl;
        std::cout << "========================================" << std::endl;
    }
};

/**
 * @brief Wrapper class for the librealsense library to run a realsense device.
 *
 */
class rs2wrapper : public rs2args
{

    rs2::config cfg;
    rs2::pipeline pipe;
    // Declare depth colorizer for pretty visualization of depth data
    rs2::colorizer color_map;

public:
    /**
     * @brief Construct a new rs2wrapper object
     *
     * @param argc Number of arguments.
     * @param argv Arguments in an array of char*.
     * @param ctx An insstance of rs2::context .
     */
    rs2wrapper(int argc,
               char *argv[],
               rs2::context ctx = rs2::context()) : rs2args(argc, argv)
    {
        // prints out info
        info();
        // Create save directory
        mkdir(save_path(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
        // Add network device context
        if (network())
        {
            rs2::net_device dev(ip());
            std::cout << "IP address found..." << std::endl;
            dev.add_to(ctx);
        }
        // Configure pipeline config
        // Start pipeline
        pipe = rs2::pipeline(ctx);
        cfg.enable_stream(RS2_STREAM_COLOR, width(), height(), RS2_FORMAT_RGB8, fps());
        cfg.enable_stream(RS2_STREAM_DEPTH, width(), height(), RS2_FORMAT_Z16, fps());
        rs2::pipeline_profile profile = pipe.start(cfg);
        std::cout << "Pipeline started..." << std::endl;
        std::cout << "Initialized realsense device..." << std::endl;
    }
    /**
     * @brief Flushes N initial frames o give autoexposure, etc. a chance to settle.
     *
     */
    void initial_flush(int num_frames = 30)
    {
        for (auto i = 0; i < num_frames; ++i)
            pipe.wait_for_frames();
        std::cout << "Flushed 30 initial frames..." << std::endl;
    }
    /**
     * @brief Collects a set of frames and postprocesses them.
     *
     * Currently this function polls for a set of frames and
     * saves the color & depth (as colormap) images as png,
     * and their metadata as csv.
     *
     * @param save_file_prefix Prefix for the save file.
     */
    void step(const std::string &save_file_prefix)
    {
        // Loop through the set of frames from the camera.
        for (auto &&frame : pipe.wait_for_frames())
        {
            // We can only save video frames as pngs, so we skip the rest
            if (rs2::video_frame vf = frame.as<rs2::video_frame>())
            {
                // auto stream = frame.get_profile().stream_type();
                // Use the colorizer to get an rgb image for the depth stream
                if (vf.is<rs2::depth_frame>())
                    vf = color_map.process(frame);

                // Write images to disk
                std::stringstream png_file;
                png_file << save_path()
                         << "/"
                         << save_file_prefix
                         << "-rs-save-to-disk-output-"
                         << vf.get_profile().stream_name()
                         << ".png";
                save_raw_frame_data(png_file.str(), frame);
                // stbi_write_png(png_file.str().c_str(),
                //                vf.get_width(),
                //                vf.get_height(),
                //                vf.get_bytes_per_pixel(),
                //                vf.get_data(),
                //                vf.get_stride_in_bytes());
                std::cout << "Saved " << png_file.str() << std::endl;

                // Record per-frame metadata for UVC streams
                std::stringstream csv_file;
                csv_file << save_path()
                         << "/"
                         << save_file_prefix
                         << "-rs-save-to-disk-output-"
                         << vf.get_profile().stream_name()
                         << "-metadata.csv";
                metadata_to_csv(vf, csv_file.str());
            }
        }
    }
};

// This sample captures 30 frames and writes the last frame to disk.
// It can be useful for debugging an embedded system with no display.
int main(int argc, char *argv[])
try
{

    if (argc != 5 && argc != 6)
    {
        std::cout << "Please enter fps, height, width, save path, {ipaddress}" << std::endl;
        throw std::invalid_argument("There should be 4 or 5 arguments");
    }

    // Intialize the wrapper
    // Declare RealSense pipeline, encapsulating the actual device and sensors
    // Start streaming with args defined configuration
    rs2::context ctx;
    rs2wrapper rs2_dev(argc, argv, ctx);
    rs2_dev.initial_flush();

    for (auto i = 0; i < rs2_dev.fps() * 10; ++i)
    {
        std::size_t num_zeros = 6;
        std::string old_str = std::to_string(i);
        std::string i_str = std::string(num_zeros - std::min(num_zeros, old_str.length()), '0') + old_str;
        rs2_dev.step(i_str);
    }
    return EXIT_SUCCESS;
}
catch (const rs2::error &e)
{
    std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
    return EXIT_FAILURE;
}
catch (const std::exception &e)
{
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
}

void metadata_to_csv(const rs2::frame &frm, const std::string &filename)
{
    std::ofstream csv;

    csv.open(filename);

    //    std::cout << "Writing metadata to " << filename << endl;
    csv << "Stream," << rs2_stream_to_string(frm.get_profile().stream_type()) << "\nMetadata Attribute,Value\n";

    // Record all the available metadata attributes
    for (size_t i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
    {
        if (frm.supports_frame_metadata((rs2_frame_metadata_value)i))
        {
            csv << rs2_frame_metadata_to_string((rs2_frame_metadata_value)i) << ","
                << frm.get_frame_metadata((rs2_frame_metadata_value)i) << "\n";
        }
    }

    csv.close();
}

bool save_raw_frame_data(const std::string &filename, rs2::frame frame)
{
    bool ret = false;
    auto image = frame.as<rs2::video_frame>();
    if (image)
    {
        std::ofstream outfile(filename.data(), std::ofstream::binary);
        outfile.write(static_cast<const char *>(image.get_data()),
                      image.get_height() * image.get_stride_in_bytes());
        outfile.close();
        ret = true;
    }
    return ret;
}