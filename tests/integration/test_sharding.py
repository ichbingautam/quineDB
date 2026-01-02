
import socket
import time
import sys
from concurrent.futures import ThreadPoolExecutor

HOST = '127.0.0.1'
PORT = 6379

def resp_encode(cmd):
    parts = cmd.split()
    buf = f"*{len(parts)}\r\n"
    for part in parts:
        buf += f"${len(part)}\r\n{part}\r\n"
    return buf.encode('utf-8')

def parse_resp(sock):
    # Very basic RESP parser for testing
    f = sock.makefile('rb')
    line = f.readline()
    if not line: return None
    line = line.decode('utf-8').strip()

    if line.startswith('+'):
        return line[1:]
    elif line.startswith('-'):
        raise Exception(f"Redis Error: {line[1:]}")
    elif line.startswith(':'):
        return int(line[1:])
    elif line.startswith('$'):
        length = int(line[1:])
        if length == -1: return None
        data = f.read(length)
        f.read(2) # CRLF
        return data.decode('utf-8')
    return None

def test_key(key, value):
    try:
        with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
            s.connect((HOST, PORT))

            # SET
            s.sendall(resp_encode(f"SET {key} {value}"))
            resp = parse_resp(s)
            if resp != "OK":
                print(f"FAIL: SET {key} returned {resp}")
                return False

            # GET
            s.sendall(resp_encode(f"GET {key}"))
            resp = parse_resp(s)
            if resp != value:
                print(f"FAIL: GET {key} returned {resp}, expected {value}")
                return False

        return True
    except Exception as e:
        print(f"ERROR on {key}: {e}")
        return False

def main():
    print(f"Connecting to {HOST}:{PORT}")

    keys = [f"key-{i}" for i in range(100)]

    success_count = 0
    with ThreadPoolExecutor(max_workers=10) as executor:
        futures = {executor.submit(test_key, k, f"val-{k}"): k for k in keys}
        for future in futures:
            if future.result():
                success_count += 1

    print(f"Result: {success_count}/{len(keys)} keys passed.")

    if success_count == len(keys):
        print("SUCCESS")
        sys.exit(0)
    else:
        print("FAILURE")
        sys.exit(1)

if __name__ == "__main__":
    main()
