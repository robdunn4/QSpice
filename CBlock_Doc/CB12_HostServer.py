#!C:\Users\%USERNAME%\AppData\Local\Microsoft\WindowsApps\python.exe
# ------------------------------------------------------------------------------
# CB12_HostServer.py -- Multi-client threaded version.
# ------------------------------------------------------------------------------
# Multi-client threaded rewrite of CB11_Server.py (one thread per client).
# Each client has its own InstData and its own logfile.
#
# Usage:
#   python CB12_HostServer.py [port]
#
# Note: This code was developed with AI assistance.  See warnings in the
#       documentation!!!
# ------------------------------------------------------------------------------

import sys
import socket
import struct
import threading
import traceback
import os

BUFFER_SIZE = 1024  # message buffer size

# Command constants (keep in sync with your original server)
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
PORT_GETPORTNBR  = 1032  # get: get server port number

DEFAULT_PORT = 1024  # Port to listen on if not specified on command line

# Per-client instance data class
class InstData:
    """Per-client instance data."""
    def __init__(self):
        self.log_name = None
        self.inst_name = None
        self.gain_val = 0
        self.step_nbr = 0
        self.post_process = False


shutdown_event = threading.Event()  # set to request full server shutdown
clients_lock = threading.Lock() # protects clients list
clients = []  # list of (thread, conn) tuples for cleanup


def read_int_le(buf: bytes, offset: int = 0) -> int:
    return struct.unpack_from("<i", buf, offset)[0]


def read_doubles_le(buf: bytes, offset: int = 0) -> tuple:
    n_bytes = len(buf) - offset
    if n_bytes <= 0:
        return tuple()
    count = n_bytes // 8
    fmt = "<" + "d" * count
    return struct.unpack_from(fmt, buf, offset)


def read_ints_le(buf: bytes, offset: int = 0) -> tuple:
    n_bytes = len(buf) - offset
    if n_bytes <= 0:
        return tuple()
    count = n_bytes // 4
    fmt = "<" + "i" * count
    return struct.unpack_from(fmt, buf, offset)


def safe_decode_cstring(raw: bytes) -> str:
    raw = raw.rstrip(b"\x00\r\n")
    try:
        return raw.decode("utf-8")
    except Exception:
        return raw.decode("latin-1", errors="replace")


