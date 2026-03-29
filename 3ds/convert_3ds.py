# Please don't look at this

import os
import shutil
import subprocess
from pathlib import Path
from PIL import Image, ImageOps
from concurrent.futures import ProcessPoolExecutor

ROOT_DIR = Path(__file__).parent

CACHE_DIR = ROOT_DIR.parent / "cache"
STATIC_DIR = ROOT_DIR.parent / "static"
ROMFS = ROOT_DIR / "romfs"

TEX_3DS = Path(os.environ["DEVKITPRO"]) / "tools/bin/tex3ds"
MKBCFNT = Path(os.environ["DEVKITPRO"]) / "tools/bin/mkbcfnt"

TOP_SCREEN_SIZE = (400, 240)

# We need to automatically (mostly) convert the large images in FataMoru into sprites the 3ds can manage

def convert(source, dest, compression) -> bool:
    proc = subprocess.run([
        TEX_3DS,
        "--atlas",
        "-f",
        compression,
        "-z",
        "auto",
        "-o",
        dest,
        source,
    ])

    return proc.returncode == 0

def process_img(file):
    img = Image.open(file)
    final_target = ROMFS / file.parent.stem / f"{file.stem}.t3x"

    compression = "etc1a4"

    if img.size == (800, 600) or img.size[0] > 800:
        img = ImageOps.fit(img, TOP_SCREEN_SIZE, method=Image.Resampling.LANCZOS, centering=(0.5, 0.5))
        if not img.has_transparency_data:
            compression = "etc1"

    temp_path = Path(f"/tmp/moru_{os.getpid()}_{file.stem}.png")
    img.save(temp_path)
    print(file, img)

    if not convert(temp_path, final_target, compression):
        print("AYHHHHH!!!!")

    # assert proc.returncode == 0

if __name__ == "__main__":
    work = []

    for img_dir in [
        "bgimage",
        "image"
    ]:

        (ROMFS / img_dir).mkdir(exist_ok=True)
        # (SRC / "bgimage").mkdir(exist_ok=True)

        for file in (CACHE_DIR / img_dir).iterdir():
            if not file.is_file():
                continue

            final_target = ROMFS / file.parent.stem / f"{file.stem}.t3x"
            if final_target.is_file():
                continue
            
            work.append(file)

    if work:
        with ProcessPoolExecutor(max_workers=8) as executor:
            results = executor.map(process_img, work)

        for result in results:
            pass

    for direct_copy in ["bgm", "scenario", "sound"]:
        shutil.copytree(
            CACHE_DIR / direct_copy,
            ROMFS / direct_copy,
            dirs_exist_ok=True
        )

    (ROMFS/"static").mkdir(exist_ok=True)
    for child in STATIC_DIR.iterdir():
        if not child.is_file():
            continue


        if child.suffix == ".png":
            dest = ROMFS / "static" / f"{child.stem}.t3x"
            if not dest.is_file():
                assert convert(child, dest, "rgba8888")
        elif child.suffix == ".ttf":
            dest = ROMFS/"static"/f"{child.stem}.bcfnt"
            if not dest.is_file():
                proc = subprocess.run([
                    MKBCFNT,
                    "-s",
                    "20",
                    "-o",
                    dest,
                    child
                ])
                assert proc.returncode == 0
        else:
            # Live copy!
            dest = ROMFS/"static"/child.name
            shutil.copy(child, dest)

