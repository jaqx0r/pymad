#!/usr/bin/env python

import string
import os
import sys

def msg_checking(msg):
    print "Checking", msg, "...",

def execute(cmd, display = 0):
    if display:
        print cmd
    return os.system(cmd)

def run_test(input, flags = ''):
    try:
        f = open('_temp.c', 'w')
        f.write(input)
        f.close()
        compile_cmd = '%s -o _temp _temp.c %s' % (os.environ.get('CC', 'cc'),
                                                  flags)
        if not execute(compile_cmd):
            execute('./_temp')
    finally:
        execute('rm -f _temp.c _temp')
    
mad_test_program = '''
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mad.h>

int main ()
{
  system("touch conf.madtest");
  return 0;
}
'''

def find_mad(mad_prefix = '/usr/local', enable_madtest = 1):
    """A rough translation of mad.m4"""

    mad_cflags = []
    mad_libs = []
    
    mad_include_dir = mad_prefix + '/include'
    mad_lib_dir = mad_prefix + '/lib'
    mad_libs = 'mad'

    msg_checking('for MAD')

    if enable_madtest:
        execute('rm -f conf.madtest', 0)

        try:
            run_test(mad_test_program, flags="-I" + mad_include_dir)
            if not os.path.isfile('conf.madtest'):
                raise RuntimeError, "Did not produce output"
            execute('rm conf.madtest', 0)
            
        except:
            print "test program failed"
            return None

    print "success"

    return {'mad_libs' : mad_libs,
            'mad_lib_dir' : mad_lib_dir,
            'mad_include_dir' : mad_include_dir}

def write_data(data):
    f = open('Setup', 'w')
    for item in data.items():
        f.write('%s = %s\n' % item)
    f.close()
    print "Wrote Setup file"
            
def print_help():
    print '''%s
    --prefix      Give the prefix in which MAD was installed.''' % sys.argv[0]
    sys.exit(0)

def parse_args():
    data = {}
    argv = sys.argv
    for pos in range(len(argv)):
        if argv[pos] == '--help':
            print_help()
        if argv[pos] == '--prefix':
            pos = pos + 1
            if len(argv) == pos:
                print "Prefix needs an argument"
                sys.exit(1)
            data['prefix'] = argv[pos]

    return data
    
def main():
    args = parse_args()
    prefix = args.get('prefix', '/usr/local')

    data = find_mad(mad_prefix = prefix)
    if not data:
        print "Config failure"
        sys.exit(1)
    write_data(data)

if __name__ == '__main__':
    main()