def handle_client(conn: socket.socket, addr):
    inst = InstData()
    logfile = None
    close_socket = False

    peer = f"{addr[0]}:{addr[1]}"
    print(f"[+] Connected: {peer}")

    try:
        with conn:
            conn.settimeout(1.0)
            while not close_socket and not shutdown_event.is_set():
                try:
                    data = conn.recv(BUFFER_SIZE)
                except socket.timeout:
                    continue
                except OSError:
                    break

                if not data:
                    break

                try:
                    cmd = read_int_le(data, 0)
                except struct.error:
                    print(f"[!] {peer} sent malformed header. Closing.")
                    break

                # user-defined port_evaluate command
                if cmd == PORT_EVALUATE:
                    inputs = read_doubles_le(data, 4)
                    if (len(inputs) != 3):
                        print(f"[!] {peer} sent malformed EVALUATE packet.")
                        exit(-1)  # malformed packet

                    simTime, IN, EN = inputs[:3]

                    # return evaluation result
                    OUT = IN * inst.gain_val if EN > 0 else 0.0
                    conn.sendall(struct.pack("<d", OUT))

                # user-defined port_max_stepsize command
                elif cmd == PORT_MAX_STEPSIZE:
                    inputs = read_doubles_le(data, 4)
                    if (len(inputs) != 1):
                        print(f"[!] {peer} sent malformed MAX_STEPSIZE packet.")
                        exit(-1)  # malformed packet

                    simTime = inputs[0]
                    dt = 1e308

                    # return max step size (dt)
                    resp = struct.pack("<d", dt)
                    conn.sendall(resp)

                # user-defined port_truncate command
                elif cmd == PORT_TRUNCATE:
                    inputs = read_doubles_le(data, 4)
                    if (len(inputs) != 4):
                        print(f"[!] {peer} sent malformed TRUNCATE packet.")
                        exit(-1)  # malformed packet    

                    simTime, IN, EN, dt = inputs[:4]
                    
                    # return possibly adjusted timestep (dt)
                    conn.sendall(struct.pack("<d", dt))

                elif cmd == PORT_LOGNAME:
                    inst.log_name = safe_decode_cstring(data[4:])
                    conn.sendall(struct.pack("<i", cmd))

                elif cmd == PORT_INSTNAME:
                    inst.inst_name = safe_decode_cstring(data[4:])
                    conn.sendall(struct.pack("<i", cmd))

                elif cmd == PORT_GAIN:
                    vals = read_ints_le(data, 4)
                    inst.gain_val = vals[0] if vals else 0
                    conn.sendall(struct.pack("<i", cmd))

                elif cmd == PORT_STEPNBR:
                    vals = read_ints_le(data, 4)
                    inst.step_nbr = vals[0] if vals else 0
                    conn.sendall(struct.pack("<i", cmd))

                elif cmd == PORT_DESTROY:
                    conn.sendall(struct.pack("<i", cmd))

                elif cmd == PORT_POSTPROCESS:
                    inst.post_process = True
                    conn.sendall(struct.pack("<i", cmd))

                elif cmd == PORT_INITIALZE:
                    # omitting logfile(s) in this demo version but you would open them like this...
                    # try:
                    #     mode = "w" if inst.step_nbr == 1 else "a"
                    #     logfile = open(inst.log_name, mode, encoding="utf-8")
                    #     if inst.step_nbr == 1:
                    #         logfile.write(f"Inst {inst.inst_name} Port {DEFAULT_PORT}: Beginning Simulation...\n")
                    #     logfile.write(f"Inst {inst.inst_name} Port {DEFAULT_PORT}: Beginning Step {inst.step_nbr}\n")
                    #     logfile.flush()
                    # except OSError:
                    #     print(f"[!] {peer}: cannot open logfile {inst.log_name}")
                    conn.sendall(struct.pack("<i", cmd))

                elif cmd == PORT_CLOSESERVER:
                    print(f"[i] {peer} requested server socket close.")
                    close_socket = True
                    break

                elif cmd == PORT_GETPORTNBR:
                    conn.sendall(struct.pack("<i", addr[1]))

                else:
                    print(f"[!] Unknown command {cmd} from {peer}")
                    exit(-1)  # unknown command
                    # break

    except Exception as e:
        print(f"[!] Exception in client handler {peer}: {e}")
        traceback.print_exc()
    finally:
        if logfile and not logfile.closed:
            logfile.close()
        print(f"[-] Disconnected: {peer}")
        with clients_lock:
            clients[:] = [(t, c) for (t, c) in clients if c is not conn]


def serve(port: int):
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
        s.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
        s.bind(("", port))
        s.listen()
        s.settimeout(1.0)
        print(f"[i] Listening on port {port} (Ctrl+C to stop)")
        try:
            while not shutdown_event.is_set():
                try:
                    conn, addr = s.accept()
                except socket.timeout:
                    continue
                t = threading.Thread(target=handle_client, args=(conn, addr), daemon=True)
                with clients_lock:
                    clients.append((t, conn))
                t.start()
        except KeyboardInterrupt:
            print("\n[i] KeyboardInterrupt — shutting down.")
            shutdown_event.set()
        finally:
            with clients_lock:
                for t, c in clients:
                    try:
                        c.close()
                    except:
                        pass
                for t, _ in clients:
                    t.join(timeout=1)
            print("[i] Server shutdown complete.")


if __name__ == "__main__":
    if (len(sys.argv) != 2):
        print(f"Usage: python {os.path.basename(sys.argv[0])} [port]")

    else:
        try:
            serve(int(sys.argv[1]))
        except ValueError:
            print(f"Invalid port \"{sys.argv[1]}\" -- Canceling server start.")

    getchar = input("Press Enter to exit...")
    