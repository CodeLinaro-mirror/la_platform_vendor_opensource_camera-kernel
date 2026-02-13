#!/usr/bin/env sh

# Copyright (c) Qualcomm Technologies, Inc. and/or its subsidiaries.
# SPDX-License-Identifier: GPL-2.0-only

mkdir -p $OUT_DIR

for hfile in $( ls $IN_DIR )
do
	echo "Process file $IN_DIR/$hfile"
	headers_install.sh $IN_DIR/$hfile $OUT_DIR/$hfile &
done

wait
