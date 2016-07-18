#!/usr/bin/env python
#
# ----------------------------------------------------------------------
#
# Brad T. Aagaard, U.S. Geological Survey
# Charles A. Williams, GNS Science
# Matthew G. Knepley, University of Chicago
#
# This code was developed as part of the Computational Infrastructure
# for Geodynamics (http://geodynamics.org).
#
# Copyright (c) 2010-2016 University of California, Davis
#
# See COPYING for license information.
#
# ----------------------------------------------------------------------
#

__all__ = ['PhysicalProperties',
           'Solution',
           'StateVariables',
           'Fault',
           ]


# ----------------------------------------------------------------------
def has_h5py():
  if not "flag" in dir(has_h5py):
    try:
      import h5py
      has_h5py.flag = True
    except ImportError:
      print "WARNING: Cannot find h5py Python modele."
      print "         Tests limited to running PyLith without errors."
      print "         Install h5py (available via the installer utility) "
      print "         in order to enable verification of output."
      has_h5py.flag = False
  return has_h5py.flag


# ----------------------------------------------------------------------
def run_pylith(appClass, dbClass=None, nprocs=1):
  """
  Helper function to generate spatial databases and run PyLith.
  """
  if not "done" in dir(run_pylith):
    if not dbClass is None:
      # Generate spatial databases
      db = dbClass()
      db.run()

    # Run PyLith, limiting number of processes to number of local CPUs.
    import multiprocessing
    maxprocs = multiprocessing.cpu_count()

    app = appClass()
    app.nodes = min(nprocs, maxprocs)
    run_pylith.done = True # Put before run() so only called once
    app.run()
  return


# End of file
