
import socket
import sys

HOST = '127.0.0.1'
PORT = 6379

def resp_encode(parts):
    buf = f"*{len(parts)}\r\n"
    for part in parts:
        buf += f"${len(part)}\r\n{part}\r\n"
    return buf.encode('utf-8')

def parse_resp(f):
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
    elif line.startswith('*'):
        count = int(line[1:])
        if count == -1: return None
        arr = []
        for _ in range(count):
            arr.append(parse_resp(f))
        return arr
    return None

def test_lists():
    print(f"Connecting to {HOST}:{PORT}")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        f = s.makefile('rb')

        key = "mylist"

        # DEL
        s.sendall(resp_encode(["DEL", key]))
        parse_resp(f)

        # LPUSH a b c
        print("Testing LPUSH...")
        s.sendall(resp_encode(["LPUSH", key, "a", "b", "c"]))
        resp = parse_resp(f)
        assert resp == 3, f"Expected 3, got {resp}"

        # LRANGE 0 -1
        print("Testing LRANGE...")
        s.sendall(resp_encode(["LRANGE", key, "0", "-1"]))
        resp = parse_resp(f)
        expected = ["c", "b", "a"]
        assert resp == expected, f"Expected {expected}, got {resp}"

        # LPOP
        print("Testing LPOP...")
        s.sendall(resp_encode(["LPOP", key]))
        resp = parse_resp(f)
        assert resp == "c", f"Expected 'c', got {resp}"

        # Check remaining
        s.sendall(resp_encode(["LRANGE", key, "0", "-1"]))
        resp = parse_resp(f)
        assert resp == ["b", "a"], f"Expected ['b', 'a'], got {resp}"

        # Forwarding Test (Using a key likely to be on another shard)
        # We need to find a key that maps to a different core than 'mylist'.
        # Since we don't know the exact mapping easily without the hash function,
        # we can try a few keys.
        print("Testing Forwarding...")
        for i in range(10):
            k = f"list-{i}"
            s.sendall(resp_encode(["DEL", k]))
            parse_resp(f)

            s.sendall(resp_encode(["LPUSH", k, "X"]))
            parse_resp(f)

            s.sendall(resp_encode(["LPOP", k]))
            val = parse_resp(f)
            assert val == "X", f"Forwarding failed for key {k}: got {val}"

        print("SUCCESS")
        s.close()

    except Exception as e:
        print(f"FAILURE: {e}")
        sys.exit(1)

if __name__ == "__main__":
    test_lists()
