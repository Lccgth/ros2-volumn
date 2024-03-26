/*
* There will be several nodes be executed in the watchdog file
* Because we will do polling in period
* And polling will take more resource and time
* We make this function to another node
*/

// TODO: 認證

#include <memory>
#include <iostream>
#include <chrono>
#include <string>
#include <unordered_map>
#include <vector>

#include "rclcpp/rclcpp.hpp"
#include "rcl_interfaces/msg/log.hpp"
#include "system_interfaces/msg/node_change.hpp"
#include "system_interfaces/msg/check_node.hpp"

using namespace std::chrono_literals;


// Enumeration class for logging level
enum class LOG {
    INFO, DEBUG, ERROR, WARN, FATAL, UNKNOWN
};

// A hash table to store all nodes' last information timestamp 
std::unordered_map<std::string, std::pair<int32_t, uint32_t>> node_table;

// The nodes exist in current ros2 system
std::vector<std::string> node_list;

// White list of the system
std::unordered_map<std::string, int> white_list; 

class Watchdog : public rclcpp::Node
{
    private:
        // The subscription of ROS2 logging information
        rclcpp::Subscription<rcl_interfaces::msg::Log>::SharedPtr log_sub_;

        // The subscription of nodes' still alive message
        rclcpp::Subscription<system_interfaces::msg::CheckNode>::SharedPtr check_sub_;
        
        // The publisher will inform the node what should they do when error occurs
        rclcpp::Publisher<system_interfaces::msg::NodeChange>::SharedPtr publisher_;

        // If the message's level is ERROR or WARN or FATAL
        // We will send the corresponding to the node
        void check_message_level(const rcl_interfaces::msg::Log &msg) {
            std::string node_name = "/" + msg.name;
            LOG level = get_message_level(msg.level);

            switch (level) {
                case LOG::ERROR:
                    inform_node(node_name, "deactivate");
                    break;
                case LOG::WARN:
                    inform_node(node_name, "cleanup");
                    break;
                case LOG::FATAL:
                    inform_node(node_name, "shutdown");
                    break;
                case LOG::UNKNOWN:
                    RCLCPP_INFO_STREAM(this->get_logger(), "[Watchdog log] : " << node_name << " sends a message with \"UNKNOWN\" type");
                    break;
                default:
                    // We will not handle INFO and DEBUG level
                    break;
            }
        }

        // When new message comes in, update the table information according to the node name
        // And the time when messages arrived.
        void update_table(const system_interfaces::msg::CheckNode & msg) {
            std::string node_name = "/" + msg.node_name;
            int32_t sec = msg.seconds;
            uint32_t nsec = msg.nanoseconds;
            // RCLCPP_INFO_STREAM(this->get_logger(), "update_table(): " << node_name << " " << sec << " " << nsec);

            // If the time of the message's time is -1
            // It means the node has invoked the "on_shutdown()" function
            // So we will delete the node from the hash table
            if (sec == 0 && nsec == 0) {
                if (node_table.count(node_name) != 0) {
                    node_table.erase(node_name);
                    white_list.erase(node_name + "_monitor");
                    return;
                }
                RCLCPP_INFO_STREAM(this->get_logger(), "The node [" << node_name << "] has occurred some error of time");
                return;
            }

            // If the node is new to the system, insert it and then return
            if (node_table.count(node_name) == 0) {
                RCLCPP_INFO_STREAM(this->get_logger(), "A new node : [" << node_name << "] has been added to the system");
                node_table[node_name] = {sec, nsec};
                white_list[node_name + "_monitor"] = 1;
                return;
            }

            // Update the hash table
            node_table[node_name] = {sec, nsec};
            
            // Calculate the difference of the node's new message arrival time and its last message arrival time
            int32_t diff_time = sec - node_table[node_name].first;
            if (diff_time >= 10) {
                inform_node(node_name, "deactivate");
            }
        }

        // Convert level's type from uint8_t to LOG enum class
        LOG get_message_level(const uint8_t log_level) {
            int levelcode = static_cast<int>(reinterpret_cast<std::uint8_t>(log_level));
            switch (levelcode)
            {
                case 10:
                    return LOG::DEBUG;
                    break;
                case 20:
                    return LOG::INFO;
                    break;
                case 30:
                    return LOG::WARN;
                    break;
                case 40:
                    return LOG::ERROR;
                    break;
                case 50:
                    return LOG::FATAL;
                    break;
                default:
                    return LOG::UNKNOWN;
            }
        }

        // Publish the message to the node who has error
        void inform_node(std::string node_name, std::string operation) {
            auto msg = system_interfaces::msg::NodeChange();
            msg.node_name = node_name;
            msg.state = operation;
            RCLCPP_INFO_STREAM(this->get_logger(), "Node name : " << node_name << " Target state : " << operation);
            publisher_->publish(msg);
        }

    public:
        Watchdog() : Node("watchdog") {
            log_sub_ = this->create_subscription<rcl_interfaces::msg::Log>("/rosout", 10, std::bind(&Watchdog::check_message_level, this, std::placeholders::_1));
            check_sub_ = this->create_subscription<system_interfaces::msg::CheckNode>("check_node", 10, std::bind(&Watchdog::update_table, this, std::placeholders::_1));
            publisher_ = this->create_publisher<system_interfaces::msg::NodeChange>("node_change", 10);
            RCLCPP_INFO_STREAM(this->get_logger(), "Watchdog is activating now!");
        }
};

