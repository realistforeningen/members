#! /usr/bin/env python
# encoding: utf-8

APPNAME = 'rf-memberlist'
VERSION = '0.1'

top = '.'
out = 'build'

def options(opt):
    opt.load('compiler_c gnu_dirs')
    opt.add_option('--debug', action='store', default=False,
                   help='Turn on debug mode (True/False) [default: False]')

def configure(ctx):
    from waflib.Tools.compiler_c import c_compiler
    c_compiler['linux'] = ['clang', 'gcc', 'icc']
    libs = ['panelw', 'formw', 'ncursesw', 'ssh', 'sqlite3']
    ctx.env.CFLAGS = ['-Wall', '-Wunused']
    if ctx.options.debug:
        print('=== DEBUG MODE ===')
        ctx.env.CFLAGS.append('-g')
    else:
        ctx.env.CFLAGS.append('-Ofast')
    ctx.load('compiler_c')
    ctx.check_cc(msg='Testing compiler', fragment="int main() { return 0; }\n", execute=True)
    for lib in libs:
        ctx.check_cc(lib=lib, cflags=ctx.env.CFLAGS, uselib_store='L', mandatory=True)

def build(ctx):
    ctx.program(source='src/rf.c',
                target=APPNAME,
                includes='.',
                use='L')
    ctx.shlib(source='src/icu.c',
              target='SqliteIcu')
