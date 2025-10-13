#!C:\Users\%USERNAME%\AppData\Local\Microsoft\WindowsApps\python.exe
# If you use Cygwin, this might work better for you:  #!C:\cygwin64\bin\python3.9.exe

# ------------------------------------------------------------------------------
# CB11_Server.py -- Example server Python code for C-Block Basics #11.
# ------------------------------------------------------------------------------
# Full project code and documentation is available here:
#    https://github.com/robdunn4/QSpice
#

import sys
import socket
import struct

BUFFER_SIZE = 1024  # message buffer size

# Constants:
PORT_EVALUATE = 2
PORT_MAX_STEPSIZE = 3
PORT_TRUNCATE = 4
PORT_LOGNAME = 1024  # set logfile name string
PORT_INSTNAME = 1025  # set instance name string
PORT_GAIN = 1026  # set gain int val
PORT_STEPNBR = 1027  # set step # int val
PORT_DESTROY = 1028  # event: client Destroy()
PORT_POSTPROCESS = 1029  # event: client PostProcess()
PORT_INITIALZE = 1030  # cmd: initialize server with current values
PORT_CLOSESERVER = 1031  # cmd: close server

PORT = 1024  # Port to listen on if not specified on command line(non-privileged ports are > 1023)


# Since there's one server process per instance, you can put per instance data globally here.
# Using a class to encapsulate per-instance data for clarity
class InstData:
    log_name = None
    inst_name = None
    gain_val = None
    step_nbr = None
    post_process = False


# this isn't really right -- should fail and display syntax if arg count != 2
if __name__ == "__main__":
    if len(sys.argv) > 1:
        PORT = sys.argv[1]
    else:
        print(f"Using port {PORT}")

with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    inst = InstData()
    LOGFILE = None
    CLOSE_SOCKET = False

    s.bind(("", int(PORT)))
    s.listen()
    conn, addr = s.accept()

    with conn:
        while not CLOSE_SOCKET:
            data = conn.recv(BUFFER_SIZE)
            if not data:
                break
            cmd = ((memoryview(data)[0:4]).cast("i"))[0]

            if cmd == PORT_EVALUATE:
                inputs = (memoryview(data)[4 : len(data)]).cast("d")
                simTime = inputs[0]
                IN = inputs[1]
                EN = inputs[2]

                # Your job is to implement the calculation of the values of output pins
                OUT = 0.0
                if EN > 0:
                    OUT = IN * inst.gain_val
                data = bytearray(struct.pack("d", OUT))
                conn.sendall(data)

            elif cmd == PORT_MAX_STEPSIZE:
                inputs = (memoryview(data)[4 : len(data)]).cast("d")
                simTime = inputs[0]

                # You can compute a smaller timestep to stipulate here
                dt = 1e308  # default 1e308 does nothing

                LOGFILE.write(
                    f"Inst {inst.inst_name} Port {PORT}: "
                    f"MaxStep():  t={simTime}, retVal={dt}\n"
                )

                data = bytearray(struct.pack("d", dt))
                conn.sendall(data)

            elif cmd == PORT_TRUNCATE:
                inputs = (memoryview(data)[4 : len(data)]).cast("d")
                simTime = inputs[0]
                IN = inputs[1]
                EN = inputs[2]
                dt = inputs[3] # This is the otherwise planned timestep.

                # You can stipulate a shorter timestep here

                LOGFILE.write(
                    f"Inst {inst.inst_name} Port {PORT}: "
                    f"Trunc():    t={simTime}, "
                    f"Gain={inst.gain_val}, EN={EN}, IN={IN}, "
                    f"timestep={simTime}, retVal={dt}\n"
                )

                data = bytearray(struct.pack("d", dt))
                conn.sendall(data)

            elif cmd == PORT_LOGNAME:
                inst.log_name = data[4 : len(data) - 1].decode("utf-8")
                data = bytearray(struct.pack("i", cmd))
                conn.sendall(data)

            elif cmd == PORT_INSTNAME:
                inst.inst_name = data[4 : len(data) - 1].decode("utf-8")
                data = bytearray(struct.pack("i", cmd))
                conn.sendall(data)

            elif cmd == PORT_GAIN:
                inst.gain_val = (memoryview(data)[4 : len(data)]).cast("i")[0]
                data = bytearray(struct.pack("i", cmd))
                conn.sendall(data)

            elif cmd == PORT_STEPNBR:
                inst.step_nbr = (memoryview(data)[4 : len(data)]).cast("i")[0]
                data = bytearray(struct.pack("i", cmd))
                conn.sendall(data)

            elif cmd == PORT_DESTROY:
                # log end of step; error handling omitted
                LOGFILE.write(
                    f"Inst {inst.inst_name} Port {PORT}: "
                    f"End of Step {inst.step_nbr}\n"
                )

                # log end of simulation; error handling omitted
                if inst.post_process:
                    LOGFILE.write(
                        f"Inst {inst.inst_name} Port {PORT}: " "End of Simulation...\n"
                    )

                # close log file; error handling omitted
                LOGFILE.close()

                data = bytearray(struct.pack("i", cmd))
                conn.sendall(data)

            elif cmd == PORT_POSTPROCESS:
                # set end of simulation flag (used in PORT_DESTROY)
                inst.post_process = True
                data = bytearray(struct.pack("i", cmd))
                conn.sendall(data)

            elif cmd == PORT_INITIALZE:
                # open logfile here
                try:
                    FMODE = "w" if inst.step_nbr == 1 else "a"
                    LOGFILE = open(inst.log_name, FMODE, encoding="utf-8")
                except OSError:
                    print('Unable to open logfile ("{inst.LOGNAME}")')
                    sys.exit(-1)

                # write initial log entries; error handling omitted
                if inst.step_nbr == 1:
                    LOGFILE.write(
                        f"Inst {inst.inst_name} Port {PORT}: "
                        "Beginning Simulation...\n"
                    )
                LOGFILE.write(
                    f"Inst {inst.inst_name} Port {PORT}: "
                    f"Beginning Step {inst.step_nbr}\n"
                )

                data = bytearray(struct.pack("i", cmd))
                conn.sendall(data)

            elif cmd == PORT_CLOSESERVER:
                # close server, no response
                CLOSE_SOCKET = True

            else:
                # exit ungracefully
                print(f"(Unhandled message in server ({PORT}).  Closing server...")
                exit(-1)
