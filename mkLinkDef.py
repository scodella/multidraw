#!/usr/bin/env python

import os
import glob

with open('obj/LinkDef.h', 'w') as out:
    for fname in os.listdir('inc'):
        if fname.endswith('.h'):
            out.write('#include "%s"\n' % os.path.realpath('inc/' + fname))

    out.write('\n')
    out.write('#ifdef __CLING__\n')
    out.write('#pragma link off all globals;\n')
    out.write('#pragma link off all classes;\n')
    out.write('#pragma link off all functions;\n')
    out.write('#pragma link C++ nestedclass;\n')
    out.write('#pragma link C++ nestedtypedef;\n')
    out.write('\n')
    out.write('#pragma link C++ namespace multidraw;\n')

    for fname in os.listdir('inc'):
        if not fname.endswith('.h'):
            continue
        elif fname == 'TTreeFormulaCached.h':
            out.write('#pragma link C++ class TTreeFormulaCached;\n')
        else:
            out.write('#pragma link C++ class multidraw::%s;\n' % fname[:-2])

    out.write('#endif\n')
