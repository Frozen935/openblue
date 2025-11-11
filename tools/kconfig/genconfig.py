#!/usr/bin/env python3
#
# Copyright (c) 2021, Nordic Semiconductor ASA
#
# SPDX-License-Identifier: Apache-2.0

"""
This script is used to generate a header file from a Kconfig configuration.
It is a simplified version of the genconfig.py script from Zephyr.
"""

import os
import sys
from kconfiglib import Kconfig

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

def main():
    """
    Main function.
    """
    for k, v in REQUIRED_ENV.items():
        os.environ.setdefault(k, v)

    if len(sys.argv) != 4 or sys.argv[1] != '--header-output':
        print(f"Usage: {sys.argv[0]} --header-output <output_file> <config_file>",
              file=sys.stderr)
        sys.exit(1)

    header_file = sys.argv[2]
    config_file = sys.argv[3]
    kconfig_model = "Kconfig"

    try:
        kconf = Kconfig(kconfig_model, warn_to_stderr=False)
        kconf.load_config(config_file)
        kconf.write_autoconf(header_file)
        print(f"Generated config header: {header_file}")
    except Exception as e:
        print(f"Error generating config header: {e}", file=sys.stderr)
        sys.exit(1)

if __name__ == "__main__":
    main()