import importlib.util
import sys
import subprocess

print('[Kconfig] Checking Python module: kconfiglib')
if importlib.util.find_spec('kconfiglib') is None:
    print('[Kconfig] kconfiglib not found, installing via \\'pip --user\\'...')
    try:
        subprocess.check_call([sys.executable, '-m', 'pip', 'install', '--user', 'kconfiglib'])
        print('[Kconfig] kconfiglib installed successfully.')
    except subprocess.CalledProcessError as e:
        print(f'[Kconfig] ERROR: Failed to install kconfiglib: {e}', file=sys.stderr)
        sys.exit(1)
else:
    print('[Kconfig] kconfiglib is already present.')