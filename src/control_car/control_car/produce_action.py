from pros_library_yp.pros_node import *
from std_msgs.msg import Float32MultiArray


class ProduceAction(ProsNode):
    def __init__(self, node_name, **kwargs):
        super().__init__(node_name, **kwargs)
       
        super().trigger_configure()
        super().trigger_activate()
        
    def on_configure(self, state: State):
        self.get_logger().info("on_configure() is called.")
        self.publisher_ = self.create_publisher(Float32MultiArray, "wheel_speed", 10)
        return TransitionCallbackReturn.SUCCESS

    def on_activate(self, state: State):
        self.get_logger().info("on_activate() is called.")
        while rclpy.ok():
            key = str(input("Press a key and press Enter: "))
            if key == "w":
                self.back_right = 2000
                self.back_left = 2000
                self.front_right = 2000
                self.front_left = 2000
            elif key == "s":
                self.back_right = -2000
                self.back_left = -2000
                self.front_right = -2000
                self.front_left = -2000
            elif key == "a":
                self.front_right = -2000
                self.front_left = 2000
                self.back_right = -2000
                self.back_left = 2000
            elif key == "d":
                self.front_right = 2000
                self.front_left = -2000
                self.back_right = 2000
                self.back_left = -2000
            else :
                self.back_right = 0
                self.back_left = 0
                self.front_right = 0
                self.front_left = 0
            data = [float(self.front_right), float(self.front_left), float(self.back_right), float(self.back_left)]
            self.publisher_.publish(Float32MultiArray(data=data))
        return super().on_activate(state)

    def on_deactivate(self, state: State):
        self.get_logger().info("on_deactivate() is called.")
        return super().on_deactivate(state)

    def on_cleanup(self, state: State):
        self.get_logger().info('on_cleanup() is called.')
        return TransitionCallbackReturn.SUCCESS 

def main():
    rclpy.init()
    node = ProduceAction("produce_action")
    run(node)


if __name__ == '__main__':
    main()
