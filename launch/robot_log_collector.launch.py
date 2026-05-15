from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    params_file = LaunchConfiguration("params_file")

    default_params_file = PathJoinSubstitution(
        [
            FindPackageShare("robot_log_collector"),
            "config",
            "robot_log_collector.yaml",
        ]
    )

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "params_file",
                default_value=default_params_file,
                description="Path to the robot_log_collector parameter file.",
            ),
            Node(
                package="robot_log_collector",
                executable="robot_log_collector",
                name="robot_log_collector",
                output="screen",
                parameters=[params_file],
            ),
        ]
    )
