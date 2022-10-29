#!/bin/bash
date >> elapsed.log

for i in {1..10};
do
    # echo $1
    start=`date +%s%3N` && \
    ./cliente $1 $2 && \
    end=`date +%s%3N` && \
    echo "$i: $((end-start))ms" >> elapsed.log &
done