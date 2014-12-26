#! /usr/bin/env python
# encoding: utf-8

APPNAME = 'rf-memberlist'
VERSION = '0.1'

top = '.'
out = 'build'

def configure(ctx):
    ctx.env.CFLAGS = ['-Wall', '-lpanelw', '-lformw',
                      '-lncursesw', '-lssh', '-lsqlite3']
    try:
        ctx.find_program('clang', var='CC')
    except:
        ctx.find_program('gcc', var='CC')

def build(ctx):
    ctx(rule='${CC} -shared ${SRC} `icu-config --ldflags`'
        '-fPIC -o ${TGT}', source='src/icu.c', target='libSqliteIcu.so')
    ctx(rule='${CC} -o ${TGT} ${SRC} ${CFLAGS}',
        source='src/rf.c', target=APPNAME)
