// -*- C++ -*-
//
// ======================================================================
//
// Brad T. Aagaard, U.S. Geological Survey
// Charles A. Williams, GNS Science
// Matthew G. Knepley, University at Buffalo
//
// This code was developed as part of the Computational Infrastructure
// for Geodynamics (http://geodynamics.org).
//
// Copyright (c) 2010-2022 University of California, Davis
//
// See LICENSE.md for license information.
//
// ======================================================================
//

/** @file libsrc/utils/utilsfwd.hh
 *
 * @brief Forward declarations for PyLith utils objects.
 *
 * Including this header file eliminates the need to use separate
 * forward declarations.
 */

#if !defined(pylith_utils_utilsfwd_hh)
#define pylith_utils_utilsfwd_hh

namespace pylith {
    namespace utils {
        class PetscOptions;
        class PetscDefaults;

        class EventLogger;
        class GenericComponent;
        class PyreComponent;

        class PylithVersion;
        class PetscVersion;
        class DependenciesVersion;

        class TestArray;

    } // utils
} // pylith

#endif // pylith_utils_utilsfwd_hh

// End of file
