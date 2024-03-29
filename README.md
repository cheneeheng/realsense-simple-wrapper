# RealSense Wrapper

This is a repository to run realsense camera devices (RS) using [Realsense SDK](https://github.com/IntelRealSense/librealsense).

## Installing RS SDK in linux

The librealsense SDK can be obtained by either:
1. compiling from [source](https://github.com/IntelRealSense/librealsense/blob/master/doc/installation.md). Specific flags are needed to install the python wrappers and network cpability (refer [here](docker/realsense/dockerfiles/Dockerfile.Ubuntu20)).

2. installing the python wrapper/package to run RS can be obtained through [pip](https://pypi.org/project/pyrealsense/).

3. installing the [pre-build packages](https://github.com/IntelRealSense/librealsense/blob/master/doc/distribution_linux.md).

## Running RS in python

The code in [realsense](realsense) folder uses the [pyrealsense2](https://pypi.org/project/pyrealsense/) python package to stream images/frames from realsense devices.

| File                                                             | Details                                                                                             |
| :--------------------------------------------------------------- | :-------------------------------------------------------------------------------------------------- |
| [realsense_run_devices.py](rs_py/realsense_run_devices.py)       | Runs X realsense devices that are connected to the PC.                                              |
| [realsense_wrapper.py](rs_py/realsense_wrapper.py)               | Contains the _RealsenseWrapper_ class that contains all the functions to run the realsense devices. |
| [realsense_device_manager.py](rs_py/realsense_device_manager.py) | Helper functions from the official realsense repo.                                                  |
|                                                                  |                                                                                                     |

## Running RS in RaspberryPi-4B

See [here](pi4_scripts/README.md).

## Data transmission testing

| Setting                    | C Read | C Write | D Read |    D Write    |  FPS  | Status                                 |
| :------------------------- | :----: | :-----: | :----: | :-----------: | :---: | :------------------------------------- |
| pyrealsense2_net 640x480   |   X    |    X    |   X    |       X       |   6   | OK                                     |
| pyrealsense2_net 640x480   |   X    |    X    |   X    |       X       |  15   | OK                                     |
| pyrealsense2_net 640x480   |   X    |    X    |   X    |       X       |  30   | Fail, randomly jitters between 5-15fps |
| realsense2-net c++ 640x480 |   X    |    X    |   X    | X (+colormap) |   6   | OK                                     |
| realsense2-net c++ 640x480 |   X    |    X    |   X    | X (+colormap) |  15   | Fail, randomly jitters between 5fps    |
| realsense2-net c++ 640x480 |   X    |    X    |   X    | X (+colormap) |  30   | Fail, randomly jitters between 5fps    |
| realsense2-net c++ 640x480 |   X    |    X    |   X    |       X       |   6   | OK                                     |
| realsense2-net c++ 640x480 |   X    |    X    |   X    |       X       |  15   | OK                                     |
| realsense2-net c++ 640x480 |   X    |    X    |   X    |       X       |  30   | Fail, randomly jitters between 5-15fps |
| usbip wifi 640x480         |   X    |    X    |   X    |               |   6   | Fail, randomly drops 1-2 frames        |
| usbip wifi 640x480         |   X    |    X    |   X    |               |  15   | Fail, randomly jitters between 2-6fps  |

## UVC-stream stuck at pipe.start()
It is possible that the request sent by the messenger returns error during start. This is caused by libusb having trouble communicating with the usb port. The error combined with `invoke_and_wait` in `concurrency.h` will cause the program to get stuck during pipe.start(). A hack is to keep sending request until no error is returned.

```
# librealsense/src/uvc/uvc-streamer.cpp:l151

void uvc_streamer::start()
    {
        _action_dispatcher.invoke_and_wait([this](dispatcher::cancellable_timer c)
        {
            if(_running)
                return;
            
            bool __reset_request = true;
            while(__reset_request)
            {
            __reset_request = false;
            
            _context.messenger->reset_endpoint(_read_endpoint, RS2_USB_ENDPOINT_DIRECTION_READ);
            
            {
                std::lock_guard<std::mutex> lock(_running_mutex);
                _running = true;
            }

            for(auto&& r : _requests)
            {
                auto sts = _context.messenger->submit_request(r);
                if(sts != platform::RS2_USB_STATUS_SUCCESS)
                    __reset_request = true;
                    // throw std::runtime_error("failed to submit UVC request while start streaming");
                    
            }
            
            if(__reset_request)
                for(auto&& r : _requests)
                    _context.messenger->cancel_request(r); 
            }
            
            _publish_frame_thread->start();

        }, [this](){ return _running; });
    }
```

## Good to know

### 1. [Sensor timestamp](https://github.com/IntelRealSense/librealsense/issues/2188)
- SENSOR_TIMESTAMP: Device clock / sensor starts taking the image
- FRAME_TIMESTAMP: Device clock / frame starts to transfer to the driver
- BACKEND_TIMESTAMP: PC clock / frame starts to transfer to the driver (HW->Kernel transition)
- TIME_OF_ARRIVAL: PC clock / frame is transfered to the driver (kernel->user space transition)

There is an [issue](https://github.com/IntelRealSense/librealsense/issues/7972) with arm-based chips, i.e. pi4, where the backend_timestamp cannot be obtained.

The clocks for depth and color sensors are different. The frameset tries to map the closest frames together. See [here](https://github.com/IntelRealSense/librealsense/issues/1548).

### 2. [HW Sync](https://dev.intelrealsense.com/docs/external-synchronization-of-intel-realsense-depth-cameras)
The realsense can be syncroized using an external sync cable so that the cameras captures at the exact same timestamp.

### 3. Streaming using *rs-server* and viewing with *realsense-viewer*
The command `rs-server` needs to be run on the host connected to a RS to stream frames from the RS. The RS library on the host server needs to be compiled with `-DBUILD_NETWORK_DEVICE=ON` [flag](https://github.com/IntelRealSense/librealsense/issues/7123) in order for the `rs-server` to work.

Once the `rs-server` on the host is running `realsense-viewer` can be used to view the stream on a client that is connected to the host in the same network. Currently there is a [bug](https://github.com/IntelRealSense/librealsense/issues/9971) with SDK-V2.5 where the depth values are not streamed properly and stream settings in the realsense-viewer GUI cannot be changed (keeps crashing). [SDK-V2.47](https://github.com/IntelRealSense/librealsense/releases/tag/v2.47.0https://github.com/IntelRealSense/librealsense/releases/tag/v2.47.0) can be used instead.

There is an official [guide](https://dev.intelrealsense.com/docs/open-source-ethernet-networking-for-intel-realsense-depth-cameras) on open source ethernet networking to stream RS over a network. The following service (taken from the raspberrypi image in the guide) enables rs-server to run constantly.

```
# /lib/systemd/system/rs-server.service

[Unit]
Description=Intel RealSense Camera Service
After=network-online.target

[Service]
ExecStart=/bin/bash /home/realsense/run-server.sh
WorkingDirectory=/home/realsense/librealsense/build
StandardOutput=inherit
StandardError=inherit
Restart=always
User=realsense

[Install]
WantedBy=multi-user.target
```

### 4. `-DFORCE_LIBUVC=true` vs `-DFORCE_RSUSB_BACKEND=true`
The former is an old flag of the latter one. See [here](https://github.com/IntelRealSense/librealsense/issues/7144).

### 5. Metadata extraction
The extraction of metadata is supported in windows but requires kernel patching for linux OS. See [here](https://github.com/IntelRealSense/librealsense/blob/master/doc/frame_metadata.md#os-support) and [here](https://github.com/IntelRealSense/librealsense/issues/7039).

### 6. Bug in align for python with rs_net
When align is used with rs_net device, the resulting depth images is all zeros.

### 7. Depthmap color visualization
The rs2::colorizer class performs histogram equalization and thus performs differently compared to the colorizing example using opencv. See [here](https://github.com/IntelRealSense/librealsense/issues/1310).

### 8. Depth filter example
See [here](https://github.com/IntelRealSense/librealsense/blob/jupyter/notebooks/depth_filters.ipynb). 

### 9. Multi IP connection using realsense_net is not supported
See [here](https://github.com/IntelRealSense/librealsense/issues/6376)

### 10. Auto exposure
Setting the auto exposure manually may cause drop in FPS. See [here](https://github.com/IntelRealSense/librealsense/issues/1957).

The `RS2_OPTION_AUTO_EXPOSURE_LIMIT` options require stream to restart. Unit in mirco sec. See [here](https://intelrealsense.github.io/librealsense/doxygen/rs__option_8h.html).

### 11. Disparity
The disparity is given with 1/32 pixel resolution. See [here](https://github.com/IntelRealSense/librealsense/issues/3039) and [here](https://github.com/IntelRealSense/librealsense/issues/3344).

### 12. frame alignment
The rs.align function is slow. Compile it with cuda to make it faster. Uses about 100mb gpu memory for 30fs 848x480 color + depth stream.