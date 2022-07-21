#include "utils.h"
#include "rs_wrapper.h"

// doesnt work in the class init.
rs2::align align_to_depth(RS2_STREAM_DEPTH);
rs2::align align_to_color(RS2_STREAM_COLOR);

int num_zeros_to_pad = NUM_ZEROS_TO_PAD;

std::string pad_zeros(const std::string &in_str, const size_t &num_zeros)
{
    std::string out_str =
        std::string(num_zeros - std::min(num_zeros, in_str.length()), '0') +
        in_str;
    return out_str;
}

std::string filename_prefix_with_timestamp(const rs2::frame &frm,
                                           const std::string &prefix,
                                           const size_t &num_zeros)
{
    std::string _prefix;
    if (frm.supports_frame_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP))
    {
        _prefix = std::to_string(frm.get_frame_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP));
        _prefix = pad_zeros(_prefix, num_zeros);
    }
    else if (frm.supports_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP))
    {
        _prefix = std::to_string(frm.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP));
        _prefix = pad_zeros(_prefix, num_zeros);
    }
    else if (frm.supports_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP))
    {
        _prefix = std::to_string(frm.get_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP));
        _prefix = pad_zeros(_prefix, num_zeros);
    }
    else
    {
        _prefix = prefix;
    }
    return _prefix;
}

bool framedata_to_bin(const rs2::frame &frm, const std::string &filename)
{
    bool ret = false;
    rs2::video_frame image = frm.as<rs2::video_frame>();
    if (image)
    {
        std::ofstream outfile(filename, std::ofstream::binary);
        outfile.write(static_cast<const char *>(image.get_data()),
                      image.get_height() * image.get_stride_in_bytes());
        outfile.close();
        ret = true;
    }
    return ret;
}

void metadata_to_csv(const rs2::frame &frm, const std::string &filename)
{
    std::ofstream csv;
    csv.open(filename);

    csv << "Stream,"
        << rs2_stream_to_string(frm.get_profile().stream_type())
        << "\nAttribute,Value\n";

    rs2_frame_metadata_value metadata_idx;
    for (size_t i = 0; i < RS2_FRAME_METADATA_COUNT; i++)
    {
        metadata_idx = (rs2_frame_metadata_value)i;
        if (frm.supports_frame_metadata(metadata_idx))
        {
            // rs2_metadata_type => long long.
            rs2_metadata_type metadata = frm.get_frame_metadata(metadata_idx);
            csv << rs2_frame_metadata_to_string(metadata_idx)
                << ","
                << metadata
                << "\n";
        }
    }
    csv.close();
}

storagepaths::storagepaths()
{
}

