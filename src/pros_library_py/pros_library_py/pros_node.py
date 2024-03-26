import rclpy
import threading
import sys
from rclpy.lifecycle import Node
from rclpy.lifecycle import Publisher
from rclpy.lifecycle import State
from rclpy.lifecycle import LifecycleNode
from rclpy.lifecycle import TransitionCallbackReturn
from rclpy.timer import Timer
from system_interfaces.msg import CheckNode
from system_interfaces.msg import NodeChange



class ProsThread(LifecycleNode):
    def __init__(self, node_name, **kwargs):
        super().__init__(node_name, **kwargs)
        self.node_name = node_name
        self.node_change_sub_ = self.create_subscription(NodeChange, 'node_change', self.listen_on_change, 10)
        self.node_change_sub_
        self.alive_pub_ = self.create_publisher(CheckNode, 'check_node', 10)
        self.alive_timer_ = self.create_timer(1, self.check_alive)

    def get_node_name(self):
        return self.node_name
    
    def listen_on_change(self, msg):
        temp_node_name = msg.node_name[1:]

        if temp_node_name != self.node_name:
            return

        current_state_id = self.get_current_state().id
        if msg.state == "cleanup" and current_state_id == State.PRIMARY_STATE_INACTIVE:
            self.cleanup()
            self.configure()
            self.activate()
        elif msg.state == "deactivate" and current_state_id == State.PRIMARY_STATE_ACTIVE:
            self.deactivate()
            self.activate()
        elif msg.state == "cleanup" and current_state_id == State.PRIMARY_STATE_ACTIVE:
            self.deactivate()
            self.cleanup()
        elif msg.state == "shutdown":
            self.shutdown()
            rclpy.shutdown()


    def check_alive(self):
        msg = CheckNode()
        sec, nsec = self.get_clock().now().seconds_nanoseconds()
        msg.node_name = self.node_name
        msg.seconds = sec
        msg.nanoseconds = nsec
        self.alive_pub_.publish(msg)
    
    def trigger_configure(self):
        super().trigger_configure()
        
    def trigger_activate(self):
        super().trigger_activate()
    
    
class MonitorThread(Node):
    def __init__(self, node_name, **kwargs):
        super().__init__(node_name, **kwargs)
        self.shutdown_sub_ = self.create_subscription(NodeChange, 'node_change', self.check_shutdown, 10)
        self.destroy_publisher_ = self.create_publisher(CheckNode, 'check_node', 10)
        self.node_name = node_name
    
    def check_shutdown(self, msg):
        temp_node_name = msg.node_name[1:]
        if temp_node_name == self.node_name and msg.state == "shutdown" :
            msg = CheckNode()
            pros_node_name = self.node_name[:-8]
            msg.node_name = pros_node_name
            msg.seconds = 0
            msg.nanoseconds = 0
            print(pros_node_name, "shutdown.")
            self.destroy_publisher_.publish(msg)
            rclpy.shutdown()

class ProsNode(ProsThread):
    def __init__(self, node_name, **kwargs):
        super().__init__(node_name, **kwargs)
        self.destroy_publisher_ = self.create_publisher(CheckNode, 'check_node', 10)
    
    def on_shutdown(self, state: State):
        msg = CheckNode()
        msg.seconds = 0
        msg.nanoseconds = 0
        self.destroy_publisher_.publish(msg)
        return TransitionCallbackReturn.SUCCESS

    def on_error(self, state: State):
        return TransitionCallbackReturn.SUCCESS
    


def spin_pros(node):
    exe = rclpy.executors.SingleThreadedExecutor()
    exe.add_node(node)
    exe.spin()
    rclpy.shutdown()

def spin_monitor(node_name):
    print("Ready to spin monitor node")
    exe = rclpy.executors.SingleThreadedExecutor()
    monitor_node = MonitorThread(node_name)
    exe.add_node(monitor_node)
    exe.spin()
    rclpy.shutdown()
    print("Monitor node is closed")

def run(node):
    node.get_logger().info('pros_thread run')
    pros = threading.Thread(target=spin_pros, args=(node,))
    pros.start()
    monitor_node_name = node.get_node_name() + "_monitor"
    spin_monitor(monitor_node_name)
    print("end all thread")
    sys.exit(0)


