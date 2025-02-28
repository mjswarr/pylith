#!/usr/bin/env nemesis
#
# ======================================================================
#
# Brad T. Aagaard, U.S. Geological Survey
# Charles A. Williams, GNS Science
# Matthew G. Knepley, University at Buffalo
#
# This code was developed as part of the Computational Infrastructure
# for Geodynamics (http://geodynamics.org).
#
# Copyright (c) 2010-2022 University of California, Davis
#
# See LICENSE.md for license information.
#
# ======================================================================
#
# @file tests/pytests/bc/TestAbsorbingDampers.py
#
# @brief Unit testing of Python AbsorbingDampers object.

import unittest

from pylith.testing.UnitTestApp import TestComponent
from pylith.bc.AbsorbingDampers import (AbsorbingDampers, boundary_condition)


class TestAbsorbingDampers(TestComponent):
    """Unit testing of AbsorbingDampers object.
    """
    _class = AbsorbingDampers
    _factory = boundary_condition

if __name__ == "__main__":
    suite = unittest.TestSuite()
    suite.addTest(unittest.makeSuite(TestAbsorbingDampers))
    unittest.TextTestRunner(verbosity=2).run(suite)


# End of file
