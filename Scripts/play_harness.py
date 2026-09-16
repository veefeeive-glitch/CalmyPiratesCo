"""Minimal Windows play harness for Calmy live playtest: focus window, keys, screenshot."""
import ctypes
import ctypes.wintypes as w
import sys
import time
from pathlib import Path

user32 = ctypes.windll.user32
gdi32 = ctypes.windll.gdi32

VK = {
    'W': 0x57, 'A': 0x41, 'S': 0x53, 'D': 0x44, 'E': 0x45, 'F': 0x46, 'G': 0x47,
    'ESC': 0x1B, 'ENTER': 0x0D, 'SPACE': 0x20, 'SHIFT': 0x10, 'UP': 0x26, 'DOWN': 0x28,
    'LEFT': 0x25, 'RIGHT': 0x27, 'LMB': 0x01,
}

class POINT(ctypes.Structure):
    _fields_ = [('x', ctypes.c_long), ('y', ctypes.c_long)]

class RECT(ctypes.Structure):
    _fields_ = [('left', ctypes.c_long), ('top', ctypes.c_long), ('right', ctypes.c_long), ('bottom', ctypes.c_long)]

class KEYBDINPUT(ctypes.Structure):
    _fields_ = [('wVk', w.WORD), ('wScan', w.WORD), ('dwFlags', w.DWORD), ('time', w.DWORD), ('dwExtraInfo', ctypes.POINTER(ctypes.c_ulong))]

class MOUSEINPUT(ctypes.Structure):
    _fields_ = [('dx', w.LONG), ('dy', w.LONG), ('mouseData', w.DWORD), ('dwFlags', w.DWORD), ('time', w.DWORD), ('dwExtraInfo', ctypes.POINTER(ctypes.c_ulong))]

class INPUTUNION(ctypes.Union):
    _fields_ = [('ki', KEYBDINPUT), ('mi', MOUSEINPUT)]

class INPUT(ctypes.Structure):
    _fields_ = [('type', w.DWORD), ('union', INPUTUNION)]

KEYEVENTF_KEYUP = 0x0002
INPUT_KEYBOARD = 1
INPUT_MOUSE = 0
MOUSEEVENTF_MOVE = 0x0001
MOUSEEVENTF_LEFTDOWN = 0x0002
MOUSEEVENTF_LEFTUP = 0x0004
MOUSEEVENTF_ABSOLUTE = 0x8000
BI_RGB = 0

def find_window(substr='CalmyPiratesCo'):
    found = []
    @ctypes.WINFUNCTYPE(ctypes.c_bool, w.HWND, w.LPARAM)
    def enum_proc(hwnd, _):
        if user32.IsWindowVisible(hwnd):
            buf = ctypes.create_unicode_buffer(512)
            user32.GetWindowTextW(hwnd, buf, 512)
            if substr.lower() in buf.value.lower():
                found.append((hwnd, buf.value))
        return True
    user32.EnumWindows(enum_proc, 0)
    return found[0] if found else (None, None)

def focus(hwnd):
    user32.ShowWindow(hwnd, 9)  # SW_RESTORE
    user32.SetForegroundWindow(hwnd)
    time.sleep(0.2)

def key_down(vk):
    i = INPUT(type=INPUT_KEYBOARD, union=INPUTUNION(ki=KEYBDINPUT(wVk=vk, wScan=0, dwFlags=0, time=0, dwExtraInfo=None)))
    user32.SendInput(1, ctypes.byref(i), ctypes.sizeof(i))

def key_up(vk):
    i = INPUT(type=INPUT_KEYBOARD, union=INPUTUNION(ki=KEYBDINPUT(wVk=vk, wScan=0, dwFlags=KEYEVENTF_KEYUP, time=0, dwExtraInfo=None)))
    user32.SendInput(1, ctypes.byref(i), ctypes.sizeof(i))

def tap(name, hold=0.05):
    vk = VK[name.upper()] if isinstance(name, str) else name
    key_down(vk); time.sleep(hold); key_up(vk)

def hold(name, seconds):
    vk = VK[name.upper()]
    key_down(vk); time.sleep(seconds); key_up(vk)

def look(dx, dy, steps=8):
    for _ in range(steps):
        i = INPUT(type=INPUT_MOUSE, union=INPUTUNION(mi=MOUSEINPUT(dx=int(dx/steps), dy=int(dy/steps), mouseData=0, dwFlags=MOUSEEVENTF_MOVE, time=0, dwExtraInfo=None)))
        user32.SendInput(1, ctypes.byref(i), ctypes.sizeof(i))
        time.sleep(0.01)

def click():
    i1 = INPUT(type=INPUT_MOUSE, union=INPUTUNION(mi=MOUSEINPUT(0,0,0,MOUSEEVENTF_LEFTDOWN,0,None)))
    i2 = INPUT(type=INPUT_MOUSE, union=INPUTUNION(mi=MOUSEINPUT(0,0,0,MOUSEEVENTF_LEFTUP,0,None)))
    user32.SendInput(1, ctypes.byref(i1), ctypes.sizeof(i1))
    time.sleep(0.05)
    user32.SendInput(1, ctypes.byref(i2), ctypes.sizeof(i2))

