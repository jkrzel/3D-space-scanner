# 3D-space-scanner
A scanning device that uses ultrasound distance sensor (HC-SR04) to create a 3D plot of the scanned area.

  The project is mainly focused on creating short distance obstacle detection for moving robots. The ultrasound sensor is not the best option for creating a 3D scanning device becouse of the complications associated with
ultrasound beam. The echo is usually lost on longer distances or is being wrongly bounced off the more complicated surfaces. However this project is a great base for developement of more accurate scan devices
using laser distance sensors.
  The device is able to create a 3D plot with visible obstacles in range of 200 cm thanks to the movement mechanism of two micro servos (SG-90). The servos provide movement of the sensor in Yaw Axis (0 deg - 180 deg)
and in Pitch Axis (50 deg - 130 deg). The whole device control system is set on Arduino UNO R3 that transmits every measured distance and the current positions of servos in packets of 6 bytes via UART to a 
Python program that constructs the 3D plot using matplotlib library.

The whole transmission length varies depending on scan accuracy settings in Arduino code but it can be calculated using this formula:

    TRANSMISSION_LENGTH = ( ( (180 / TURN_VALUE) + 1 ) * ( (80 / TURN_VALUE) + 1 ) * 6 ) + 3 [ Bytes ]

  where:
    (180 / TURN_VALUE) + 1 ) * ( (80 / TURN_VALUE) + 1 ) - is the number of expected packets

Packets are constructed as follows
  
Packets consist of 6 bytes: B5 B4 B3 B2 B1 B0, where:
  B5 - Start marker (0xFF)
  B4 - Rotation in x-axis
  B3 - Rotation in y-axis
  B2 - Integer part of distance value
  B1 - Fractional part of distance value
  B0 - End marker (0xFE)
  
Scanner won't continue work until program in Python sends back the ACK message. If the packet byte order has been damaged in any way through the transmission prosses the Python programme will send an NAK 
message to the Arduino to retry the measurement in the same position and send the packet again. This way no packet will be lost in the communication process.

Transmission control messagges:
  START_OF_TRANSMISSION 0xFD      - Marker of a start of a transmission
  END_OF_TRANSMISSION   0xFC      - Marker of an end of a transmission
  START_MARKER          0xFF      - Marking of the start of a packet
  END_MARKER            0xFE      - Marking of an end of a packet
  ACK                   0x06      - Acknowledgement of sent packet
  NAK                   0x15      - Negative-acknowledgement of sent packet ( retry the measurment )

The 3d plot creation process is simply calculating the point coordinates using servo rotation values and vector extention depending on the measured distance.


