from pros_library_py.pros_node import *
from system_interfaces.msg import YoloBoundingBox


class Yolo(ProsNode):
    def __init__(self, node_name, **kwargs):
        super().__init__(node_name, **kwargs)
       
        super().trigger_configure()
        super().trigger_activate()
        
    def on_configure(self, state: State):
        self.get_logger().info("on_configure() is called.")
        self.subscription_ = self.create_subscription(YoloBoundingBox, "yolo_bounding_box", self.callback, 10)
        return TransitionCallbackReturn.SUCCESS

    def on_activate(self, state: State):
        self.get_logger().info("on_activate() is called.")
        
        self.publisher_.publish()
        return super().on_activate(state)

    def on_deactivate(self, state: State):
        self.get_logger().info("on_deactivate() is called.")
        return super().on_deactivate(state)

    def on_cleanup(self, state: State):
        self.get_logger().info('on_cleanup() is called.')
        return TransitionCallbackReturn.SUCCESS 
    
    def callback(self, msg):
        self.get_logger().info("Get message")
    

def main():
    rclpy.init()
    node = Yolo("yolo_bounding_box")
    run(node)


if __name__ == '__main__':
    main()
