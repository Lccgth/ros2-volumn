from pros_library.pros_node import *
from std_msgs.msg import Float32MultiArray
import rclpy
from rclpy.node import Node

class CenterNode(ProsNode):
    def __init__(self, node_name, **kwargs):
        super().__init__(node_name, **kwargs)
        """
        TODO: your variables
        """
        super().trigger_configure()
        super().trigger_activate()
        self.subsvriber_ = self.create_subscription(Float32MultiArray, "unity2Ros", self.callback_robot_news, 10)
        self.get_logger().info("Data receive from unity")
        self.publisher_ros2unity = self.create_publisher(Float32MultiArray,"ros2Unity", 10)
        # self.timer_ = self.create_timer(0.5, self.publish_news)
        self.get_logger().info("Unity publisher")

        self.publisher_ros2Ai = self.create_publisher(Float32MultiArray, 'ros2Ai', 10)

        self.subscriber_fromAi = self.create_subscription(Float32MultiArray, "Ai2ros", self.callback_fromAi, 10)

    def callback_fromAi(self, msg):
        self.aiState = msg.data
        self.ros2unity_msg = Float32MultiArray()
        self.ros2unity_msg.data = self.aiState
        print(self.ros2unity_msg.data)
        self.publisher_ros2unity.publish(self.ros2unity_msg)


    def callback_robot_news(self, msg):
        self.unityState = msg.data
        self.ros2Ai_msg = Float32MultiArray()
        self.ros2Ai_msg.data = self.unityState
        self.publisher_ros2Ai.publish(self.ros2Ai_msg)


    def on_configure(self, state: State):
        self.get_logger().info("on_configure() is called.")
        """
        TODO: configure
        """
        return TransitionCallbackReturn.SUCCESS

    def on_activate(self, state: State):
        self.get_logger().info("on_activate() is called.")
        """
        TODO: What will be executed when node activating.
        """
        return super().on_activate(state)

    def on_deactivate(self, state: State):
        self.get_logger().info("on_deactivate() is called.")
        """
        TODO: Turn off/Reset the operations when activating.
        """
        return super().on_deactivate(state)

    def on_cleanup(self, state: State):
        self.get_logger().info('on_cleanup() is called.')
        """
        TODO: Clean all the variables.
        """
        return TransitionCallbackReturn.SUCCESS 

def main():
    rclpy.init()
    node = CenterNode("center")
    run(node)


if __name__ == '__main__':
    main()
