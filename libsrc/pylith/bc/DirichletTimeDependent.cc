// -*- C++ -*-
//
// ----------------------------------------------------------------------
//
// Brad T. Aagaard, U.S. Geological Survey
// Charles A. Williams, GNS Science
// Matthew G. Knepley, University of Chicago
//
// This code was developed as part of the Computational Infrastructure
// for Geodynamics (http://geodynamics.org).
//
// Copyright (c) 2010-2016 University of California, Davis
//
// See COPYING for license information.
//
// ----------------------------------------------------------------------
//

#include <portinfo>

#include "DirichletTimeDependent.hh" // implementation of object methods

#include "pylith/bc/TimeDependentAuxiliaryFactory.hh" // USES TimeDependentAuxiliaryFactory
#include "pylith/feassemble/ConstraintBoundary.hh" // USES ConstraintBoundary
#include "pylith/topology/Field.hh" // USES Field
#include "pylith/topology/FieldOps.hh" // USES FieldOps
#include "pylith/topology/Mesh.hh" // USES Mesh

#include "pylith/fekernels/TimeDependentFn.hh" // USES TimeDependentFn kernels

#include "spatialdata/units/Nondimensional.hh" // USES Nondimensional

#include "pylith/utils/error.hh" // USES PYLITH_METHOD_BEGIN/END
#include "pylith/utils/journals.hh" // USES PYLITH_COMPONENT_*

#include <cassert> // USES assert()
#include <stdexcept> // USES std::runtime_error
#include <sstream> // USES std::ostringstream
#include <typeinfo> // USES typeid()

// ---------------------------------------------------------------------------------------------------------------------
namespace pylith {
    namespace bc {
        class _DirichletTimeDependent {
            // PUBLIC MEMBERS //////////////////////////////////////////////////////////////////////////////////////////
public:

            /** Set kernels for constraint.
             *
             * @param[out] constraint Constraint for boundary condition.
             * @param[in] bc Dirichlet time-dependent boundary condition.
             * @param[in] solution Solution field.
             */
            static
            void setKernelConstraint(pylith::feassemble::ConstraintBoundary* constraint,
                                     const pylith::bc::DirichletTimeDependent& bc,
                                     const pylith::topology::Field& solution);

        }; // _DirichletTimeDependent

    } // bc
} // pylith

// ---------------------------------------------------------------------------------------------------------------------
// Default constructor.
pylith::bc::DirichletTimeDependent::DirichletTimeDependent(void) :
    _dbTimeHistory(NULL),
    _auxiliaryFactory(new pylith::bc::TimeDependentAuxiliaryFactory),
    _useInitial(true),
    _useRate(false),
    _useTimeHistory(false) {
    PyreComponent::setName("dirichlettimedependent");
} // constructor


// ---------------------------------------------------------------------------------------------------------------------
// Destructor.
pylith::bc::DirichletTimeDependent::~DirichletTimeDependent(void) {
    deallocate();
} // destructor


// ---------------------------------------------------------------------------------------------------------------------
// Deallocate PETSc and local data structures.
void
pylith::bc::DirichletTimeDependent::deallocate(void) {
    PYLITH_METHOD_BEGIN;

    BoundaryCondition::deallocate();

    delete _auxiliaryFactory;_auxiliaryFactory = NULL;
    _dbTimeHistory = NULL; // :KLUDGE: Use shared pointer.

    PYLITH_METHOD_END;
} // deallocate


// ---------------------------------------------------------------------------------------------------------------------
// Set indices of constrained degrees of freedom at each location.
void
pylith::bc::DirichletTimeDependent::setConstrainedDOF(const int* flags,
                                                      const int size) {
    assert((flags && size > 0) || (!flags && 0 == size) );
    _constrainedDOF.resize(size);
    for (int i = 0; i < size; ++i) {
        _constrainedDOF[i] = flags[i];
    } // for
} // setConstrainedDOF


// ---------------------------------------------------------------------------------------------------------------------
// Get indices of constrained degrees of freedom.
const pylith::int_array&
pylith::bc::DirichletTimeDependent::getConstrainedDOF(void) const {
    return _constrainedDOF;
} // getConstrainedDOF


