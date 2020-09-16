// ----------------------------------------------------------------------------
// nexus | NextDemoTrackingPlane1.h
//
// Tracking plane 1 of the Demo++ geometry.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#ifndef NEXT100_TRACKING_PLANE_1_H
#define NEXT100_TRACKING_PLANE_1_H

#include "BaseGeometry.h"
#include <G4ThreeVector.hh>
#include <vector>

class G4VPhysicalVolume;
class G4GenericMessenger;
class G4Navigator;

namespace nexus {

  class NextDemoSiPMBoard;
  class BoxPointSampler;

  // Geometry of the tracking plane of the Demo++ detector

  class NextDemoTrackingPlane1: public BaseGeometry
  {
  public:
    // Constructor
    NextDemoTrackingPlane1();

    // Destructor
    ~NextDemoTrackingPlane1();
    
    void SetMotherPhysicalVolume(G4VPhysicalVolume* mother_phys);
    
    void Construct() override;
    
    G4ThreeVector GenerateVertex(const G4String&) const override;

  private:
    void GenerateBoardPositions(G4double board_posz);

  private:
    G4bool verbosity_;
    G4bool visibility_;

    const G4double plate_side_, plate_thickness_, plate_hole_side_;

    G4int num_boards_;
    NextDemoSiPMBoard* sipm_board_;
    G4ThreeVector board_dimensions_;
    std::vector<G4ThreeVector> board_pos_;

    BoxPointSampler* plate_gen_;

    G4VPhysicalVolume* mother_phys_;

    G4GenericMessenger* msg_;

    // Geometry Navigator
    G4Navigator* geom_navigator_;
  };

  inline void NextDemoTrackingPlane1::SetMotherPhysicalVolume(G4VPhysicalVolume* mother_phys)
    { mother_phys_ = mother_phys; }

} // namespace nexus

#endif
