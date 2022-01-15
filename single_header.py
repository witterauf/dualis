import re
from collections import namedtuple

IncludeFile = namedtuple('IncludeFile', ['path', 'start', 'end'])

DUALIS_PATH = 'src'
OUTPUT = 'dualis.h'

with open(f'{DUALIS_PATH}/dualis.h', 'rb') as f:
    dualis = bytearray(f.read())

inc = re.compile(b'#include "(?P<file>.*\.h)".*[\\n(\\r\\n)]')
includes = []
for include in inc.finditer(dualis):
    includes.append(IncludeFile(include['file'].decode('utf-8'),
                                include.start(), include.end()))
includes = list(reversed(includes))

for include in includes:
    with open(f'{DUALIS_PATH}/{include.path}', 'rb') as f:
        include_file = bytes(f.read())
    # Only one #pragma once needed for single-header dualis
    include_file = include_file.replace(b'#pragma once', b'')
    # Remove includes of other dualis headers
    include_file = re.sub(b'#include ".*\.h"[\n(\r\n)]+', b'', include_file)
    dualis[include.start:include.end] = b'\n\n' + include_file

# Make line breaks consistent and allow at most one empty line at once
dualis = re.sub(b'\r\n', b'\n', dualis)
dualis = re.sub(b'\n{3,}', b'\n\n', dualis)

with open(OUTPUT, 'wb') as f:
    f.write(dualis)
    