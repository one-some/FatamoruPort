import os
import shutil
import subprocess
from pathlib import Path
from PIL import Image, ImageOps

CACHE_DIR = Path("../cache")
CONVERT_DIR = Path("./gfx")
ROMFS = Path("./romfs")
SRC = Path("./src")

try:
    shutil.rmtree(CONVERT_DIR)
except FileNotFoundError:
    pass
CONVERT_DIR.mkdir(exist_ok=True)

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


for img_dir in [
    "bgimage",
    "image"
]:

    dest = CONVERT_DIR / img_dir
    dest.mkdir()

    (ROMFS / img_dir).mkdir(exist_ok=True)
    # (SRC / "bgimage").mkdir(exist_ok=True)

    for child in (CACHE_DIR / img_dir).iterdir():
        if not child.is_file():
            continue

        final_target = ROMFS / img_dir / f"{child.stem}.t3x"
        if final_target.is_file():
            # print("Skip.")
            continue

        img = Image.open(child)

        compression = "etc1a4"

        if img.size == (800, 600) or img.size[0] > 800:
            img = ImageOps.fit(img, TOP_SCREEN_SIZE, method=Image.Resampling.LANCZOS, centering=(0.5, 0.5))
            if not img.has_transparency_data:
                compression = "etc1"

        img.save("/tmp/moru.png")
        print(child, img)

        if not convert("/tmp/moru.png", final_target, compression):
            print("AYHHHHH!!!!")

        # assert proc.returncode == 0

for direct_copy in ["bgm", "scenario", "sound"]:
    shutil.copytree(
        CACHE_DIR / direct_copy,
        ROMFS / direct_copy,
        dirs_exist_ok=True
    )

# shutil.copytree(
#     Path("../static"),
#     ROMFS / "static",
#     dirs_exist_ok=True
# )
(ROMFS/"static").mkdir(exist_ok=True)
for child in Path("../static").iterdir():
    if not child.is_file():
        continue


    if child.suffix == ".png":
        assert convert(child, ROMFS/"static"/f"{child.stem}.t3x", "rgba8888")
    elif child.suffix == ".ttf":
        dest = ROMFS/"static"/f"{child.stem}.bcfnt"
        proc = subprocess.run([
            MKBCFNT,
            "-o",
            dest,
            child
        ])
        assert proc.returncode == 0
    else:
        dest = ROMFS/"static"/child.name
        shutil.copy(child, dest)

