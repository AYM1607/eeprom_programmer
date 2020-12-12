import glob
import argparse
import serial


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--port")
    args = parser.parse_args()

    # Get all the valid serial ports available.
    available_ports = glob.glob("/dev/cu.*")

    # Only do this if the user did not specify a port.
    if args.port is None:
        # Make the user select a port to start a listening session.
        print("Available ports:")
        for i, port in enumerate(available_ports):
            print(f"{i} - {port}")

    selected_port_index = None
    while selected_port_index is None and args.port is None:
        try:
            selected_port_index = input(
                "\nType port number to start listening section: "
            )
            selected_port_index = int(selected_port_index)
            if selected_port_index < 0 or selected_port_index >= len(available_ports):
                print("The port number you provided does not exist.")
                selected_port_index = None
        except:
            print("The typed input is not valid, please enter a valid number.")
            selected_port_index = None

    port = args.port if args.port is not None else available_ports[selected_port_index]
    # TODO: Allow selecting baud raute, 115200 by default for now.
    with serial.Serial(port, 115200) as ser:
        while True:
            byte = ser.read()
            print(byte.decode("utf-8"), end="")


if __name__ == "__main__":
    main()