class PollingNode : public rclcpp::Node
{
    private:
        // Publish the message to the node who occurred error
        rclcpp::Publisher<system_interfaces::msg::NodeChange>::SharedPtr node_change_pub_;

        // The timer will poll the table every 10 seconds
        rclcpp::TimerBase::SharedPtr polling_timer_;

        // Same as watchdog's function
        // Which is used to send message to the node
        void inform_node(std::string node_name, std::string operation) {
            auto msg = system_interfaces::msg::NodeChange();
            msg.node_name = node_name;
            msg.state = operation;
            RCLCPP_INFO_STREAM(this->get_logger(), "Response Time Out [" << node_name << "]: Target state : " << operation);
            node_change_pub_->publish(msg);
        }

        // Polling the table every 10 seconds
        void polling_table() {
            RCLCPP_INFO_STREAM(this->get_logger(), "polling node table");
            
            for (auto const & node_info : node_table) {
                std::string node_name = node_info.first;
                int32_t node_sec = node_info.second.first;
                int32_t cur_sec = this->now().seconds();

                if (cur_sec - node_sec >= 10) {
                    RCLCPP_INFO_STREAM(this->get_logger(), "[" << node_name << "] already time out");
                    inform_node(node_name, "deactivate");
                } else if (cur_sec - node_sec >= 60) {
                    RCLCPP_INFO_STREAM(this->get_logger(), "[" << node_name << "] may occur some fatal errors"); 
                    inform_node(node_name + "_monitor", "shutdown");
                }
            }
        }

    public:
        PollingNode() : Node("watchdog_polling_node") {
            // Initialize the variables
            node_change_pub_ = this->create_publisher<system_interfaces::msg::NodeChange>("node_change", 10);
            polling_timer_ = this->create_wall_timer(10s, std::bind(&PollingNode::polling_table, this));
        }
};

class CheckSystem : public rclcpp::Node
{
    private:
        // Check the list every 5 seconds
        rclcpp::TimerBase::SharedPtr check_list_timer_;

        // Check the table every 5 seconds
        rclcpp::TimerBase::SharedPtr check_table_timer_;

        // A temp space that stores ros2 node list
        std::unordered_map<std::string, int> nodes;

        // A publisher to inform node monitor
        rclcpp::Publisher<system_interfaces::msg::NodeChange>::SharedPtr kill_pub_;

        // Check if there are nodes has been deleted but still existing in the node list
        void check_list() {
            node_list = rclcpp::Node::get_node_names();
            nodes.clear();
            RCLCPP_INFO_STREAM(this->get_logger(), "periodically check system node list.");

            for (auto const & name : node_list) {
                nodes[name] = 1;
            }

            for (auto const & node : nodes) {
                std::string node_name = node.first;
                if (node_name[1] == '_') continue;
                
                if (node_table.count(node_name) == 0 && white_list.count(node_name) == 0) {
                    RCLCPP_INFO_STREAM(this->get_logger(), "remove [" << node_name << "] from \"node list\" because of not existing in system \"node table\".");
                    
                    // inform node
                    inform_node(node_name + "_monitor", "shutdown");
                }
            }
        }

        // Remove nodes that are not deleted from the table but has already been deleted from system
        void check_table() {
            node_list = rclcpp::Node::get_node_names();
            nodes.clear();
            RCLCPP_INFO_STREAM(this->get_logger(), "periodically check system node table.");

            for (auto const & name : node_list) {
                nodes[name] = 1;
            }

            for (auto const & node : node_table) {
                std::string node_name = node.first;
                if (node_name[1] == '_') continue;

                if (nodes.count(node_name) == 0 && white_list.count(node_name) == 0) {
                    RCLCPP_INFO_STREAM(this->get_logger(), "remove [" << node_name << "] from '\"node table\" because of not existing in system \"node list\".");
                    node_table.erase(node_name);
                }
            }
        }

        void inform_node(std::string node_name, std::string operation) {
            auto msg = system_interfaces::msg::NodeChange();
            msg.node_name = node_name;
            msg.state = operation;
            kill_pub_->publish(msg);
        }

    public:
        CheckSystem() : Node("watchdog_check_system_node") {
            check_list_timer_ = this->create_wall_timer(5s, std::bind(&CheckSystem::check_list, this));
            check_table_timer_ = this->create_wall_timer(5s, std::bind(&CheckSystem::check_table, this));
            kill_pub_ = this->create_publisher<system_interfaces::msg::NodeChange>("node_change", 10);
        }
};

int main(int argc, char * argv[])
{
    rclcpp::init(argc, argv);
    rclcpp::executors::MultiThreadedExecutor exe;

    // Split polling function to another node
    // To reduce loading of watchdog node
    std::shared_ptr<Watchdog> watchdog = std::make_shared<Watchdog>();
    std::shared_ptr<PollingNode> polling_node = std::make_shared<PollingNode>();
    std::shared_ptr<CheckSystem> check_system_node = std::make_shared<CheckSystem>();
    
    // Add nodes to white list
    white_list["/watchdog"] = 1;
    white_list["/watchdog_polling_node"] = 1;
    white_list["/watchdog_check_system_node"] = 1;

    // Add watchdog nodes and spin
    exe.add_node(watchdog);
    exe.add_node(polling_node);
    exe.add_node(check_system_node);
    exe.spin();

    rclcpp::shutdown();
    return 0;
}
