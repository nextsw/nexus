// ----------------------------------------------------------------------------
// nexus | Na22Source.cc
//
// Na-22 calibration specific source with plastic support used at LSC.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "Na22Source.h"
#include "MaterialsList.h"
#include "Visibilities.h"

#include <G4Tubs.hh>
#include <G4NistManager.hh>
#include <G4Material.hh>
#include <G4LogicalVolume.hh>
#include <G4PVPlacement.hh>
#include <G4VisAttributes.hh>

#include <CLHEP/Units/SystemOfUnits.h>

namespace nexus {

  using namespace CLHEP;

  Na22Source::Na22Source():
    DiskSource()
  {
    source_diam_ = 3.*mm;
    source_thick_ = .1*mm;
    support_diam_ = 25.33*mm;
    support_thick_ = 6.2*mm;
  }

  Na22Source::~Na22Source()
  {

  }

  void Na22Source::Construct()
  {

    ///Plastic support
    // G4double support_thick = 6.2*mm;
    // G4double support_diam = 25.33*mm;
    G4Tubs* support_solid =
      new G4Tubs("SUPPORT", 0., support_diam_/2., support_thick_/2., 0., twopi);

    G4Material* plastic = materials::PS();
    G4LogicalVolume* support_logic =
      new G4LogicalVolume(support_solid, plastic, "NA22_SOURCE_SUPPORT");

    this->SetLogicalVolume(support_logic);

    // G4double source_thick = .1*mm;
    // G4double source_diam = 3.*mm;
    G4Tubs* source_solid =
      new G4Tubs("SOURCE", 0., source_diam_/2., source_thick_/2., 0., twopi);
    G4Material* sodium22_mat =
      G4NistManager::Instance()->FindOrBuildMaterial("G4_Na");
    G4LogicalVolume* source_logic =
      new G4LogicalVolume(source_solid, sodium22_mat, "NA22");

    new G4PVPlacement(0, G4ThreeVector(0., 0.,  support_thick_/2. - source_thick_/2.),
		      source_logic, "NA22",
		      support_logic, false, 0, false);
    G4VisAttributes source_col = nexus::LightGreen();
    source_col.SetForceSolid(true);
    source_logic->SetVisAttributes(source_col);

    return;

  }

  // G4double Na22Source::GetSourceDiameter()
  // {
  //   return source_diam_;
  // }

  // G4double Na22Source::GetSourceThickness()
  // {
  //   return source_thick_;
  // }

  //  G4double Na22Source::GetSupportDiameter()
  // {
  //   return support_diam_;
  // }

  // G4double Na22Source::GetSupportThickness()
  // {
  //   return support_thick_;
  // }

}
