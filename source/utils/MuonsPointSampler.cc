// ---------------------------------------------------------------------------- 
//  //
//  \brief Point Sampler on the surface of a rectangular used for muon generation.
//
//  Author: Neus Lopez March <neus.lopez@ific.uv.es>
//        
//  Created: 30 Jan 2015
//
//  Copyright (c) 2015 NEXT Collaboration
// ----------------------------------------------------------------------------

#include "MuonsPointSampler.h"
#include <Randomize.hh>
#include <G4RunManager.hh>
#include "CLHEP/Units/PhysicalConstants.h"
#include <G4Box.hh>
#include <G4VSolid.hh>
#include <vector>


namespace nexus {

  using namespace CLHEP;
  
  MuonsPointSampler::MuonsPointSampler
  (G4double x, G4double yPoint, G4double z):
    x_(x),yPoint_(yPoint),z_(z)
  {
  }
  
  
  G4ThreeVector MuonsPointSampler::GenerateVertex()
  {
    
    // Get a random point in the surface of the muon plane
    G4ThreeVector point = GetXZPointInMuonsPlane();
    
    return point;
  }
  
  G4ThreeVector MuonsPointSampler::GetXZPointInMuonsPlane()
  {
    
    // y is fixed
    G4double x = -x_ + G4UniformRand()*2*x_;
    G4double z = -z_ + G4UniformRand()*2*z_;
    
    G4ThreeVector mypoint(x, yPoint_, z);
    
    // std::cout<<"Generating Muons in: "<<x<<" , "<<y<<" , "<<z<<std::endl;
    return mypoint;
  }
  
} // end namespace nexus
