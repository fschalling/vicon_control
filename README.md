# Vicon control

This repository includes:

* A GUI and command line tools to connect to and get frames from a computer running the propietary [Vicon Tracker](https://www.vicon.com/products/software/tracker)
* A GUI and command line tools to connect to and receive sensor data from a peer running the provided robot software
* Template software that can be used to close the control loop using Vicon Tracker data to create a reference for the robot
* Template software to be run on the robot which listens to a reference and sends sensor data

To achieve modularity of each package, several open source files have been added from third parties:

* *vicon_workspace/vicon_datastream_sdk*: Holds relevant headers and libraries from the [Vicon Datastream SDK](https://www.vicon.com/downloads/utilities-and-sdk/datastream-sdk)
* *robot_control/pru-cgt*: Holds binaries, headers and libraries generated by [Texas Instruments PRU Code Generation Tools](http://software-dl.ti.com/codegen/non-esd/downloads/download.htm#PRU)
* *robot_control/pru-icss*: Holds the PRU linker command file and relevant headers of the PRU-ICSS generated by the [Linux Processor SDK for AM335x](http://software-dl.ti.com/processor-sw/esd/PROCESSOR-SDK-LINUX-AM335X/latest/index_FDS.html)

## Target systems

The four components have been designed for ROS Lunar running on Ubuntu 16.04. The *robot_control* package has scripts and generates binaries and firmware to be used on a [BeagleBone Black](https://beagleboard.org/black)

# Contents
* [Overview](https://github.com/SjoerdKoop/vicon_control#overview)
* [Setup](https://github.com/SjoerdKoop/vicon_control#setup)
	* [Prerquisites](https://github.com/SjoerdKoop/vicon_control#prerequisites)
	* [Installation](https://github.com/SjoerdKoop/vicon_control#installation)
	* [Setting up the connection](https://github.com/SjoerdKoop/vicon_control#setting-up-the-connection)
	* [BeagleBone Black specific settings](https://github.com/SjoerdKoop/vicon_control#beaglebone-black-specific-settings)
* [Usage](https://github.com/SjoerdKoop/vicon_control#usage)
	* [ROS workspaces](https://github.com/SjoerdKoop/vicon_control#ros-workspaces)
	* [Robot control](https://github.com/SjoerdKoop/vicon_control#robot-control)
	* [Robot control design](https://github.com/SjoerdKoop/vicon_control#robot-control-design)
	* [Vision control design](https://github.com/SjoerdKoop/vicon_control#vision-control-design)
* [Communication](https://github.com/SjoerdKoop/vicon_control#communication)
	* [ROS: data_update](https://github.com/SjoerdKoop/vicon_control#ros-data_update)
	* [ROS: object_update](https://github.com/SjoerdKoop/vicon_control#ros-object_update)
	* [ROS: reference_update](https://github.com/SjoerdKoop/vicon_control#ros-reference_update)
	* [UDP: reference](https://github.com/SjoerdKoop/vicon_control#udp-reference)
	* [UDP: sensor data](https://github.com/SjoerdKoop/vicon_control#udp-sensor-data)
* [Executables](https://github.com/SjoerdKoop/vicon_control#executables)
* [Troubleshooting](https://github.com/SjoerdKoop/vicon_control#troubleshooting)

# Overview

The recommend setup scheme is shown below:

![General Scheme](/images/general_scheme.png)

In this scheme, the components are set up in a modular fashion so that components can be changed without influencing the others.

* The vicon software creates a client using the [Vicon datastream SDK](https://www.vicon.com/products/software/datastream-sdk) and generates an array of objects over the ROS topic [*object_update*](https://github.com/SjoerdKoop/vicon_control#ros-object_update). A replacement would only have to satisfy publishing the same data over the same topic.
* The vision controller listens to the [*object_update*](https://github.com/SjoerdKoop/vicon_control#ros-object_update) topic and should generate a suitable reference from the detected objects. This reference should be an array of floats, sent over the ROS topic [*reference_update*](https://github.com/SjoerdKoop/vicon_control#ros-reference_update).
* The robot software creates a peer which listens to the [*reference_update*](https://github.com/SjoerdKoop/vicon_control#ros-reference_update) topic and converts the message into an [UDP message](https://github.com/SjoerdKoop/vicon_control#udp-reference) to be send. This peer also listens to the robot and visualizes [sensor data](https://github.com/SjoerdKoop/vicon_control#udp-sensor-data).
* The robot controller listens to the [UDP reference message](https://github.com/SjoerdKoop/vicon_control#udp-reference) and should should control the plant accordingly. It also sends [periodic data](https://github.com/SjoerdKoop/vicon_control#udp-sensor-data) to the other end of the socket.

This results in the cascaded control loop as shown below:

![Control Loop](/images/control_loop.png)

# Setup

Good practice is to always keep your system up to date before installing any software:

```
sudo apt-get update
sudo apt-get dist-upgrade
```

## Prerequisites

Since the applications are build on the ROS framework, it is required to install and setup ROS to be able to run them. To do so, please follow the instructions described in the [ROS Lunar installation guide](http://wiki.ros.org/lunar/Installation) for your operating system. Another ROS version might still be able to run the software, but is not supported.

To cross compile ARM binaries on Ubuntu 16.04, the appropriate toolchains have to be installed. This is only required when the robot control package is being used. The toolchains for C and C++ can be installed with:

```
sudo apt-get install gcc-arm-linux-gnueabihf g++-arm-linux-gnueabihf
```

## Installation

Each component can be installed by running the corresponding *installation* script:

```
. vicon_control/robot_control/installation
. vicon_control/robot_workspace/installation
. vicon_control/vicon_workspace/installation 
. vicon_control/vision_control/installation 
```

*Before the installation scripts, there is a dot followed by a space before the actual file. This ensures that the settings also apply for the current terminal.*

The installation script of *robot_control* exports environment variables defining the location of the PRU files and calls *make*.

The other scripts call *catkin_make*. If *catkin_make* can be run (i.e. ROS has been properly installed), the scripts will source the package path to the user's *~/.bashrc*.

## Setting up the connection

### User PC

Currently, the Vicon Tracker PC has an IP address of 192.168.10.1 and is sending messages over port 801 on subnet (with netmask) 255.255.254.0. Consequently, the user PC has to have an IP address of 192.168.10.x (1 < x < 255) and must reside in the same subnet of 255.255.254.0. Example settings in the standard ubuntu desktop environment are shown below:

![Connection Settings](/images/connection_settings.png)

*When revising the settings, the GUI converted 255.255.254.0 to 23. This is normal and means that it is properly set up.*

### Robot

On the BeagleBoneBlack, network configuration is done by editing the */etc/network/interfaces* file. A proper static IP address on the same subnet can be set by appending the settings below to the file. The placeholder &lt;device&gt; should be set to the communication device that connects to the user's network (can be checked with *ifconfig*). The value *x* should be set so that it is not the same IP address as the user PC or Vicon Tracker PC. Any other embedded systems should have similar methods to change these settings.

```
## Set static <device> ip address
auto <device>
	iface <device> inet static
        address 192.168.10.x
        netmask 255.255.254.0
        gateway 192.168.10.254
        dns-nameservers 8.8.8.8
        dns-nameserver 8.8.4.4
```

## BeagleBone Black specific settings

Sampling of encoders has to be done with a relatively high sample rate. Software running on the CPU of the BeagleBone Black will not be able keep up with the rate of change of output signals of attached encoders as their speed increases. Since the BeagleBone Black has two Programmable Realtime Units (PRU's), a good solution would be to use these for computing the values of the encoders. These PRU's run specific firmware at a clock speed of 200 MHz. Computing the next state of an encoder takes 39-42 cycles, which corresponds to a ~5MHz computation frequency.

### Using the PRU

Since most PRU pins coincide with pins of the video subsystem (as can be seen in the images below), the video subsystem has to be disabled at boot-time. This can be done by uncommenting *disable_uboot_overlay_video=1* in  */boot/uEnv.txt*. The PRU subsystem is disabled by default. It has to be enabled in */boot/uEnv.txt*:

```
/boot/uEnv.txt
--------------
...
disable_uboot_overlay_video=1
...
uboot_overlay_pru=/lib/firmware/AM335X-PRU-RPROC-4-9-TI-00A0.dtbo
...
```

Make sure the PRU firmware exists and that the version coincides with your kernel version (check with *uname -r*). Reboot to apply the changes.

<img src="http://beagleboard.org/static/images/cape-headers.png" width="440"> <img src="http://beagleboard.org/static/images/cape-headers-pru.png" width="440">

Maximizing available resources (without disabling the onboard memory), PRU1 will be able to read from 6 encoders on pins 0 to 11 with a computation frequency of 833 kHz. Where the A and B pins of each encoder are being used.

To achieve modularity, PRU0 is used to create PWM signals for motors attached to the encoder using 6 of the PRU0 pins. The PRU only generates the PWM signal for each motor, directional signals are given from the controller running on the main CPU.

Using this setup, all tasks are separated:

* PRU1 handles the sensing
* PRU0 handles the actuating
* CPU runs the controller

### Setting the pins

The operating system can access the pin using the General Purpose Input/Output (GPIO) subsystem. This allows software running on the CPU to read/write the values and set the direction (input or output) of the pins. Changing the mode a specific pin can be achieved using `config-pin <header>_<pin number> <mode>`

# Usage

For documentation on running single nodes from a terminal, please refer to [Executables](https://github.com/SjoerdKoop/vicon_control#executables).

## ROS workspaces

The Robot GUI, Vicon GUI and vision controller can be run simultaneously by invoking *start*. This is the recommended method to use if all three nodes are required.

```
vicon_control/start
```

This script calls *roslauch* and includes the launch files from their corresponding packages. These launch files can also be run separately, which is the recommended method when not all nodes are required:

```
roslaunch robot_gui robot_gui.launch
roslaunch vicon_gui vicon_gui.launch
roslaunch vision_control vision_control.launch
```

Furthermore, the *robot_gui* and *vicon_gui* plugins can be manually added to a rqt window by running `rqt` (they are located under *Plugins/Visualization*) or as a standalone window with:

```
rqt -ht -s robot_gui
rqt -ht -s vicon_gui
```

## Robot control

## Robot control design

## Vision control design

Controllers should inherit from *VisionController* and should override the *objectsToReference* function. An example implementation can be seen as the example controller in the package *vision_control*.

# Communication

As shown in the general scheme, communication is an integral part connecting all the subsystems. Communication from the cameras to the Vicon Tracker, and from the Vicon Tracker to the user pc is predefined by Vicon. Therefore, four additional communication instances between nodes are defined. Additionally, *[visualization_msgs/Marker](http://docs.ros.org/api/visualization_msgs/html/msg/Marker.html)* messages are send over the topic *marker_update* in the Vicon GUI to generate markers in the rviz screen. Finally, *robot_tools/data_update_array* messages are send over the topic *[data_update](https://github.com/SjoerdKoop/vicon_control#ros-data_update)* in the robot GUI. The latter might be useful to use when the vision controller should respond to sensor data.

## ROS: data_update

To communicate from the robot peer to the data vsiualization, a *data_update_array* message is send over the topic *data_update*. This message consist of and array of *data_update*:

```
robot_tools/data_update_array.msg
---------------------------------
data_update[] updates
```

Where a *data_update* is defined by a name and a value:

```
robot_tools/data_update.msg 
---------------------------
string name
float32 value
```

The name will be the name given to the sensor on the robot.

## ROS: object_update

To communicate from the Vicon workspace to the vision controller, a *ros_object_array* message is send over the topic *object_update*. This message consist of and array of *ros_object*:

```
vicon_tools/ros_object_array.msg
--------------------------------
ros_object[] objects
```

Where a *ros_object* is defined by a name, three translational values and three rotational values:

```
vicon_tools/ros_object.msg 
--------------------------
string name
float64 x
float64 y
float64 z
float64 rx
float64 ry
float64 rz
```

The name will either be the defined name in the Vicon Tracker software for objects or "marker&lt;id&gt;" for markers. The ROS data type of *float64* coincides with the *c++* datatype of *double*. Since object data from the Vicon Tracker arrives as doubles, this type will also be used for the ROS message. Additionally for markers, the rotational values will always be 0, since it is impossible to deduce rotation from a single marker.

## ROS: reference_update

To communicate from the vision controller to the robot workspace, a standard *[Float32MultiArray](https://docs.ros.org/api/std_msgs/html/msg/Float32MultiArray.html)* message is send over topic *reference_update*. This message can be found in the *std_msgs* package and is defined as:

```
std_msgs/Float32MultiArray.msg
------------------------------
MultiArrayLayout layout
float32[] data
```

Since we only use one dimension, the *layout* variable is not relevant. A *float32* in ROS coincides with a *float* in *c++*, this is used instead of a double to effectively halve the magnitude of communication messages without losing relevant accuracy. Reference values should be added (*data.push_back(value)*) in the same order as they are to be read by the robot.

## UDP: reference

Communication of the reference generated by the vision controller to the robot is done over an UDP socket. The message consists of the number of total reference variables and each of those variables, as shown below:

![Reference UDP message](/images/reference_udp_message.png)

## UDP: sensor data

Communication of sensor data to the user is done over the same UDP socket. The message consists of the number of total sensor data, and the names and values of each sensor, as shown below:

![Sensor Data UDP message](/images/sensor_data_udp_message.png)

Since a string can have variable length, a consensus has to be made between the server and the client. Currently, the consensus is that the name string is 16 bytes long.

# Executables

The software packets create several useful command line programs for debugging or running without a GUI or without launch files. The following executables define the functionality of the subsystems:


* Vicon workspace tools (vicon_tools): These executables connect to a Vicon datastream at a given IP address and port.
	* `rosrun vicon_tools dual <Vicon datastream IP address> <Vicon datastream port> <number of markers>`
	* `rosrun vicon_tools markers <Vicon datastream IP address> <Vicon datastream port> <number of markers>`
	* `rosrun vicon_tools objects <Vicon datastream IP address> <Vicon datastream port>`
* Vision Control: Runs the vision controller
	* `rosrun vision_control vision_control`
* Robot Workspace tools (robot_tools):  This executable connects to a robot at a given IP address and port.
	* `rosrun robot_tools communicate <robot IP address> <robot port>`

The following executables are helpful tools to assist in debugging:

* Vision control tools (vision_control_tools): These tools print object updates and send reference updates.
	* `rosrun vision_control_tools object_subscriber`
	* `rosrun vision_control_tools reference_publisher` 
* Robot Workspace tools (robot_tools): These tools allow the user to send a set reference the robot at a given IP address and port and print data updates.
	* `rosrun robot_tools data_subscriber`
	* `rosrun robot_tools send_reference <robot IP address> <robot port>`
* Robot control tools: These tools are to be run on the robot. They consist of a server of data updates and a listener to reference updates from a specific host at a given IP address and port and a tool that samples the shared memory with the PRU and prints the current data at an index.
	* `data_server <user IP address> <user port>`
	* `sudo read_shared_memory <index>`\
	  *(This executable has to be run as a super user, since it involves opening a global memory map.)*
	* `reference_client <user IP address> <user port>`
	* `sudo write_shared_memory <index> <value>`\
          *(This executable has to be run as a super user, since it involves opening a global memory map.)*

# Troubleshooting

**I cannot run ROS Lunar, only other versions of ROS.**

When it is not possible to run the Lunar distribution, it is worth a try to attempt to run the software with other distributions. I have heard it also worked with Kinetic.

**I would like to change the functionality, how can I modify and run the code?**

Feel free to change any file. Updates in ROS workspaces can be applied by running *catkin_make* in the workspace. Updates tot the *robot_control* package can be applied by running make in *vicon_control/robot_control*.

**Marker detection seems unstable and unusable, how can I change this?**

Current marker detection is simple and designed for low latency. It involves a simple first order motion estimation model: ![Prediction Formula](http://latex.codecogs.com/svg.latex?x_%7Bk%2B1%7D%3Dx_k%2B%28x_k-x_%7Bk-1%7D%29%5CDelta%5C+t).

Data association is achieved by linking the marker to it's closest neighbour within a threshold, as detected by the Vicon tracker system. To replace/improve this algorithm, replace the *ViconClient::getMarkers()* function in *vicon_control/vicon_workspace/src/vicon_tools/src/vicon_trools/clients.cpp*.

**System X is not able to connect or not getting data from system Y**

* Check the hardware, make sure the cables are properly connected
* Make sure the IP address is set to the system you want to connect to
* Make sure the ports of both systems are equal
* Check the descriptions in [Setting up the connection](https://github.com/SjoerdKoop/vicon_control#setting-up-the-connection)