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

#include "BruneSlipFn.hh" // implementation of object methods

#include "pylith/feassemble/ParameterManager.hh" // USES ParameterManager

#include "spatialdata/spatialdb/SpatialDB.hh" // USES SpatialDB
#include "spatialdata/geocoords/CoordSys.hh" // USES CoordSys

#include <assert.h> // USES assert()
#include <sstream> // USES std::ostringstream
#include <stdexcept> // USES std::runtime_error

// ----------------------------------------------------------------------
// Default constructor.
pylith::faults::BruneSlipFn::BruneSlipFn(void) :
  _dbFinalSlip(0),
  _dbSlipTime(0),
  _dbPeakRate(0)
{ // constructor
} // constructor

// ----------------------------------------------------------------------
// Destructor.
pylith::faults::BruneSlipFn::~BruneSlipFn(void)
{ // destructor
  _dbFinalSlip = 0;
  _dbSlipTime = 0;
  _dbPeakRate = 0;
} // destructor

// ----------------------------------------------------------------------
// Copy constructor.
pylith::faults::BruneSlipFn::BruneSlipFn(const BruneSlipFn& f) :
  SlipTimeFn(f),
  _slipField(f._slipField),
  _dbFinalSlip(f._dbFinalSlip),
  _dbSlipTime(f._dbSlipTime),
  _dbPeakRate(f._dbPeakRate)
{ // copy constructor
} // copy constructor

// ----------------------------------------------------------------------
// Initialize slip time function.
void
pylith::faults::BruneSlipFn::initialize(const ALE::Obj<Mesh>& mesh,
					const ALE::Obj<Mesh>& faultMesh,
					const spatialdata::geocoords::CoordSys* cs)
{ // initialize
  assert(!mesh.isNull());
  assert(!faultMesh.isNull());
  assert(0 != cs);
  assert(0 != _dbFinalSlip);
  assert(0 != _dbSlipTime);
  assert(0 != _dbPeakRate);

  // Get fault vertices
  const ALE::Obj<Mesh::label_sequence>& vertices = faultMesh->depthStratum(0);

  // Create and allocate sections for parameters
  delete _parameters; 
  _parameters = new feassemble::ParameterManager(faultMesh);
  if (0 == _parameters)
    throw std::runtime_error("Could not create manager for parameters of "
			     "Brune slip time function.");
  assert(0 != _parameters);
  
  // Parameter: final slip
  _parameters->addReal("final slip");
  const ALE::Obj<real_section_type>& finalSlip = 
    _parameters->getReal("final slip");
  assert(!finalSlip.isNull());
  finalSlip->setFiberDimension(faultMesh->depthStratum(0), 3);
  faultMesh->allocate(finalSlip);

  // Parameter: slip initiation time
  _parameters->addReal("slip time");
  const ALE::Obj<real_section_type>& slipTime = 
    _parameters->getReal("slip time");
  assert(!slipTime.isNull());
  slipTime->setFiberDimension(faultMesh->depthStratum(0), 1);
  faultMesh->allocate(slipTime);

  // Parameter: peak slip rate
  _parameters->addReal("peak rate");
  const ALE::Obj<real_section_type>& peakRate = 
    _parameters->getReal("peak rate");
  assert(!peakRate.isNull());
  peakRate->setFiberDimension(faultMesh->depthStratum(0), 1);
  faultMesh->allocate(peakRate);
  
  // Open databases and set query values
  _dbFinalSlip->open();
  const char* slipValues[] = {"strike-slip", "dip-slip", "fault-opening"};
  _dbFinalSlip->queryVals(slipValues, 3);

  _dbSlipTime->open();
  const char* slipTimeValues[] = {"slip-time"};
  _dbSlipTime->queryVals(slipTimeValues, 1);

  _dbPeakRate->open();
  const char* peakRateValues[] = {"slip-rate"};
  _dbPeakRate->queryVals(peakRateValues, 1);

  // Get coordinates of vertices
  const ALE::Obj<real_section_type>& coordinates = 
    mesh->getRealSection("coordinates");
  assert(!coordinates.isNull());
  const int spaceDim = cs->spaceDim();

  // Query databases for parameters
  double slipData[3];
  double slipTimeData;
  double peakRateData;
  const Mesh::label_sequence::iterator vBegin = vertices->begin();
  const Mesh::label_sequence::iterator vEnd = vertices->end();
  for (Mesh::label_sequence::iterator v_iter=vBegin; 
       v_iter != vEnd;
       ++v_iter) {
    // Get coordinates of vertex
    const real_section_type::value_type* vCoords = 
      coordinates->restrictPoint(*v_iter);
    
    int err = _dbFinalSlip->query(&slipData[0], 3, vCoords, spaceDim, cs);
    if (err) {
      std::ostringstream msg;
      msg << "Could not find final slip at (";
      for (int i=0; i < spaceDim; ++i)
	msg << "  " << vCoords[i];
      msg << ") using spatial database " << _dbFinalSlip->label() << ".";
      throw std::runtime_error(msg.str());
    } // if
    finalSlip->updatePoint(*v_iter, &slipData[0]);

    err = _dbSlipTime->query(&slipTimeData, 1, vCoords, spaceDim, cs);
    if (err) {
      std::ostringstream msg;
      msg << "Could not find slip initiation time at (";
      for (int i=0; i < spaceDim; ++i)
	msg << "  " << vCoords[i];
      msg << ") using spatial database " << _dbSlipTime->label() << ".";
      throw std::runtime_error(msg.str());
    } // if
    slipTime->updatePoint(*v_iter, &slipTimeData);

    err = _dbPeakRate->query(&peakRateData, 1, vCoords, spaceDim, cs);
    if (err) {
      std::ostringstream msg;
      msg << "Could not find peak slip rate at (";
      for (int i=0; i < spaceDim; ++i)
	msg << "  " << vCoords[i];
      msg << ") using spatial database " << _dbPeakRate->label() << ".";
      throw std::runtime_error(msg.str());
    } // if
    peakRate->updatePoint(*v_iter, &peakRateData);
  } // for

  // Close databases
  _dbFinalSlip->close();
  _dbSlipTime->close();
  _dbPeakRate->close();

  // Allocate slip field
  _slipField->setFiberDimension(faultMesh->depthStratum(0), 3);
  faultMesh->allocate(_slipField);
} // initialize

