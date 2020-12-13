import argparse
import time
import serial
from tqdm import tqdm


def get_parsed_args():

    parser = argparse.ArgumentParser()
    subparsers = parser.add_subparsers(title="Operation to perform", dest="operation")

    program_parser = subparsers.add_parser("program")
    dump_parser = subparsers.add_parser("dump")
    erase_parser = subparsers.add_parser("erase")

    # Parser for the "program" subcommand.
    program_parser.add_argument(
        "-f",
        "--file",
        help="Binary file containing the program",
        type=argparse.FileType("rb"),
        required=True,
    )

    program_parser.add_argument(
        "-p", "--port", help="Serial port where programmer is located", required=True
    )

    # Parser for the dump subcommand.
    dump_parser.add_argument(
        "-f",
        "--file",
        help="The file where the binary dump will be stored",
        type=argparse.FileType("wb"),
        required=True,
    )

    dump_parser.add_argument(
        "-p", "--port", help="Serial port where programmer is located", required=True
    )

    dump_parser.add_argument(
        "-s",
        "--start",
        help="The address where to start dumping",
        type=int,
        required=True,
    )

    dump_parser.add_argument(
        "-c",
        "--byte-count",
        help="The amount of bytes to dump",
        type=int,
        required=True,
    )

    dump_parser.add_argument(
        "--display",
        help="Print the dump to the screen after storing it to the file",
        action="store_true",
    )

    # Parser for the erase command .
    erase_parser.add_argument(
        "-p",
        "--port",
        help="Serial port where the programmer is located",
        required=True,
    )

    return parser.parse_args()


def program(args):
    print("Programming...")
    with serial.Serial(args.port, 115200) as ser:
        # Give a chance to the arduino to reset.
        # TODO: Arduino resets by default when opening a serial
        # connection, there's ways to avoid this. Investigate more.
        time.sleep(2)

        # Sending command
        ser.write((0).to_bytes(1, "big"))

        bytes_to_write = args.file.read()
        # Forward the bytes from the selected file
        for i in tqdm(range(32 * 1024)):
            value = (bytes_to_write[i]).to_bytes(1, "big")
            ser.write(value)
            # This delay was found by experimentation.
            # A byte write in the arduino takes about 80us.
            # Every 64 bytes (a page) we do data polling and could
            # take much longer. A byte send given a 115200 baud rate
            # should take ~70us, a 100us delay should give the EEPROM enough time
            # to internally write the page withouh the UART read buffer (64 bytes)
            # overflowing.
            time.sleep(100.0 * 1e-6)
        print("Done programming!")


def dump(args):
    print("Dumping...")
    with serial.Serial(args.port, 115200) as ser:
        address = int(args.start)
        byte_count = int(args.byte_count)

        # TODO: Avoid arduino autoreset.
        print("Waiting for the arduino to reset...")
        time.sleep(2)

        # Sending command
        ser.write((1).to_bytes(1, "big"))

        # Sending address, low byte first.
        ser.write((address & 0xFF).to_bytes(1, "big"))
        ser.write(((address & 0xFF00) >> 8).to_bytes(1, "big"))
        # Sending byte count, low byte first.
        ser.write((byte_count & 0xFF).to_bytes(1, "big"))
        ser.write(((byte_count & 0xFF00) >> 8).to_bytes(1, "big"))

        # Read the stream coming from the Arduino and forward them
        # to the user-selected dump file.
        for _ in range(byte_count):
            args.file.write(ser.read())
        print("Done dumping!")


def erase(args):
    print("Erasing...")
    with serial.Serial(args.port, 115200) as ser:

        # TODO: Avoid arduino autoreset.
        print("Waiting for the arduino to reset...")
        time.sleep(2)

        ser.write((2).to_bytes(1, "big"))

        # Wait for the ack.
        if ser.read() == b"\xFF":
            print("Erasing completed!")
        else:
            print("Erasing failed")


def main():
    args = get_parsed_args()

    if args.operation == "program":
        program(args)
    elif args.operation == "dump":
        dump(args)
    elif args.operation == "erase":
        erase(args)
    else:
        print("Unrecognized command, exiting now...")


if __name__ == "__main__":
    main()
