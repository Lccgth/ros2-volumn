FROM public.ecr.aws/paia-tech/ros2-humble:dev-20230907
ENV ROS2_WS /home/PROS-Unity
ENV ROS_DOMAIN_ID=1

WORKDIR ${ROS2_WS}
RUN apt install ros-humble-rosbridge-server
RUN . /opt/ros/humble/setup.sh && \
    colcon build --packages-select system_interfaces pros_library monitor_system

RUN echo "source ${ROS2_WS}/install/setup.bash " >> /.bashrc 
RUN echo "source /.bashrc" >> ~/.bashrc 

CMD ["bash","-l"]