// ----------------------------------------------------------------------
// Get slip on fault surface at time t.
const ALE::Obj<pylith::real_section_type>&
pylith::faults::BruneSlipFn::slip(const double t,
				  const ALE::Obj<Mesh>& faultMesh)
{ // slip
  assert(0 != _parameters);
  assert(!_slipField.isNull());
  
  // Get fault vertices
  const ALE::Obj<Mesh::label_sequence>& vertices = faultMesh->depthStratum(0);

  // Get parameters
  const ALE::Obj<real_section_type>& finalSlip = 
    _parameters->getReal("final slip");
  assert(!finalSlip.isNull());

  const ALE::Obj<real_section_type>& slipTime = 
    _parameters->getReal("slip time");
  assert(!slipTime.isNull());

  const ALE::Obj<real_section_type>& peakRate = 
    _parameters->getReal("peak rate");
  assert(!peakRate.isNull());

  double slipValues[3];
  const Mesh::label_sequence::iterator vBegin = vertices->begin();
  const Mesh::label_sequence::iterator vEnd = vertices->end();
  for (Mesh::label_sequence::iterator v_iter=vBegin;
       v_iter != vEnd;
       ++v_iter) {
    // Get values of parameters at vertex
    const real_section_type::value_type* vFinalSlip = 
      finalSlip->restrictPoint(*v_iter);
    const real_section_type::value_type* vSlipTime = 
      slipTime->restrictPoint(*v_iter);
    const real_section_type::value_type* vPeakRate = 
      peakRate->restrictPoint(*v_iter);

    const double vFinalSlipMag = sqrt(vFinalSlip[0]*vFinalSlip[0] +
				      vFinalSlip[1]*vFinalSlip[1] +
				      vFinalSlip[2]*vFinalSlip[2]);
    const double vSlip = _slip(t-vSlipTime[0], vFinalSlipMag, vPeakRate[0]);
    const double scale = vSlip / vFinalSlipMag;
    slipValues[0] = scale * vFinalSlip[0];
    slipValues[1] = scale * vFinalSlip[1];
    slipValues[2] = scale * vFinalSlip[2];

    // Update field
    _slipField->updatePoint(*v_iter, &slipValues[0]);
  } // for

  return _slipField;
} // slip


// End of file 
