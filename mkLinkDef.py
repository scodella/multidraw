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
    if int(os.environ['CMSSW_VERSION'].split('_')[1]) < 9:
        # older SCRAM version cannot handle LinkDef properly -> we do it manually below
        linkdef_path = 'interface/LinkDef.h'
    else:
        linkdef_path = 'src/LinkDef.h'

    second_last_slash = thisdir.rfind('/', 0, thisdir.rfind('/') - 1)
    thisdir_cmssw_rel = thisdir[second_last_slash + 1:]
else:
    linkdef_path = 'obj/LinkDef.h'

with open(thisdir + '/' + linkdef_path, 'w') as out:
    for fname in os.listdir(thisdir + '/interface'):
        if not fname.endswith('.h') or fname == 'LinkDef.h':
            continue

        if args.cmssw:
            out.write('#include "%s/interface/%s"\n' % (thisdir_cmssw_rel, fname))
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
        if not fname.endswith('.h') or fname == 'LinkDef.h':
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

    if int(os.environ['CMSSW_VERSION'].split('_')[1]) < 10:
        import subprocess
        import shutil
        import glob

        dict_name = 'dictMultiDraw'
        pcm_path = os.environ['CMSSW_BASE'] + '/lib/' + os.environ['SCRAM_ARCH'] + '/' + dict_name + '_rdict.pcm'

        try:
            os.unlink(pcm_path)
        except:
            pass
        
        cmd = ['rootcling', '-f', thisdir + '/src/' + dict_name + '.cc']
        cmd.append('-I' + os.environ['CMSSW_BASE'] + '/src')
        proc = subprocess.Popen(['root-config', '--incdir'], stdout = subprocess.PIPE, stderr = subprocess.PIPE)
        root_inc = proc.communicate()[0].strip()
        cmd.append('-I' + root_inc)
        cmd.append(thisdir + '/interface/LinkDef.h')
        proc = subprocess.Popen(cmd)
        proc.communicate()
        if proc.returncode != 0:
            raise RuntimeError('rootcling failed')

        shutil.copyfile(thisdir + '/src/' + dict_name + '_rdict.pcm', pcm_path)
        os.unlink(thisdir + '/src/' + dict_name + '_rdict.pcm')