// ---------------------------------------------------------------------------------------------------------------------
// Set time history database.
void
pylith::bc::DirichletTimeDependent::setTimeHistoryDB(spatialdata::spatialdb::TimeHistory* th) {
    _dbTimeHistory = th;
} // setTimeHistoryDB


// ---------------------------------------------------------------------------------------------------------------------
// Get time history database.
const spatialdata::spatialdb::TimeHistory*
pylith::bc::DirichletTimeDependent::getTimeHistoryDB(void) {
    return _dbTimeHistory;
} // getTimeHistoryDB


// ---------------------------------------------------------------------------------------------------------------------
// Use initial value term in time history expression.
void
pylith::bc::DirichletTimeDependent::useInitial(const bool value) {
    PYLITH_COMPONENT_DEBUG("useInitial(value="<<value<<")");

    _useInitial = value;
} // useInitial


// ---------------------------------------------------------------------------------------------------------------------
// Get flag associated with using initial value term in time history expression.
bool
pylith::bc::DirichletTimeDependent::useInitial(void) const {
    return _useInitial;
} // useInitial


// ---------------------------------------------------------------------------------------------------------------------
// Use rate value term in time history expression.
void
pylith::bc::DirichletTimeDependent::useRate(const bool value) {
    PYLITH_COMPONENT_DEBUG("useRate(value="<<value<<")");

    _useRate = value;
} // useRate


// ---------------------------------------------------------------------------------------------------------------------
// Get flag associated with using rate value term in time history expression.
bool
pylith::bc::DirichletTimeDependent::useRate(void) const {
    return _useRate;
} // useRate


// ---------------------------------------------------------------------------------------------------------------------
// Use time history term in time history expression.
void
pylith::bc::DirichletTimeDependent::useTimeHistory(const bool value) {
    PYLITH_COMPONENT_DEBUG("useTimeHistory(value="<<value<<")");

    _useTimeHistory = value;
} // useTimeHistory


// ---------------------------------------------------------------------------------------------------------------------
// Get flag associated with using time history term in time history expression.
bool
pylith::bc::DirichletTimeDependent::useTimeHistory(void) const {
    return _useTimeHistory;
} // useTimeHistory


// ---------------------------------------------------------------------------------------------------------------------
// Verify configuration is acceptable.
void
pylith::bc::DirichletTimeDependent::verifyConfiguration(const pylith::topology::Field& solution) const {
    PYLITH_METHOD_BEGIN;
    PYLITH_COMPONENT_DEBUG("verifyConfiguration(solution="<<solution.label()<<")");

    if (!solution.hasSubfield(_subfieldName.c_str())) {
        std::ostringstream msg;
        msg << "Cannot constrain field '"<< _subfieldName
            << "' in component '" << PyreComponent::getIdentifier() << "'"
            << "; field is not in solution.";
        throw std::runtime_error(msg.str());
    } // if

    const topology::Field::SubfieldInfo& info = solution.subfieldInfo(_subfieldName.c_str());
    const int numComponents = info.description.numComponents;
    const int numConstrained = _constrainedDOF.size();
    for (int iConstrained = 0; iConstrained < numConstrained; ++iConstrained) {
        if (_constrainedDOF[iConstrained] >= numComponents) {
            std::ostringstream msg;
            msg << "Cannot constrain degree of freedom '" << _constrainedDOF[iConstrained] << "'"
                << " in component '" << PyreComponent::getIdentifier() << "'"
                << "; solution field '" << _subfieldName << "' contains only " << numComponents << " components.";
            throw std::runtime_error(msg.str());
        } // if
    } // for

    PYLITH_METHOD_END;
} // verifyConfiguration


// ---------------------------------------------------------------------------------------------------------------------
// Create integrator and set kernels.
pylith::feassemble::Integrator*
pylith::bc::DirichletTimeDependent::createIntegrator(const pylith::topology::Field& solution) {
    return NULL;
} // createIntegrator


