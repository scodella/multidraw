#!/usr/bin/env python

import os
import sys
import glob
from argparse import ArgumentParser

argParser = ArgumentParser(description = 'Generate LinkDef.h')
argParser.add_argument('--cmssw', '-C', action = 'store_true', dest = 'cmssw', help = 'Write the LinkDef with include paths relative to $CMSSW_BASE/src. Write a BuildFile too.')

args = argParser.parse_args()
sys.argv = []

thisdir = os.path.dirname(os.path.realpath(__file__))

if args.cmssw:
    linkdef_path = 'src/LinkDef.h'
else:
    linkdef_path = 'obj/LinkDef.h'

with open(linkdef_path, 'w') as out:
    for fname in os.listdir(thisdir + '/interface'):
        if not fname.endswith('.h'):
            continue

        if args.cmssw:
            second_last_slash = thisdir.rfind('/', 0, thisdir.rfind('/') - 1)
            out.write('#include "%s/interface/%s"\n' % (thisdir[second_last_slash + 1:], fname))
        else:
            out.write('#include "%s/interface/%s"\n' % (thisdir, fname))

    out.write('\n')
    out.write('#ifdef __CLING__\n')
    out.write('#pragma link off all globals;\n')
    out.write('#pragma link off all classes;\n')
    out.write('#pragma link off all functions;\n')
    out.write('#pragma link C++ nestedclass;\n')
    out.write('#pragma link C++ nestedtypedef;\n')
    out.write('\n')
    out.write('#pragma link C++ namespace multidraw;\n')

    for fname in os.listdir(thisdir + '/interface'):
        if not fname.endswith('.h'):
            continue

        if fname == 'TTreeFormulaCached.h':
            out.write('#pragma link C++ class TTreeFormulaCached+;\n')
        else:
            out.write('#pragma link C++ class multidraw::%s;\n' % fname[:-2])

    out.write('#endif\n')

if args.cmssw:
    with open(thisdir + '/BuildFile.xml', 'w') as out:
        out.write('<use name="root"/>\n')
        out.write('<use name="rootcore"/>\n')
        out.write('<use name="rootgraphics"/>\n')
        out.write('<export>\n')
        out.write('  <lib name="1"/>\n')
        out.write('</export>\n')
