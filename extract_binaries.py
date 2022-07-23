import os.path
import glob

def chunks(b, n=16):
    return (b[i : i + n] for i in range(0, len(b), n))

source = [
    '#include <stdint.h>',
]

for fn in glob.glob('binaries/*'):
    basename = os.path.basename(fn)
    var_name = basename.replace('.', '_')
    with open(fn, 'rb') as r:
        source.append('uint8_t %s[] = {' % var_name)
        buf = r.read()

        for chunk in chunks(buf):
            source.append(' '.join((('0x%02X,' % x) for x in chunk)))
        source.append('};\n');

with open('arm9/include/binaries.h', 'w') as w:
    w.write('\n'.join(source))
