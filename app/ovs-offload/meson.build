# SPDX-License-Identifier: Marvell-MIT
# Copyright (c) 2023 Marvell.

# Application Sources
sources = []

if host_build
	skip_app = true
endif

# Include directories
includes += include_directories(['include'])

subdir('control')
subdir('init')
subdir('node')

deps = ['common', 'flow']