def screenshot(hwnd, path: Path):
    rect = RECT()
    user32.GetClientRect(hwnd, ctypes.byref(rect))
    wdt = rect.right - rect.left
    hgt = rect.bottom - rect.top
    if wdt <= 0 or hgt <= 0:
        raise RuntimeError('empty client')
    hdc_win = user32.GetDC(hwnd)
    hdc_mem = gdi32.CreateCompatibleDC(hdc_win)
    hbmp = gdi32.CreateCompatibleBitmap(hdc_win, wdt, hgt)
    gdi32.SelectObject(hdc_mem, hbmp)
    # PrintWindow with PW_RENDERFULLCONTENT=2 often works for D3D
    ok = user32.PrintWindow(hwnd, hdc_mem, 2)
    if not ok:
        gdi32.BitBlt(hdc_mem, 0, 0, wdt, hgt, hdc_win, 0, 0, 0x00CC0020)
    class BITMAPINFOHEADER(ctypes.Structure):
        _fields_ = [
            ('biSize', w.DWORD), ('biWidth', w.LONG), ('biHeight', w.LONG),
            ('biPlanes', w.WORD), ('biBitCount', w.WORD), ('biCompression', w.DWORD),
            ('biSizeImage', w.DWORD), ('biXPelsPerMeter', w.LONG), ('biYPelsPerMeter', w.LONG),
            ('biClrUsed', w.DWORD), ('biClrImportant', w.DWORD),
        ]
    class BITMAPINFO(ctypes.Structure):
        _fields_ = [('bmiHeader', BITMAPINFOHEADER), ('bmiColors', w.DWORD * 3)]
    bmi = BITMAPINFO()
    bmi.bmiHeader.biSize = ctypes.sizeof(BITMAPINFOHEADER)
    bmi.bmiHeader.biWidth = wdt
    bmi.bmiHeader.biHeight = -hgt
    bmi.bmiHeader.biPlanes = 1
    bmi.bmiHeader.biBitCount = 24
    bmi.bmiHeader.biCompression = BI_RGB
    row = ((wdt * 3 + 3) & ~3)
    buf = (ctypes.c_ubyte * (row * hgt))()
    gdi32.GetDIBits(hdc_mem, hbmp, 0, hgt, buf, ctypes.byref(bmi), 0)
    gdi32.DeleteObject(hbmp)
    gdi32.DeleteDC(hdc_mem)
    user32.ReleaseDC(hwnd, hdc_win)
    # Write BMP
    path.parent.mkdir(parents=True, exist_ok=True)
    import struct
    file_size = 54 + row * hgt
    with open(path, 'wb') as f:
        f.write(b'BM')
        f.write(struct.pack('<IHHI', file_size, 0, 0, 54))
        f.write(struct.pack('<IiiHHIIiiII', 40, wdt, -hgt, 1, 24, 0, row * hgt, 0, 0, 0, 0))
        # BMP expects bottom-up unless negative height; we used negative so top-down
        f.write(bytes(buf))
    return path

def main():
    cmd = sys.argv[1] if len(sys.argv) > 1 else 'shot'
    out = Path(sys.argv[2]) if len(sys.argv) > 2 else Path('shot.bmp')
    hwnd, title = find_window()
    if not hwnd:
        print('NO_WINDOW'); return 2
    print(f'WINDOW hwnd={hwnd} title={title}')
    focus(hwnd)
    if cmd == 'shot':
        p = screenshot(hwnd, out)
        print(f'SHOT {p}')
    elif cmd == 'tap':
        tap(sys.argv[2]); print(f'TAP {sys.argv[2]}')
    elif cmd == 'hold':
        hold(sys.argv[2], float(sys.argv[3])); print(f'HOLD {sys.argv[2]} {sys.argv[3]}')
    elif cmd == 'look':
        look(int(sys.argv[2]), int(sys.argv[3])); print(f'LOOK {sys.argv[2]} {sys.argv[3]}')
    elif cmd == 'click':
        click(); print('CLICK')
    elif cmd == 'seq':
        # argv[2]=out_dir then free-form tokens: shot:name | tap:E | hold:W:1.5 | look:200:0 | sleep:1
        out_dir = Path(sys.argv[2]); out_dir.mkdir(parents=True, exist_ok=True)
        n = 0
        for tok in sys.argv[3:]:
            if tok.startswith('shot:'):
                n += 1
                p = screenshot(hwnd, out_dir / f'{n:02d}-{tok.split(":",1)[1]}.bmp')
                print(f'SHOT {p}')
            elif tok.startswith('tap:'):
                tap(tok.split(':',1)[1]); print(tok)
            elif tok.startswith('hold:'):
                _, k, s = tok.split(':'); hold(k, float(s)); print(tok)
            elif tok.startswith('look:'):
                _, x, y = tok.split(':'); look(int(x), int(y)); print(tok)
            elif tok.startswith('sleep:'):
                time.sleep(float(tok.split(':',1)[1])); print(tok)
            elif tok == 'click':
                click(); print('CLICK')
            else:
                print(f'UNKNOWN {tok}'); return 3
    else:
        print('usage'); return 1
    return 0

if __name__ == '__main__':
    raise SystemExit(main())
