#!/usr/bin/env python3
# Minimal wrapper to run Kconfiglib's menuconfig and generate .config
# Also sets common UINT*/INT* MAX/MIN environment variables used in Zephyr-derived Kconfig.

import os
import sys
import inspect

REQUIRED_ENV = {
    'UINT8_MAX': '255',
    'UINT16_MAX': '65535',
    'UINT32_MAX': '4294967295',
    'INT8_MAX': '127',
    'INT8_MIN': '-128',
    'INT16_MAX': '32767',
    'INT16_MIN': '-32768',
    'INT32_MAX': '2147483647',
    'INT32_MIN': '-2147483648',
}

for k, v in REQUIRED_ENV.items():
    os.environ.setdefault(k, v)

if len(sys.argv) < 3:
    print('Usage: run_menuconfig.py <Kconfig_path> <output_.config>')
    sys.exit(2)

kconfig_path = os.path.abspath(sys.argv[1])
output_config = os.path.abspath(sys.argv[2])

# Ensure kconfiglib is available
try:
    import kconfiglib
except ImportError:
    print('kconfiglib not installed. Please install with: pip install kconfiglib', file=sys.stderr)
    sys.exit(1)

# Locate menuconfig.py provided by kconfiglib
mod_dir = os.path.dirname(inspect.getfile(kconfiglib))
menuconfig_py = os.path.join(mod_dir, 'menuconfig.py')
if not os.path.exists(menuconfig_py):
    print('Could not find menuconfig.py in kconfiglib package at', menuconfig_py, file=sys.stderr)
    sys.exit(1)

# Set KCONFIG_CONFIG to desired output file to ensure .config is written there
os.environ['KCONFIG_CONFIG'] = output_config
# Hint Kconfiglib to resolve relative sources from the repository root
os.environ.setdefault('srctree', os.path.dirname(kconfig_path))

# Run menuconfig UI if possible; otherwise fallback to non-interactive config write
cmd = [sys.executable, menuconfig_py, kconfig_path]
print('Running:', ' '.join(cmd))
ret = os.system(' '.join(cmd))
if ret != 0:
    print('Menuconfig UI failed (non-interactive environment?). Falling back to writing default .config')
    try:
        kconf = kconfiglib.Kconfig(kconfig_path)
        kconf.write_config(output_config)
        print('Fallback wrote', output_config)
    except Exception as e:
        print('Fallback failed:', e, file=sys.stderr)
        sys.exit(ret)
else:
    print('Generated', output_config)
