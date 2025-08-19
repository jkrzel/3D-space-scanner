import serial
import matplotlib.pyplot as plt
import numpy as np
from scipy.interpolate import griddata

serialPort = serial.Serial('COM6', 115200, timeout=1)

START_OF_TRANSMISSION = 0xFD        # marker of a start of a transmission
END_OF_TRANSMISSION = 0xFC          # marker of an end of a transmission
START_MARKER = 0xFF                 # marker of a start of a packet
END_MARKER = 0xFE                   # marker of an end of a packet
ACK = 0x06                          # marker of correct packet
NAK = 0x15                          # marker of incorrect packet

radius = 4.0                        # radius of movement of sensor in both axis
max_dist = 200.0                    # maximal range of sensor (HC-SR04)

raw_points = []                     # all collected points

def read_full_packet(n_packets):
    i = 0
    while i < n_packets:
        byte_in = serialPort.read(1)

        if not byte_in:
            serialPort.write(bytes([NAK]))
            i -= 1
            continue

        if byte_in[0] != START_MARKER:
            serialPort.write(bytes([NAK]))
            i -= 1
            continue

        data = serialPort.read(5)  # X, Y, distInt, distFrac, END_MARKER
        if len(data) != 5 or data[-1] != END_MARKER:
            serialPort.write(bytes([NAK]))
            i -= 1
            continue

        x_rot = data[0] - 90.0
        y_rot = data[1] - 90.0
        d = data[2] + data[3] / 100.0

        raw_points.append((x_rot, y_rot, d))

        serialPort.write(bytes([ACK]))
        i += 1


while True:
    first_byte = serialPort.read(1)
    if not first_byte:
        continue
    if first_byte[0] != START_OF_TRANSMISSION:
        continue

    turn_value = int.from_bytes(serialPort.read(1), "little")
    num_of_packets = ((180 / turn_value) + 1) * ((80 / turn_value) + 1)

    print("Start of transmission.")

    read_full_packet(num_of_packets)

    last_byte = serialPort.read(1)
    if last_byte[0] == END_OF_TRANSMISSION:
        print("End of transmission.")
        break


serialPort.close()

raw_points = np.array(raw_points)
yaw_measured = raw_points[:, 0]
pitch_measured = raw_points[:, 1]
dist_measured = raw_points[:, 2]

grid_yaw, grid_pitch = np.meshgrid(
    np.linspace(yaw_measured.min(), yaw_measured.max(), 200),
    np.linspace(pitch_measured.min(), pitch_measured.max(), 200)
)

mask = dist_measured > 0

grid_distances = griddata(
    (yaw_measured[mask], pitch_measured[mask]),
    dist_measured[mask],
    (grid_yaw, grid_pitch),
    method='linear',
    fill_value=max_dist
)

grid_distances[0, :] = max_dist
grid_distances[-1, :] = max_dist
grid_distances[:, 0] = max_dist
grid_distances[:, -1] = max_dist

X = grid_distances * np.cos(np.deg2rad(grid_pitch)) * np.cos(np.deg2rad(grid_yaw))
Y = grid_distances * np.cos(np.deg2rad(grid_pitch)) * np.sin(np.deg2rad(grid_yaw))
Z = grid_distances * np.sin(np.deg2rad(grid_pitch))

fig = plt.figure(figsize=(10, 7))
ax = fig.add_subplot(111, projection="3d")
surf = ax.plot_surface(X, Y, Z, cmap="viridis", linewidth=0, antialiased=True)

ax.set_xlabel("X [cm]")
ax.set_ylabel("Y [cm]")
ax.set_zlabel("Z [cm]")
plt.title("Scan 3D")
plt.show()
