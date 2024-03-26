#include <memory>

#include "rclcpp/rclcpp.hpp"
#include "std_msgs/msg/string.hpp"
#include "rcutils/logging_macros.h"
#include "rcl_interfaces/msg/log.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <time.h>

using namespace std;


class Log : public rclcpp::Node {

public:
    Log(fstream& file);

private:
    string current_time();
    string get_log_level(const uint8_t log_level);
    void log_call_back(const rcl_interfaces::msg::Log &log_msg);

    fstream& log_file_;
    vector<string> nodes;
    rclcpp::Subscription<rcl_interfaces::msg::Log>::SharedPtr subscription_;
};


int main(int argc, char * argv[]) {
    rclcpp::init(argc, argv);
    fstream file;
    file.open("log_system.csv", ios::in | ios_base::app);
    rclcpp::spin(make_shared<Log>(file));
    rclcpp::shutdown();
    file << "===Close Logging===" << "\n";
    file.close();
    return 0;
}

/*************/
/* Log class */ 
/*************/

Log::Log(fstream& file) : Node("syslog"), log_file_(file) {
    subscription_ = this->create_subscription<rcl_interfaces::msg::Log>("/rosout", 10, bind(&Log::log_call_back, this, placeholders::_1));
}

string Log::current_time() {
    time_t now = time(0);
    time_t taipeiTime = now + 8 * 3600; 
    char buf[80];
    struct tm* taipeiTstruct = gmtime(&taipeiTime);
    strftime(buf, sizeof(buf), "%Y-%m-%d.%X", taipeiTstruct);
    return buf;
}

string Log::get_log_level(const uint8_t log_level) {
    int levelcode = static_cast<int>(reinterpret_cast<uint8_t>(log_level));
    switch (levelcode) {
        case 10:
            return "DEBUG";
            break;
        case 20:
            return "INFO";
            break;
        case 30:
            return "WARN";
            break;
        case 40:
            return "ERROR";
            break;
        case 50:
            return "FATAL";
            break;
        default:
            return "UNKNOWN";
    }
}

void Log::log_call_back(const rcl_interfaces::msg::Log &log_msg) {
    cout << "Received log message from node [" << log_msg.name.c_str() << "]: " << log_msg.msg.c_str() << "\n";

    // Get node.name index
    int index;
    auto it = find(nodes.begin(), nodes.end(), log_msg.name);
    if (it != nodes.end()){
        index = distance(nodes.begin(), it);
    }
    else{
        nodes.push_back(log_msg.name);
        index = nodes.size() - 1;
    }

    // Logging
    log_file_ << current_time();
    for(int i = 0; i < index + 1; i++)
        log_file_ << ",";
    log_file_ << "[" << get_log_level(log_msg.level) << "]" << "[" << log_msg.name << "]" << log_msg.msg.c_str() << "\n";

}

