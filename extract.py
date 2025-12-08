import os
from pathlib import Path

GAME_DIR = Path("/home/claire/.steam/root/steamapps/common/The House in Fata Morgana")
SIGNATURE = b"XP3\x0D\x0A\x20\x0A\x1A\x8B\x67\x01"
CACHE_DIR = Path("./cache")
CACHE_DIR.mkdir(exist_ok=True)

def read_file_sect(xp3_io):
    entry_size = int.from_bytes(xp3_io.read(8), "little")
    start = xp3_io.tell()

    path = ""
    segments = []

    while xp3_io.tell() < start + entry_size:
        sect_type = xp3_io.read(4)

        if sect_type == b"info":
            info_length = int.from_bytes(xp3_io.read(8), "little")
            info_start = xp3_io.tell()

            flags = xp3_io.read(4)

            og_size = int.from_bytes(xp3_io.read(8), "little")
            comp_size = int.from_bytes(xp3_io.read(8), "little")
            fname_len = int.from_bytes(xp3_io.read(2), "little")

            working_path = b""
            for _ in range(fname_len):
                working_path += xp3_io.read(2)
            path = working_path.decode('utf-16le')

            xp3_io.seek(info_start + info_length, os.SEEK_SET)
        elif sect_type == b"segm":
            seg_count = int.from_bytes(xp3_io.read(8), "little") // 28

            for _ in range(seg_count):
                is_compressed = int.from_bytes(xp3_io.read(4), "little")
                assert not is_compressed

                segments.append({
                    "offset": int.from_bytes(xp3_io.read(8), "little"),
                    "size": int.from_bytes(xp3_io.read(8), "little"),
                    "comp_size": int.from_bytes(xp3_io.read(8), "little"),
                })
        elif sect_type == b"adlr":
            assert int.from_bytes(xp3_io.read(8), "little") == 4
            adlr = int.from_bytes(xp3_io.read(4), "little")
        else:
            print(f"Got weirdo {sect_type}")
            assert False
    return path, segments

def extract_xp3(xp3_io, target_dir):
    # One day I'm gonna use the struct module... but not today!

    assert xp3_io.read(len(SIGNATURE)) == SIGNATURE

    xp3_io.seek(8, os.SEEK_CUR)

    version = int.from_bytes(xp3_io.read(4), "little")
    assert version == 1

    table_size = int.from_bytes(xp3_io.read(8), "little")
    # print(table_size)

    flags = int.from_bytes(xp3_io.read(1), "little")
    assert flags == 0

    offset = int.from_bytes(xp3_io.read(8), "little")
    xp3_io.seek(offset, os.SEEK_SET)

    # Not compressed
    assert xp3_io.read(1) == b'\x00'

    compressed_table_size = int.from_bytes(xp3_io.read(8), "little")

    while True:
        magic = xp3_io.read(4)

        if not magic:
            break

        if magic == b"File":
            path, segments = read_file_sect(xp3_io)
            print(path)
            before = xp3_io.tell()

            out = bytearray()

            for s in segments:
                xp3_io.seek(s["offset"], os.SEEK_SET)
                out += xp3_io.read(s["size"])

            if path.endswith(".ks"):
                # I DON'T WANNA DEAL WITH THIS IN C!!!
                out = out.decode("utf-16")
                out = out.replace("\r\n", "\n")
                out = out.encode("utf-8")

            out_path = target_dir / path
            out_path.parent.mkdir(parents=True, exist_ok=True)

            try:
                out_path.write_bytes(out)
            except OSError as e:
                # Boo ho
                if "protected archive" not in str(e):
                    raise

            xp3_io.seek(before, os.SEEK_SET)
        else:
            print(magic)
            assert False

def extract_files():
    print("[xp3] Extracting game data...")
    for xp3_name in ["data", "data_en"]:
        # data_en will overwrite whatever it needs to in data

        xp3 = GAME_DIR / f"{xp3_name}.xp3"
        with xp3.open("rb") as file:
            extract_xp3(file, CACHE_DIR)

if __name__ == "__main__":
    extract_files()
