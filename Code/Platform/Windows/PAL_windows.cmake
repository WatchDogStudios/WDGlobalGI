#
# Copyright (c) Contributors to the Open 3D Engine Project.
# For complete copyright and license terms please see the LICENSE at the root of this distribution.
#
# SPDX-License-Identifier: Apache-2.0 OR MIT
#

# WDGlobalGI is implemented entirely with compute shaders and does not require
# hardware ray tracing, so it is supported on every platform that runs Atom.
set(PAL_TRAIT_WDGLOBALGI_SUPPORTED TRUE)