void storagepaths::create(const std::string &device_sn,
                          const std::string &base_path)
{
    std::string path;
    // Base
    path = base_path;
    mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    // Device
    path.append("/");
    path.append(device_sn);
    mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    // System time , trial
    time_t current_time;
    time(&current_time);
    path.append("/");
    path.append(std::to_string(current_time));
    mkdir(path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    // Calib
    calib = path + "/calib";
    mkdir(calib.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    // Timestamp
    timestamp = path + "/timestamp";
    mkdir(timestamp.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    // Color
    color = path + "/color";
    mkdir(color.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    color_metadata = path + "/color_metadata";
    mkdir(color_metadata.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    // Depth
    depth = path + "/depth";
    mkdir(depth.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
    depth_metadata = path + "/depth_metadata";
    mkdir(depth_metadata.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}

void storagepaths::show()
{
    print("calib : " + calib, 0);
    print("color : " + color, 0);
    print("depth : " + depth, 0);
    print("color_metadata : " + color_metadata, 0);
    print("depth_metadata : " + depth_metadata, 0);
}

/*******************************************************************************
 * rs2wrapper PUBLIC FUNCTIONS
 ******************************************************************************/
rs2wrapper::rs2wrapper(int argc,
                       char *argv[],
                       rs2::context context,
                       std::string device_sn) : rs2args(argc, argv)
{
    // prints out CLI args
    print_args();

    single_device_sn = device_sn;

    // context grabs the usb resources of the cameras.
    ctx = std::make_shared<rs2::context>(context);

    // Get available devices
    if (network())
    {
        print("Network mode", 0);
        rs2::net_device dev((std::string)ip());
        print("Network device found", 0);
        dev.add_to(*ctx);
        auto serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
        std::vector<std::string> available_device;
        available_device.push_back(serial);
        available_device.push_back(ip());
        available_devices.push_back(available_device);
        print("using : " + std::string(serial));
    }
    else
    {
        print("Local mode", 0);
        if (single_device_sn == "-1")
        {
            for (auto &&dev : ctx->query_devices())
            {
                auto serial = dev.get_info(RS2_CAMERA_INFO_SERIAL_NUMBER);
                auto product_line = dev.get_info(RS2_CAMERA_INFO_PRODUCT_LINE);
                std::vector<std::string> available_device;
                available_device.push_back(serial);
                available_device.push_back(product_line);
                available_devices.push_back(available_device);
                print("found : " + std::string(serial));
            }
        }
        else
        {
            std::vector<std::string> available_device;
            available_device.push_back(single_device_sn.c_str());
            available_device.push_back("D400");
            available_devices.push_back(available_device);
            print("using : " + std::string(single_device_sn.c_str()));
        }
    }

    // Sort the devices.
    std::sort(available_devices.begin(), available_devices.end(),
              [](const std::vector<std::string> &a,
                 const std::vector<std::string> &b)
              {
                  return a[0] < b[0];
              });

    // storage
    for (auto &&available_device : available_devices)
    {
        storagepaths _storagepaths;
        _storagepaths.create(available_device[0], save_path());
        storagepaths_perdev[available_device[0]] = _storagepaths;
    }

    // stream
    {
        stream_config_color.stream_type = RS2_STREAM_COLOR;
        stream_config_color.width = width();
        stream_config_color.height = height();
        stream_config_color.format = color_format();
        stream_config_color.framerate = fps();
        stream_config_depth.stream_type = RS2_STREAM_DEPTH;
        stream_config_depth.width = width();
        stream_config_depth.height = height();
        stream_config_depth.format = depth_format();
        stream_config_depth.framerate = fps();
    }
}

void rs2wrapper::initialize(const bool &enable_ir_emitter,
                            const bool &verbose)
{
    for (auto &&available_device : available_devices)
    {
        const std::string device_sn = available_device[0];
        device dev;
        print("Initializing RealSense devices " + std::string(device_sn), 0);

        // 1. pipeline
        rs2::pipeline pipe = initialize_pipeline(ctx);
        dev.pipeline = std::make_shared<rs2::pipeline>(pipe);

        // 2. configure
        configure_stream(device_sn, stream_config_color, stream_config_depth);
        rs2::config cfg = rs_cfg[device_sn];
        if (!network())
            cfg.enable_device(std::string(device_sn));
        bool check = cfg.can_resolve(pipe);
        if (check)
            print("'cfg' usable with 'pipeline' : True", 0);
        else
            print("'cfg' usable with 'pipeline' : False", 0);

        // 3. pipeline profile
        rs2::pipeline_profile profile = pipe.start(cfg);
        dev.pipeline_profile = std::make_shared<rs2::pipeline_profile>(profile);
        if (verbose)
            print("pipeline started...", 0);

        // 4. sensors
        std::vector<rs2::sensor> sensors = profile.get_device().query_sensors();
        for (auto &&sensor : sensors)
        {
            if (auto css = sensor.as<rs2::color_sensor>())
            {
                dev.color_sensor = std::make_shared<rs2::color_sensor>(css);
                print("color sensor available...", 0);
            }
            else if (auto dss = sensor.as<rs2::depth_stereo_sensor>())
            {
                dev.depth_sensor = std::make_shared<rs2::depth_stereo_sensor>(dss);
                print("depth sensor available...", 0);
            }
        }

        // 5. IR
        if (enable_ir_emitter)
        {
            if (dev.depth_sensor->supports(RS2_OPTION_EMITTER_ENABLED))
            {
                // TODO: add arg for this.
                dev.depth_sensor->set_option(RS2_OPTION_EMITTER_ENABLED, 1);
                print("ir emitter enabled...", 0);
            }
        }

        // 6. enabled devices
        enabled_devices[device_sn] = dev;
        print_camera_infos(dev.pipeline_profile);

        if (storagepaths_perdev.size() > 0)
            query_timestamp_mode(std::string(device_sn));

        print("Initialized RealSense devices " + std::string(device_sn), 0);
    }
}

void rs2wrapper::initial_flush(const int &num_frames)
{
    for (const auto &enabled_device : enabled_devices)
    {
        auto device_sn = enabled_device.first;
        auto dev = enabled_device.second;
        for (auto i = 0; i < num_frames; ++i)
            dev.pipeline->wait_for_frames();
        print("Flushed 30 initial frames...", 0);
    }
}

void rs2wrapper::step(const std::string &save_file_prefix)
{
    for (const auto &enabled_device : enabled_devices)
    {
        std::string device_sn = enabled_device.first;
        device dev = enabled_device.second;
        std::vector<rs2::stream_profile> streams = dev.pipeline_profile->get_streams();

        // Loop through the set of frames from the camera.
        rs2::frameset frameset;
        rs2_metadata_type color_timestamp = 0;
        rs2_metadata_type depth_timestamp = 0;

        if (dev.pipeline->poll_for_frames(&frameset))
        {
            if (frameset.size() == streams.size())
            {
                time(&global_timestamp);

                rs2::frameset aligned_frameset = align_to_color.process(frameset);
                for (auto &&stream : streams)
                {
                    if (stream.stream_type() == RS2_STREAM_COLOR)
                    {
                        rs2::frame aligned_frame = aligned_frameset.first_or_default(RS2_STREAM_COLOR);
                        save_stream(device_sn, aligned_frame, color_timestamp);
                    }
                    else if (stream.stream_type() == RS2_STREAM_DEPTH)
                    {
                        rs2::frame aligned_frame = aligned_frameset.first_or_default(RS2_STREAM_DEPTH);
                        save_stream(device_sn, aligned_frame, depth_timestamp);
                    }
                }

                save_timestamp(device_sn, color_timestamp, depth_timestamp);
            }
        }
    }
}

void rs2wrapper::save_calib()
{
    for (const auto &enabled_device : enabled_devices)
    {
        auto device_sn = enabled_device.first;
        auto dev = enabled_device.second;
        std::string csv_file = storagepaths_perdev[device_sn].calib + "/calib.csv";
        std::ofstream csv;
        csv.open(csv_file);

        // Intrinsics of color & depth frames
        rs2::stream_profile profile_color = dev.pipeline_profile->get_stream(RS2_STREAM_COLOR);
        rs2_intrinsics intr_color = profile_color.as<rs2::video_stream_profile>().get_intrinsics();
        // Fetch stream profile for depth stream
        // Downcast to video_stream_profile and fetch intrinsics
        rs2::stream_profile profile_depth = dev.pipeline_profile->get_stream(RS2_STREAM_DEPTH);
        rs2_intrinsics intr_depth = profile_depth.as<rs2::video_stream_profile>().get_intrinsics();

        // Extrinsic matrix from color sensor to Depth sensor
        rs2_extrinsics extr = profile_color.as<rs2::video_stream_profile>().get_extrinsics_to(profile_depth);

        // Write calibration data to json file
        csv << intr_color.width << ","
            << intr_color.height << ","
            << intr_color.ppx << ","
            << intr_color.ppy << ","
            << intr_color.fx << ","
            << intr_color.fy << ","
            << rs2_distortion_to_string(intr_color.model) << ",";
        for (auto &&value : intr_color.coeffs)
            csv << value << ",";
        csv << "\n";

        csv << intr_depth.width << ","
            << intr_depth.height << ","
            << intr_depth.ppx << ","
            << intr_depth.ppy << ","
            << intr_depth.fx << ","
            << intr_depth.fy << ","
            << rs2_distortion_to_string(intr_depth.model) << ",";
        for (auto &&value : intr_depth.coeffs)
            csv << value << ",";
        csv << "\n";

        for (auto &&value : extr.rotation)
            csv << value << ",";
        for (auto &&value : extr.translation)
            csv << value << ",";
        csv << "\n";

        std::vector<rs2::sensor> sensors =
            dev.pipeline_profile->get_device().query_sensors();
        for (auto &&sensor : sensors)
        {
            if (auto dss = sensor.as<rs2::depth_stereo_sensor>())
            {
                csv << dss.get_depth_scale() << ","
                    << dss.get_stereo_baseline() << ",\n";
            }
        }

        std::cout << "[INFO] : Saved camera calibration data..." << std::endl;
    }
}

/*******************************************************************************
 * rs2wrapper PRIVATAE FUNCTIONS
 ******************************************************************************/
rs2::pipeline rs2wrapper::initialize_pipeline(const std::shared_ptr<rs2::context> context)
{
    if (network())
    {
        rs2::pipeline pipe(*ctx);
        return pipe;
    }
    else
    {
        rs2::pipeline pipe;
        return pipe;
    }
}

void rs2wrapper::query_timestamp_mode(const std::string &device_sn)
{
    for (auto &&frame : enabled_devices[device_sn].pipeline->wait_for_frames())
    {
        if (auto vf = frame.as<rs2::video_frame>())
        {
            if (vf.supports_frame_metadata(RS2_FRAME_METADATA_SENSOR_TIMESTAMP))
                timestamp_mode = RS2_FRAME_METADATA_SENSOR_TIMESTAMP;
            else if (vf.supports_frame_metadata(RS2_FRAME_METADATA_FRAME_TIMESTAMP))
                timestamp_mode = RS2_FRAME_METADATA_FRAME_TIMESTAMP;
            else if (vf.supports_frame_metadata(RS2_FRAME_METADATA_TIME_OF_ARRIVAL))
                timestamp_mode = RS2_FRAME_METADATA_FRAME_TIMESTAMP;
            break;
        }
    }
}

void rs2wrapper::save_timestamp(const std::string &device_sn,
                                const rs2_metadata_type &color_timestamp,
                                const rs2_metadata_type &depth_timestamp)
{
    auto save_path = storagepaths_perdev[device_sn].timestamp + "/timestamp.txt";
    std::fstream filestream;
    filestream.open(save_path, std::fstream::in | std::fstream::out | std::fstream::app);
    // If file does not exist, Create new file
    if (!filestream)
    {
        filestream.open(save_path, std::fstream::in | std::fstream::out | std::fstream::trunc);
    }
    filestream << global_timestamp
               << "::"
               << color_timestamp
               << "::"
               << depth_timestamp
               << "\n";
    filestream.close();
}

void rs2wrapper::configure_stream(const std::string &device_sn,
                                  const stream_config &stream_config_color,
                                  const stream_config &stream_config_depth)
{
    rs2::config cfg;
    cfg.enable_stream(stream_config_color.stream_type,
                      stream_config_color.width,
                      stream_config_color.height,
                      stream_config_color.format,
                      stream_config_color.framerate);
    cfg.enable_stream(stream_config_depth.stream_type,
                      stream_config_depth.width,
                      stream_config_depth.height,
                      stream_config_depth.format,
                      stream_config_depth.framerate);
    rs_cfg[device_sn] = cfg;
}

void rs2wrapper::save_stream(const std::string &device_sn,
                             const rs2::frame &frame,
                             rs2_metadata_type &timestamp)
{
    timestamp = frame.get_frame_metadata(timestamp_mode);
    std::string filename = pad_zeros(std::to_string(timestamp), 12);
    // Record per-frame metadata for UVC streams
    std::string csv_file = storagepaths_perdev[device_sn].depth_metadata + "/" + filename + ".csv";
    metadata_to_csv(frame, csv_file);
    // Write images to disk
    std::string png_file = storagepaths_perdev[device_sn].depth + "/" + filename + ".bin";
    framedata_to_bin(frame, png_file);
}

void rs2wrapper::print_camera_infos(const std::shared_ptr<rs2::pipeline_profile> profile)
{
    std::cout << "========================================" << std::endl;
    std::cout << ">>>>> RS2_CAMERA_INFO <<<<<" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << "Name          : " << profile->get_device().get_info(RS2_CAMERA_INFO_NAME) << std::endl;
    std::cout << "Serial Number : " << profile->get_device().get_info(RS2_CAMERA_INFO_SERIAL_NUMBER) << std::endl;
    try
    {
        std::cout << "Product Line  : " << profile->get_device().get_info(RS2_CAMERA_INFO_PRODUCT_LINE) << std::endl;
    }
    catch (const rs2::error &e)
    {
        std::cout << "not available, " << e.what() << std::endl;
    }
    try
    {
        std::cout << "Firmware      : " << profile->get_device().get_info(RS2_CAMERA_INFO_FIRMWARE_VERSION) << std::endl;
    }
    catch (const rs2::error &e)
    {
        std::cout << "not available, " << e.what() << std::endl;
    }
    try
    {
        std::cout << "USB type      : " << profile->get_device().get_info(RS2_CAMERA_INFO_USB_TYPE_DESCRIPTOR) << std::endl;
    }
    catch (const rs2::error &e)
    {
        std::cout << "not available, " << e.what() << std::endl;
    }
    std::cout << "========================================" << std::endl;
}

void rs2wrapper::print_camera_temperature(const std::string &device_sn)
{
    {
        auto dss = enabled_devices[device_sn].depth_sensor;
        if (dss->supports(RS2_OPTION_ASIC_TEMPERATURE))
        {
            auto temp = dss->get_option(RS2_OPTION_ASIC_TEMPERATURE);
            print("{device_sn} Temperature ASIC      : " + std::to_string(temp), 0);
        }
        if (dss->supports(RS2_OPTION_PROJECTOR_TEMPERATURE))
        {
            auto temp = dss->get_option(RS2_OPTION_PROJECTOR_TEMPERATURE);
            print("{device_sn} Temperature Projector : " + std::to_string(temp), 0);
        }
    }
}