// ---------------------------------------------------------------------------------------------------------------------
// Create constraint and set kernels.
pylith::feassemble::Constraint*
pylith::bc::DirichletTimeDependent::createConstraint(const pylith::topology::Field& solution) {
    PYLITH_METHOD_BEGIN;
    PYLITH_COMPONENT_DEBUG("createConstraint(solution="<<solution.label()<<")");

    pylith::feassemble::ConstraintBoundary* constraint = new pylith::feassemble::ConstraintBoundary(this);assert(constraint);
    constraint->setMarkerLabel(getMarkerLabel());
    constraint->setConstrainedDOF(&_constrainedDOF[0], _constrainedDOF.size());
    constraint->setSubfieldName(_subfieldName.c_str());

    _DirichletTimeDependent::setKernelConstraint(constraint, *this, solution);

    PYLITH_METHOD_RETURN(constraint);
} // createConstraint


// ---------------------------------------------------------------------------------------------------------------------
// Create auxiliary field.
pylith::topology::Field*
pylith::bc::DirichletTimeDependent::createAuxiliaryField(const pylith::topology::Field& solution,
                                                         const pylith::topology::Mesh& domainMesh) {
    PYLITH_METHOD_BEGIN;
    PYLITH_COMPONENT_DEBUG("createAuxiliaryField(solution="<<solution.label()<<", domainMesh=)"<<typeid(domainMesh).name()<<")");

    pylith::topology::Field* auxiliaryField = new pylith::topology::Field(domainMesh);assert(auxiliaryField);
    auxiliaryField->label("DirichletTimeDependent auxiliary field");

    assert(_auxiliaryFactory);
    assert(_normalizer);
    _auxiliaryFactory->initialize(auxiliaryField, *_normalizer, solution.spaceDim(),
                                  &solution.subfieldInfo(_subfieldName.c_str()).description);

    // :ATTENTION: The order of the factory methods must match the order of the auxiliary subfields in the FE kernels.

    if (_useInitial) {
        _auxiliaryFactory->addInitialAmplitude();
    } // if
    if (_useRate) {
        _auxiliaryFactory->addRateAmplitude();
        _auxiliaryFactory->addRateStartTime();
    } // _useRate
    if (_useTimeHistory) {
        _auxiliaryFactory->addTimeHistoryAmplitude();
        _auxiliaryFactory->addTimeHistoryStartTime();
        _auxiliaryFactory->addTimeHistoryValue();
    } // _useTimeHistory

    auxiliaryField->subfieldsSetup();
    pylith::topology::FieldOps::checkDiscretization(solution, *auxiliaryField);
    auxiliaryField->allocate();
    auxiliaryField->zeroLocal();

    assert(_auxiliaryFactory);
    _auxiliaryFactory->setValuesFromDB();

    PYLITH_METHOD_RETURN(auxiliaryField);
} // createAuxiliaryField


// ---------------------------------------------------------------------------------------------------------------------
// Create derived field.
pylith::topology::Field*
pylith::bc::DirichletTimeDependent::createDerivedField(const pylith::topology::Field& solution,
                                                       const pylith::topology::Mesh& domainMesh) {
    PYLITH_METHOD_BEGIN;
    PYLITH_COMPONENT_DEBUG("createDerivedField(solution="<<solution.label()<<", domainMesh=)"<<typeid(domainMesh).name()<<") empty method");

    PYLITH_METHOD_RETURN(NULL);
} // createDerivedField


// ---------------------------------------------------------------------------------------------------------------------
// Update auxiliary fields at beginning of time step.
void
pylith::bc::DirichletTimeDependent::prestep(pylith::topology::Field* auxiliaryField,
                                            const double t) {
    PYLITH_METHOD_BEGIN;
    PYLITH_COMPONENT_DEBUG("prestep(auxiliaryField="<<auxiliaryField<<", t="<<t<<")");

    if (_useTimeHistory) {
        assert(_normalizer);
        const PylithScalar timeScale = _normalizer->timeScale();
        TimeDependentAuxiliaryFactory::updateAuxiliaryField(auxiliaryField, t, timeScale, _dbTimeHistory);
    } // if

    PYLITH_METHOD_END;
} // prestep


