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

def test_structures():
    print(f"Connecting to {HOST}:{PORT}")
    try:
        s = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        s.connect((HOST, PORT))
        f = s.makefile('rb')

        # --- SETS ---
        print("Testing SETs...")
        key = "myset"
        s.sendall(resp_encode(["DEL", key]))
        parse_resp(f)

        s.sendall(resp_encode(["SADD", key, "a", "b", "c"]))
        res = parse_resp(f)
        assert res == 3, f"SADD expected 3, got {res}"

        s.sendall(resp_encode(["SADD", key, "a"])) # duplicate
        res = parse_resp(f)
        assert res == 0, f"SADD duplicate expected 0, got {res}"

        s.sendall(resp_encode(["SMEMBERS", key]))
        members = parse_resp(f)
        # Sort to compare set
        assert sorted(members) == ["a", "b", "c"], f"SMEMBERS unexpected: {members}"

        # --- HASHES ---
        print("Testing HASHes...")
        key = "myhash"
        s.sendall(resp_encode(["DEL", key]))
        parse_resp(f)

        s.sendall(resp_encode(["HSET", key, "field1", "val1", "field2", "val2"]))
        res = parse_resp(f)
        assert res == 2, f"HSET expected 2, got {res}"

        s.sendall(resp_encode(["HGET", key, "field1"]))
        res = parse_resp(f)
        assert res == "val1", f"HGET expected val1, got {res}"

        s.sendall(resp_encode(["HGETALL", key]))
        all_fields = parse_resp(f)
        # HGETALL returns list [f1, v1, f2, v2] order undefined in map? No, map is sorted by key
        # field1 < field2
        assert all_fields == ["field1", "val1", "field2", "val2"], f"HGETALL unexpected: {all_fields}"

        s.sendall(resp_encode(["HLEN", key]))
        res = parse_resp(f)
        assert res == 2, f"HLEN expected 2, got {res}"

        s.sendall(resp_encode(["HDEL", key, "field1"]))
        res = parse_resp(f)
        assert res == 1, f"HDEL expected 1, got {res}"

        s.sendall(resp_encode(["HLEN", key]))
        res = parse_resp(f)
        assert res == 1, f"HLEN expected 1, got {res}"

        # --- ZSETS ---
        print("Testing ZSETs...")
        key = "myzset"
        s.sendall(resp_encode(["DEL", key]))
        parse_resp(f)

        s.sendall(resp_encode(["ZADD", key, "10", "member1", "20", "member2"]))
        res = parse_resp(f)
        assert res == 2, f"ZADD expected 2, got {res}"

        s.sendall(resp_encode(["ZADD", key, "15", "member1"])) # update score
        res = parse_resp(f)
        assert res == 0, f"ZADD update expected 0, got {res}"

        # ZRANGE 0 -1 WITHSCORES
        # Order should be member1(15), member2(20)
        s.sendall(resp_encode(["ZRANGE", key, "0", "-1", "WITHSCORES"]))
        zrange = parse_resp(f)
        # Note: score formatting might vary (15 or 15.0 or 15.000000)
        # My implementation attempts to strip zeros.
        print(f"ZRANGE output: {zrange}")
        assert zrange[0] == "member1"
        assert zrange[2] == "member2"
        # Check scores loosely
        assert zrange[1].startswith("15")
        assert zrange[3].startswith("20")

        # --- LISTS (RPUSH/POP) ---
        print("Testing LISTs (RPUSH/POP)...")
        key = "mylist"
        s.sendall(resp_encode(["DEL", key]))
        parse_resp(f)

        s.sendall(resp_encode(["RPUSH", key, "a", "b", "c"]))
        res = parse_resp(f)
        assert res == 3, f"RPUSH expected 3, got {res}"

        s.sendall(resp_encode(["LLEN", key]))
        res = parse_resp(f)
        assert res == 3, f"LLEN expected 3, got {res}"

        s.sendall(resp_encode(["RPOP", key]))
        res = parse_resp(f)
        assert res == "c", f"RPOP expected c, got {res}"

        s.sendall(resp_encode(["LLEN", key]))
        res = parse_resp(f)
        assert res == 2, f"LLEN expected 2, got {res}"

        s.sendall(resp_encode(["LPUSH", key, "z"]))
        length_after_lpush = parse_resp(f)  # Consume LPUSH response
        # List is now [z, a, b]

        s.sendall(resp_encode(["LRANGE", key, "0", "-1"]))
        res = parse_resp(f)
        assert res == ["z", "a", "b"], f"List order mismatch: {res}"

        print("SUCCESS")
        s.close()
    except Exception as e:
        print(f"FAILURE: {e}")
        import traceback
        traceback.print_exc()
        sys.exit(1)

if __name__ == "__main__":
    test_structures()
