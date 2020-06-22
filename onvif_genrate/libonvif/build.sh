#!/bin/bash
rm -rf *.a *.o
cpp_list=`ls *.c*`
echo $cpp_list
g++ -c ${cpp_list} -DWITH_OPENSSL -DWITH_DOM

o_list=`ls *.o`
echo ${o_list}
ar cr libonvif.a $o_list