// ---------------------------------------------------------------------------------------------------------------------
// Get auxiliary factory associated with physics.
pylith::feassemble::AuxiliaryFactory*
pylith::bc::DirichletTimeDependent::_getAuxiliaryFactory(void) {
    return _auxiliaryFactory;
} // _getAuxiliaryFactory


// ---------------------------------------------------------------------------------------------------------------------
// Update kernel constants.
void
pylith::bc::DirichletTimeDependent::_updateKernelConstants(const PylithReal dt) {}


// ---------------------------------------------------------------------------------------------------------------------
// Set kernels for computing constraint value.
void
pylith::bc::_DirichletTimeDependent::setKernelConstraint(pylith::feassemble::ConstraintBoundary* constraint,
                                                         const pylith::bc::DirichletTimeDependent& bc,
                                                         const topology::Field& solution) {
    PYLITH_METHOD_BEGIN;
    // PYLITH_COMPONENT_DEBUG("setKernelConstraint(integrator="<<integrator<<", bc="<<bc<<",
    // solution="<<solution.label()<<")");

    PetscPointFunc bcKernel = NULL;

    const pylith::topology::Field::VectorFieldEnum fieldType = solution.subfieldInfo(bc.getSubfieldName()).description.vectorFieldType;
    const bool isScalarField = fieldType == pylith::topology::Field::SCALAR;

    const int bitInitial = bc.useInitial() ? 0x1 : 0x0;
    const int bitRate = bc.useRate() ? 0x2 : 0x0;
    const int bitTimeHistory = bc.useTimeHistory() ? 0x4 : 0x0;
    const int bitUse = bitInitial | bitRate | bitTimeHistory;
    switch (bitUse) {
    case 0x1:
        bcKernel = (isScalarField) ? pylith::fekernels::TimeDependentFn::initial_scalar :
                   pylith::fekernels::TimeDependentFn::initial_vector;
        break;
    case 0x2:
        bcKernel = (isScalarField) ? pylith::fekernels::TimeDependentFn::rate_scalar :
                   pylith::fekernels::TimeDependentFn::rate_vector;
        break;
    case 0x4:
        bcKernel = (isScalarField) ? pylith::fekernels::TimeDependentFn::timeHistory_scalar :
                   pylith::fekernels::TimeDependentFn::timeHistory_vector;
        break;
    case 0x3:
        bcKernel = (isScalarField) ? pylith::fekernels::TimeDependentFn::initialRate_scalar :
                   pylith::fekernels::TimeDependentFn::initialRate_vector;
        break;
    case 0x5:
        bcKernel = (isScalarField) ? pylith::fekernels::TimeDependentFn::initialTimeHistory_scalar :
                   pylith::fekernels::TimeDependentFn::initialTimeHistory_vector;
        break;
    case 0x6:
        bcKernel = (isScalarField) ? pylith::fekernels::TimeDependentFn::rateTimeHistory_scalar :
                   pylith::fekernels::TimeDependentFn::rateTimeHistory_vector;
        break;
    case 0x7:
        bcKernel = (isScalarField) ? pylith::fekernels::TimeDependentFn::initialRateTimeHistory_scalar :
                   pylith::fekernels::TimeDependentFn::initialRateTimeHistory_vector;
        break;
    case 0x0:
        // PYLITH_COMPONENT_WARNING("Dirichlet BC provides no constraints.");
        break;
    default:
        // PYLITH_COMPONENT_ERROR("Unknown combination of flags for Dirichlet BC terms (useInitial="<<_useInitial<<",
        // useRate="<<_useRate<<", useTimeHistory="<<_useTimeHistory<<").");
        throw std::logic_error("Unknown combination of flags for Dirichlet BC terms.");
    } // switch

    assert(constraint);
    constraint->setKernelConstraint(bcKernel);

    PYLITH_METHOD_END;
} // setKernelConstraint


// End of file
