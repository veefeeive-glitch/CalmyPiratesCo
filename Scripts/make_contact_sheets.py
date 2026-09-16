"""Tile review frames into labelled contact sheets for visual review.

python Scripts/make_contact_sheets.py <folder> [--pattern door_*_closed_*.png] [--cols 4] [--per 16] [--width 480]
Writes <folder>/sheets/sheet_NN.png.
"""
import argparse, math
from pathlib import Path
from PIL import Image, ImageDraw

ap = argparse.ArgumentParser()
ap.add_argument('folder'); ap.add_argument('--pattern', default='*.png'); ap.add_argument('--cols', type=int, default=4)
ap.add_argument('--per', type=int, default=16); ap.add_argument('--width', type=int, default=480); ap.add_argument('--prefix', default='sheet')
a = ap.parse_args()
folder = Path(a.folder)
frames = sorted(p for p in folder.glob(a.pattern) if p.parent == folder)
out = folder / 'sheets'; out.mkdir(exist_ok=True)
for s in range(math.ceil(len(frames) / a.per)):
    chunk = frames[s * a.per:(s + 1) * a.per]
    th = int(a.width * 9 / 16); rows = math.ceil(len(chunk) / a.cols)
    sheet = Image.new('RGB', (a.cols * a.width, rows * (th + 18)), (12, 12, 16))
    d = ImageDraw.Draw(sheet)
    for i, p in enumerate(chunk):
        im = Image.open(p).convert('RGB').resize((a.width, th))
        x, y = (i % a.cols) * a.width, (i // a.cols) * (th + 18)
        sheet.paste(im, (x, y + 18)); d.text((x + 4, y + 3), p.stem, fill=(230, 230, 120))
    sheet.save(out / ('%s_%02d.png' % (a.prefix, s + 1)))
print('CONTACT_SHEETS frames=%d sheets=%d out=%s' % (len(frames), math.ceil(len(frames) / a.per), out))
