// -*- C++ -*-
//
// ======================================================================
//
// Brad T. Aagaard, U.S. Geological Survey
// Charles A. Williams, GNS Science
// Matthew G. Knepley, University of Chicago
//
// This code was developed as part of the Computational Infrastructure
// for Geodynamics (http://geodynamics.org).
//
// Copyright (c) 2010 University of California, Davis
//
// See COPYING for license information.
//
// ======================================================================
//

#include <portinfo>

#include "Solver.hh" // implementation of class methods

#include "Formulation.hh" // USES Formulation

#include "pylith/topology/SolutionFields.hh" // USES SolutionFields
#include "pylith/topology/Jacobian.hh" // USES Jacobian

#include "pylith/utils/EventLogger.hh" // USES EventLogger
#include "pylith/utils/petscerror.h" // USES CHECK_PETSC_ERROR

#include <petscdmmesh_solvers.hh> // USES constructFieldSplit()

#include <cassert> // USES assert()


// ----------------------------------------------------------------------
typedef pylith::topology::Mesh::SieveMesh SieveMesh;
typedef pylith::topology::Mesh::RealSection RealSection;

EXTERN_C_BEGIN
PetscErrorCode  MyMatGetSubMatrix(Mat mat, IS isrow, IS iscol, MatReuse reuse, Mat *newmat) {
  FaultPreconCtx *ctx;
  IS              faultIS;
  PetscBool       isFaultRow, isFaultCol;
  PetscErrorCode  ierr;

  ierr = MatShellGetContext(mat, (void **) &ctx);CHKERRQ(ierr);
  ierr = PCFieldSplitGetIS(ctx->pc, ctx->faultFieldName, &faultIS);CHKERRQ(ierr);
  ierr = ISEqual(isrow, faultIS, &isFaultRow);CHKERRQ(ierr);
  ierr = ISEqual(iscol, faultIS, &isFaultCol);CHKERRQ(ierr);
  if (isFaultRow && isFaultCol) {
    if (reuse == MAT_INITIAL_MATRIX) {
      ierr = PetscObjectReference((PetscObject) ctx->faultA);CHKERRQ(ierr);
      *newmat = ctx->faultA;
    }
  } else {
    ierr = MatGetSubMatrix(ctx->A, isrow, iscol, reuse, newmat);CHKERRQ(ierr);
  }
}
EXTERN_C_END

// ----------------------------------------------------------------------
// Constructor
pylith::problems::Solver::Solver(void) :
  _formulation(0),
  _logger(0),
  _jacobianPC(0),
  _jacobianPCFault(0)
{ // constructor
} // constructor

// ----------------------------------------------------------------------
// Destructor
pylith::problems::Solver::~Solver(void)
{ // destructor
  deallocate();
} // destructor

// ----------------------------------------------------------------------
// Deallocate PETSc and local data structures.
void
pylith::problems::Solver::deallocate(void)
{ // deallocate
  _formulation = 0; // Handle only, do not manage memory.
  delete _logger; _logger = 0;
  if (0 != _jacobianPC) {
    PetscErrorCode err = MatDestroy(_jacobianPC); _jacobianPC = 0;
    CHECK_PETSC_ERROR(err);
  } // if
  if (0 != _jacobianPCFault) {
    PetscErrorCode err = MatDestroy(_jacobianPCFault); _jacobianPCFault = 0;
    CHECK_PETSC_ERROR(err);
  } // if
} // deallocate
  
// ----------------------------------------------------------------------
// Initialize solver.
void
pylith::problems::Solver::initialize(const topology::SolutionFields& fields,
				     const topology::Jacobian& jacobian,
				     Formulation* const formulation)
{ // initialize
  assert(0 != formulation);
  _formulation = formulation;
} // initialize

