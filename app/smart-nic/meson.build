# SPDX-License-Identifier: Marvell-MIT
# Copyright (c) 2024 Marvell.

# Application Sources
sources = []

if host_build
	skip_app = true
endif

# Include directories
includes += include_directories(['include'])

subdir('cli')
subdir('init')
subdir('node')

deps += ['common']
