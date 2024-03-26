from pros_library_py.pros_node import *

"""
  Make sure you have built all the packages that are dpenedent.
  e.g. "system_interfaces"
  If you want to use other packages, you need to include them by yourself.
  After include the packages, you have to follow the steps provided by ROS2 to add dependencies in "setup.py" and "package.xml"
  Wish you have a good coding experience!!
""" 

class """ ClassName """(ProsNode):
    def __init__(self, node_name, **kwargs):
        super().__init__(node_name, **kwargs)
        """
        TODO: your variables
        """
        super().trigger_configure()
        super().trigger_activate()
        


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
    node = """ ClassName """(""" NodeName """)
    run(node)


if __name__ == '__main__':
    main()
