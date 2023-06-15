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

from .Material import Material
from .materials import SelfGravElasticity as ModuleSelfGravElasticity

from .IsotropicLinearSelfGravElasticity import IsotropicLinearSelfGravElasticity


class SelfGravElasticity(Material, ModuleSelfGravElasticity):
    """
    Material behavior governed by the elasticity equation.

    Implements `Material`.
    """
    DOC_CONFIG = {
        "cfg": """
            [pylithapp.problem.materials.mat_selfgravelastic]
            description = Upper crust self gravitating elastic material
            label_value = 3
            use_body_force = True
            bulk_rheology = pylith.materials.IsotropicLinearSelfGravElasticity

            auxiliary_subfields.density.basis_order = 0
            auxiliary_subfields.body_force.basis_order = 0
            derived_subfields.cauchy_stress.basis_order = 1
            derived_subfields.cauchy_strain.basis_order = 1
        """
    }

    import pythia.pyre.inventory

    useBodyForce = pythia.pyre.inventory.bool("use_body_force", default=False)
    useBodyForce.meta['tip'] = "Include body force term in elasticity equation."

    rheology = pythia.pyre.inventory.facility("bulk_rheology", family="selfgravitating_elasticity_rheology", factory=IsotropicLinearSelfGravElasticity)
    rheology.meta['tip'] = "Bulk rheology for elastic material."

    def __init__(self, name="selfgravelasticity"):
        """Constructor.
        """
        Material.__init__(self, name)

    def _defaults(self):
        from .AuxSubfieldsElasticity import AuxSubfieldsElasticity
        self.auxiliarySubfields = AuxSubfieldsElasticity("auxiliary_subfields")

        from .DerivedSubfieldsElasticity import DerivedSubfieldsElasticity
        self.derivedSubfields = DerivedSubfieldsElasticity("derived_subfields")

    def preinitialize(self, problem):
        """Setup material.
        """
        self.rheology.preinitialize(problem)
        Material.preinitialize(self, problem)
        self.rheology.addAuxiliarySubfields(self, problem)
        ModuleSelfGravElasticity.useBodyForce(self, self.useBodyForce)

    def _createModuleObj(self):
        """Create handle to C++ SelfGravElasticity.
        """
        ModuleSelfGravElasticity.__init__(self)
        ModuleSelfGravElasticity.setBulkRheology(self, self.rheology)  # Material sets auxiliary db in rheology.


# Factories

def material():
    """Factory associated with SelfGravElasticity.
    """
    return SelfGravElasticity()


# End of file
