# ----------------------------------------------------------------------
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
# ----------------------------------------------------------------------

from .RheologySelfGravElasticity import RheologySelfGravElasticity
from .materials import IsotropicLinearSelfGravElasticity as ModuleLinearElasticity


class IsotropicLinearSelfGravElasticity(RheologySelfGravElasticity, ModuleLinearElasticity):
    """
    Isotropic linear self gravitating elastic bulk rheology.

    Implements `RheologySelfGravElasticity`.
    """
    DOC_CONFIG = {
        "cfg": """
            [pylithapp.problem.materials.mat_selfgravelastic.rheology]
            use_reference_state = False

            auxiliary_subfields.shear_modulus.basis_order = 0
            auxiliary_subfields.bulk_modulus.basis_order = 0
        """
    }

    import pythia.pyre.inventory

    useReferenceState = pythia.pyre.inventory.bool("use_reference_state", default=False)
    useReferenceState.meta['tip'] = "Use reference stress/strain state."

    def __init__(self, name="isotropiclinearselfgravelasticity"):
        """Constructor.
        """
        RheologySelfGravElasticity.__init__(self, name)

    def _defaults(self):
        from .AuxSubfieldsIsotropicLinearElasticity import AuxSubfieldsIsotropicLinearElasticity
        self.auxiliarySubfields = AuxSubfieldsIsotropicLinearElasticity("auxiliary_subfields")

    def preinitialize(self, problem):
        RheologySelfGravElasticity.preinitialize(self, problem)
        ModuleLinearElasticity.useReferenceState(self, self.useReferenceState)

    def _createModuleObj(self):
        """Call constructor for module object for access to C++ object.
        """
        ModuleLinearElasticity.__init__(self)


# FACTORIES ////////////////////////////////////////////////////////////

def incompressible_elasticity_rheology():
    """Factory associated with IsotropicLinearSelfGravElasticity.
    """
    return IsotropicLinearSelfGravElasticity()


# End of file
