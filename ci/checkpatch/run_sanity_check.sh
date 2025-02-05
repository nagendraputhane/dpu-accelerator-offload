#!/bin/bash
# SPDX-License-Identifier: Marvell-MIT
# Copyright (c) 2024 Marvell.

NONCIFILES=$(git show HEAD --stat=10000 --oneline --name-only | tail -n +2 | grep -v "^ci/")
CIFILES=$(git show HEAD --stat=10000 --oneline --name-only | tail -n +2 | grep "^ci/")

set -xe

if [[ $CIFILES != "" && $NONCIFILES != "" ]]; then
	echo "Recommending to move changes in ci/ directory to a separate commit"
fi
