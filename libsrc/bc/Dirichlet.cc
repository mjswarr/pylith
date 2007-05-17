// -*- C++ -*-
//
// ----------------------------------------------------------------------
//
//                           Brad T. Aagaard
//                        U.S. Geological Survey
//
// {LicenseText}
//
// ----------------------------------------------------------------------
//

#include <portinfo>

#include "Dirichlet.hh" // implementation of object methods

#include "spatialdata/spatialdb/SpatialDB.hh" // USES SpatialDB
#include "spatialdata/geocoords/CoordSys.hh" // USES CoordSys

#include <assert.h> // USES assert()
#include <stdexcept> // USES std::runtime_error
#include <sstream> // USES std::ostringstream

// ----------------------------------------------------------------------
// Default constructor.
pylith::bc::Dirichlet::Dirichlet(void)
{ // constructor
} // constructor

// ----------------------------------------------------------------------
// Destructor.
pylith::bc::Dirichlet::~Dirichlet(void)
{ // destructor
} // destructor

// ----------------------------------------------------------------------
// Initialize boundary condition.
void
pylith::bc::Dirichlet::initialize(const ALE::Obj<ALE::Mesh>& mesh,
				  const spatialdata::geocoords::CoordSys* cs)
{ // initialize
  assert(0 != _db);
  assert(!mesh.isNull());
  assert(0 != cs);

  // Get points associated with boundary condition
  const ALE::Obj<int_section_type>& groupField = mesh->getIntSection(_label);
  if (groupField.isNull()) {
    std::ostringstream msg;
    msg << "Could not find group of points '" << _label << "' in mesh.";
    throw std::runtime_error(msg.str());
  } // if
  assert(!groupField.isNull());
  const int_section_type::chart_type& chart = groupField->getChart();
  const int numPoints = chart.size();
  _points.resize(numPoints);
  int i = 0;
  for(int_section_type::chart_type::iterator c_iter = chart.begin();
      c_iter != chart.end();
      ++c_iter)
    _points[i] = *c_iter;

  // Get values for degrees of freedom
  const int numFixedDOF = _fixedDOF.size();
  char** valueNames = (numFixedDOF > 0) ? new char*[numFixedDOF] : 0;
  for (int i=0; i < numFixedDOF; ++i) {
    std::ostringstream name;
    name << "dof-" << _fixedDOF[i];
    const int size = 1 + name.str().length();
    valueNames[i] = new char[size];
    strcpy(valueNames[i], name.str().c_str());
  } // for
  _db->open();
  _db->queryVals((const char**) valueNames, numFixedDOF);
  for (int i=0; i < numFixedDOF; ++i) {
    delete[] valueNames[i]; valueNames[i] = 0;
  } // for
  delete[] valueNames; valueNames = 0;

  const ALE::Obj<real_section_type>& coordinates = 
    mesh->getRealSection("coordinates");
  assert(!coordinates.isNull());
  const int spaceDim = cs->spaceDim();

  _values.resize(numPoints*numFixedDOF);
  double_array queryValues(numFixedDOF);
  for (int iPoint=0, i=0; iPoint < numPoints; ++iPoint) {
    // Get coordinates of vertex
    const real_section_type::value_type* vCoords = 
      coordinates->restrictPoint(_points[iPoint]);
    int err = _db->query(&queryValues[0], numFixedDOF, vCoords, spaceDim, cs);
    if (err) {
      std::ostringstream msg;
      msg << "Could not find values at (";
      for (int i=0; i < spaceDim; ++i)
	msg << "  " << vCoords[i];
      msg << ") using spatial database " << _db->label() << ".";
      throw std::runtime_error(msg.str());
    } // if
    for (int iDOF=0; iDOF < numFixedDOF; ++iDOF)
      _values[i++] = queryValues[iDOF];
  } // for
  _db->close();
} // initialize

// ----------------------------------------------------------------------
// Set number of degrees of freedom that are constrained at points in field.
void
pylith::bc::Dirichlet::setConstraintSizes(const ALE::Obj<real_section_type>& field,
					  const ALE::Obj<ALE::Mesh>& mesh)
{ // setConstraintSizes
  assert(!field.isNull());
  assert(!mesh.isNull());

  const int numPoints = _points.size();
  const int numFixedDOF = _fixedDOF.size();
  for (int iPoint=0; iPoint < numPoints; ++iPoint)
    field->setConstraintDimension(_points[iPoint], numFixedDOF);
} // setConstraintSizes

// ----------------------------------------------------------------------
// Set which degrees of freedom are constrained at points in field.
void
pylith::bc::Dirichlet::setConstraints(const ALE::Obj<real_section_type>& field,
				      const ALE::Obj<ALE::Mesh>& mesh)
{ // setConstraints
  assert(!field.isNull());
  assert(!mesh.isNull());

  const int numPoints = _points.size();
  const int numFixedDOF = _fixedDOF.size();
  for (int iPoint=0; iPoint < numPoints; ++iPoint)
    field->setConstraintDof(_points[iPoint], &_fixedDOF[0]);
} // setConstraints

// ----------------------------------------------------------------------
// Set values in field.
void
pylith::bc::Dirichlet::setField(const double t,
				const ALE::Obj<real_section_type>& field,
				const ALE::Obj<ALE::Mesh>& mesh)
{ // setField
  assert(!field.isNull());
  assert(!mesh.isNull());

  const int numPoints = _points.size();
  const int numFixedDOF = _fixedDOF.size();
  for (int iPoint=0, i=0; iPoint < numPoints; ++iPoint, i+=numFixedDOF)
    field->updatePointBC(_points[iPoint], &_values[i]);
} // setField


// End of file 