// ----------------------------------------------------------------------
// Setup preconditioner for preconditioning with split fields.
void
pylith::problems::Solver::_setupFieldSplit(PetscPC* const pc,
					   Formulation* const formulation,
					   const topology::Jacobian& jacobian,
					   const topology::SolutionFields& fields)
{ // _setupFieldSplit
  assert(0 != pc);
  assert(0 != formulation);

  PetscErrorCode err = 0;

  const ALE::Obj<SieveMesh>& sieveMesh = fields.mesh().sieveMesh();
  assert(!sieveMesh.isNull());
  const topology::Field<topology::Mesh>& solution = fields.solution();
  const ALE::Obj<RealSection>& solutionSection = solution.section();
  assert(!solutionSection.isNull());

  err = PCSetType(*pc, PCFIELDSPLIT); CHECK_PETSC_ERROR(err);
  err = PCSetOptionsPrefix(*pc, "fs_"); CHECK_PETSC_ERROR(err);
  err = PCSetFromOptions(*pc); CHECK_PETSC_ERROR(err);

  constructFieldSplit(solutionSection, 
		      sieveMesh->getFactory()->getGlobalOrder(sieveMesh, "default", 
							      solutionSection), 
		      solution.vector(), *pc);

  const int spaceDim = sieveMesh->getDimension();
  if (solutionSection->getNumSpaces() > spaceDim &&
      formulation->useCustomConstraintPC()) {
    // Get total number of DOF associated with constraints field split
    const ALE::Obj<SieveMesh::order_type>& lagrangeGlobalOrder =
      sieveMesh->getFactory()->getGlobalOrder(sieveMesh, "faultDefault",
                                              solutionSection, spaceDim);
    assert(!lagrangeGlobalOrder.isNull());

    if (_jacobianPCFault) {
      err = MatDestroy(_jacobianPCFault); _jacobianPCFault = 0;
      CHECK_PETSC_ERROR(err);
    } // if
    PetscInt nrows = lagrangeGlobalOrder->getLocalSize();
    PetscInt ncols = nrows;

    err = MatCreate(sieveMesh->comm(), &_jacobianPCFault); CHECK_PETSC_ERROR(err);
    err = MatSetSizes(_jacobianPCFault, nrows, ncols, 
		      PETSC_DECIDE, PETSC_DECIDE); CHECK_PETSC_ERROR(err);
    err = MatSetType(_jacobianPCFault, MATAIJ);
    err = MatSetFromOptions(_jacobianPCFault); CHECK_PETSC_ERROR(err);
    
#if 1
    // Allocate just the diagonal.
    err = MatSeqAIJSetPreallocation(_jacobianPCFault, 1, 
				    PETSC_NULL); CHECK_PETSC_ERROR(err);
    err = MatMPIAIJSetPreallocation(_jacobianPCFault, 1, PETSC_NULL, 
				    0, PETSC_NULL); CHECK_PETSC_ERROR(err);
#else
    // Allocate full matrix (overestimate).
    err = MatSeqAIJSetPreallocation(_jacobianPCFault, ncols, 
				    PETSC_NULL); CHECK_PETSC_ERROR(err);
    err = MatMPIAIJSetPreallocation(_jacobianPCFault, ncols, PETSC_NULL, 
				    0, PETSC_NULL); CHECK_PETSC_ERROR(err);
#endif
    
    // Set preconditioning matrix in formulation
    formulation->customPCMatrix(_jacobianPCFault);

    // Make global preconditioner matrix
    PetscMat jacobianMat = jacobian.matrix();

    PetscInt M, N, m, n;
    err = MatGetSize(jacobianMat, &M, &N);CHECK_PETSC_ERROR(err);
    err = MatGetLocalSize(jacobianMat, &m, &n);CHECK_PETSC_ERROR(err);
    err = MatCreateShell(fields.mesh().comm(), m, n, M, N, &_ctx, &_jacobianPC);
    CHECK_PETSC_ERROR(err);
    err = MatShellSetOperation(_jacobianPC, MATOP_GET_SUBMATRIX, 
			       (void (*)(void)) MyMatGetSubMatrix);
    CHECK_PETSC_ERROR(err);
    _ctx.A = jacobianMat;
    _ctx.faultFieldName = "3";
    _ctx.faultA = _jacobianPCFault;
  } else {
   // Make global preconditioner matrix
    PetscMat jacobianMat = jacobian.matrix();
    _jacobianPC = jacobianMat;
    err = PetscObjectReference((PetscObject) jacobianMat);CHECK_PETSC_ERROR(err);
  } // if/else

} // _setupFieldSplit


// End of file
