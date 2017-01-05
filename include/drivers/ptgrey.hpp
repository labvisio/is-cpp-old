#ifndef __IS_DRIVER_PTGREY_HPP__
#define __IS_DRIVER_PTGREY_HPP__

#include "FlyCapture2.h"
#include <opencv2/core/core.hpp>
#include <opencv2/highgui/highgui.hpp>
#include <iostream>
#include <cstdint>
#include <vector>
#include <string>
#include <sstream>
#include <iomanip>
#include <thread>
#include <chrono>
#include <atomic>
#include <algorithm>
#include <memory>
#include <tuple>
#include <stdexcept>
#include <mutex>
#include <thread>
#include "../msgs/camera.hpp"

namespace is {
namespace driver {

using namespace std::chrono_literals;
using namespace is::msg::camera;
using namespace FlyCapture2;

struct PtGrey {
    GigECamera	camera;
	PGRGuid* handle;
    
    PtGrey(std::string ip_address) { 
        get_handle(ip_address, handle);
    }

    void set_fps(double fps) {

    }

    double get_fps() {

    }

    double get_max_fps() {

    }

    void set_resolution(Resolution resolution) {

    }

    Resolution get_resolution() {

    }

    void set_image_type(ImageType image_type) {
        
    }

    ImageType get_image_type() { return ImageType::RGB; }

    cv::Mat get_frame() {

    }

    /*-------------------------------------------------------------------*/
     void get_handle(const std::string& ip_address, PRGuid* guid) {
        BusManager bus;
        Error error;
        int ip_int = ip_to_int(ip_address.c_str());
        if (!ip_int) {
            throw std::runtime_error("Invalid IP address.");
        }
        IPAddress ip(ip_int);
        error = bus.GetCameraFromIPAddress(ip, guid);
        if (error != PGRERROR_OK) {
            error.PrintErrorTrace();
            throw std::runtime_error("Camera not found.");
        }
        unique_ptr<PointGrey> camera(new PointGrey(guid));
        return camera;
    }

    std::vector<std::string> split(const std::string &s, char delim) {
        std::stringstream ss(s);
        std::string item;
        std::vector<std::string> elems;
        while (std::getline(ss, item, delim)) {
            elems.push_back(item);
        }
        return elems;
    }

    int ip_to_int(const char* ip) {
	    std::string data(ip);
	    auto split_data = split(data,'.');
	    int IP = 0;
	    if (split_data.size()==4) {
	    	int sl_num = 24;
	    	int mask = 0xff000000;
	    	for ( auto& d : split_data) {
	    		int octets = stoi(d); 
	    		if ( ( octets >> 8) && 0xffffff00 ) {
	    			return 0; //error
	    		}
	    		IP += (octets << sl_num);
	    		sl_num -= 8;
	    	}
	    	return IP;
	    }
	    return 0;
    }

}

};

}  // ::driver
}  // ::is


#endif  // __IS_DRIVER_PTGREY_HPP__ 