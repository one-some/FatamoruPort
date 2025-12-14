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

TOP_SCREEN_SIZE = (400, 240)

# We need to automatically (mostly) convert the large images in FataMoru into sprites the 3ds can manage

BG_DEST = CONVERT_DIR / "bgimage"
BG_DEST.mkdir()

(ROMFS / "bgimage").mkdir(exist_ok=True)
(SRC / "bgimage").mkdir(exist_ok=True)

for child in (CACHE_DIR / "bgimage").iterdir():
    if not child.is_file():
        continue

    img = Image.open(child)

    if img.size != (800, 600):
        continue

    img = ImageOps.fit(img, TOP_SCREEN_SIZE, method=Image.Resampling.LANCZOS, centering=(0.5, 0.5))
    img.save(BG_DEST / child.name)
    print(child, img)

    proc = subprocess.run([
        TEX_3DS,
        "--atlas",
        "-f",
        "rgb565",
        "-z",
        "auto",
        # "-H",
        # SRC / "bgimage" / f"{child.stem}.h",
        "-o",
        ROMFS / "bgimage" / f"{child.stem}.t3x",
        BG_DEST / child.name
    ])

    assert proc.returncode == 0
