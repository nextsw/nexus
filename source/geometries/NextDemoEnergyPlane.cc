// ----------------------------------------------------------------------------
// nexus | NextDemoEnergyPlane.cc
//
// Energy plane geometry of the DEMO++ detector.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#include "NextDemoEnergyPlane.h"
#include "MaterialsList.h"
#include "OpticalMaterialProperties.h"
#include "Visibilities.h"

#include <G4GenericMessenger.hh>
#include <G4PVPlacement.hh>
#include <G4VisAttributes.hh>
#include <G4Material.hh>
#include <G4LogicalVolume.hh>
#include <G4Tubs.hh>
#include <G4SubtractionSolid.hh>
#include <G4UnionSolid.hh>
#include <G4OpticalSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4NistManager.hh>
#include <G4VPhysicalVolume.hh>
#include <G4TransportationManager.hh>
#include <Randomize.hh>
#include <G4RotationMatrix.hh>

namespace nexus {

  using namespace CLHEP;

  NextDemoEnergyPlane::NextDemoEnergyPlane():
    gate_support_surface_dist_ (445.5 * mm),
    num_PMTs_ (3),
    carrier_plate_thickness_ (45. * mm),
    carrier_plate_diam_ ( 234. * mm),
    carrier_plate_central_hole_diam_ (10. * mm),
    pmt_hole_length_ (18.434 * cm),
    pmt_hole_diam_ (9.6 * cm),
    pmt_flange_length_ (19.5 * mm),
    sapphire_window_thickness_ (6. * mm),
    sapphire_window_diam_ (85. * mm),
    pedot_coating_thickness_ (3. * micrometer),
    optical_pad_thickness_ (1.0 * mm),
    pmt_base_diam_ (47. * mm),
    pmt_base_thickness_ (5. * mm),
    tpb_thickness_ (1.* micrometer),
    visibility_ (1),
    verbosity_ (0)
  {
    /// Initializing the geometry navigator (used in vertex generation) ///
    geom_navigator_ =
      G4TransportationManager::GetTransportationManager()->GetNavigatorForTracking();

    /// Messenger ///
    msg_ = new G4GenericMessenger(this, "/Geometry/NextDemo/",
                                  "Control commands of the NextDemo geometry.");
    msg_->DeclareProperty("energy_plane_vis", visibility_,
                          "Energy Plane Visibility");

    msg_->DeclareProperty("energy_plane_verbosity", verbosity_,
                          "Energy Plane verbosity");

    pmt_ = new PmtR11410();
  }


  NextDemoEnergyPlane::~NextDemoEnergyPlane()
  {
    delete msg_;
    delete pmt_;
  }


  void NextDemoEnergyPlane::SetMotherLogicalVolume(G4LogicalVolume* mother_logic)
  {
    mother_logic_ = mother_logic;
  }


  void NextDemoEnergyPlane::SetGateSapphireSurfaceDistance(G4double dist)
  {
    gate_sapphire_dist_ = dist;
  }


  void NextDemoEnergyPlane::Construct()
  {
    GeneratePmtPositions();

    /// Support Plate ///
    G4Tubs* carrier_plate_nh_solid =
      new G4Tubs("EP_PLATE_NH", 0., carrier_plate_diam_/2.,
                 carrier_plate_thickness_/2., 0., twopi);

    /// Making central hole for vacuum manifold ///
    G4Tubs* carrier_plate_central_hole_solid =
      new G4Tubs("EP_PLATE_CENTRAL_HOLE", 0.,
                 carrier_plate_central_hole_diam_/2.,
                 (carrier_plate_thickness_+1.*cm)/2., 0., twopi);

    G4SubtractionSolid* carrier_plate_solid =
      new G4SubtractionSolid("EP_PLATE", carrier_plate_nh_solid,
                             carrier_plate_central_hole_solid, 0,
                             G4ThreeVector(0., 0., 0.) );

    /// Making PMT holes ///
    G4Tubs* carrier_plate_pmt_hole_solid =
      new G4Tubs("EP_PLATE_PMT_HOLE", 0., pmt_hole_diam_/2.,
                 (carrier_plate_thickness_+1.*cm)/2., 0., twopi);

    for (int i=0; i<num_PMTs_; i++) {
      carrier_plate_solid =
        new G4SubtractionSolid("EP_PLATE", carrier_plate_solid,
                               carrier_plate_pmt_hole_solid, 0,
                               pmt_positions_[i]);
    }

    G4LogicalVolume* carrier_plate_logic =
      new G4LogicalVolume(carrier_plate_solid,
                          G4NistManager::Instance()->FindOrBuildMaterial("G4_Al"),
                          "EP_PLATE");

    G4double carrier_plate_posz =
      GetELzCoord() + gate_support_surface_dist_ + carrier_plate_thickness_/2.;
    new G4PVPlacement(0, G4ThreeVector(0., 0., carrier_plate_posz),
                      carrier_plate_logic, "EP_PLATE", mother_logic_,
                      false, 0, false);


    /// Assign optical properties to materials ///
    G4Material* tpb = MaterialsList::TPB();
    tpb->SetMaterialPropertiesTable(OpticalMaterialProperties::TPB());

    G4Material* sapphire = MaterialsList::Sapphire();
    sapphire->SetMaterialPropertiesTable(OpticalMaterialProperties::Sapphire());

    G4Material* vacuum =
      G4NistManager::Instance()->FindOrBuildMaterial("G4_Galactic");
    vacuum->SetMaterialPropertiesTable(OpticalMaterialProperties::Vacuum());

    G4Material* optical_coupler = MaterialsList::OpticalSilicone();
    optical_coupler->SetMaterialPropertiesTable(OpticalMaterialProperties::OptCoupler());

    G4Material* pedot = MaterialsList::PEDOT();
    pedot->SetMaterialPropertiesTable(OpticalMaterialProperties::PEDOT());

    /// A volume of vacuum is constructed to hold the elements ///
    /// that are replicated ///
    G4Tubs* pmt_hole_solid =
      new G4Tubs("PMT_HOLE", 0., pmt_hole_diam_/2., pmt_hole_length_/2.,
                 0., twopi);
    G4LogicalVolume* pmt_hole_logic =
      new G4LogicalVolume(pmt_hole_solid, vacuum, "PMT_HOLE");

    G4Tubs* pmt_flange_solid =
      new G4Tubs("PMT_FLANGE", sapphire_window_diam_/2.,
                 pmt_hole_diam_/2., pmt_flange_length_/2., 0., twopi);

    G4LogicalVolume* pmt_flange_logic =
      new G4LogicalVolume(pmt_flange_solid,
                          G4NistManager::Instance()->FindOrBuildMaterial("G4_Cu"),
			  "PMT_FLANGE");

    G4ThreeVector flange_pos(0., 0., -pmt_hole_length_/2. + pmt_flange_length_/2.);
    new G4PVPlacement(0, flange_pos, pmt_flange_logic,
      		      "PMT_FLANGE", pmt_hole_logic, false, 0, false);


    /// Sapphire window ///
    G4Tubs* sapphire_window_solid =
      new G4Tubs("SAPPHIRE_WINDOW", 0., sapphire_window_diam_/2.,
                 sapphire_window_thickness_/2., 0., twopi);

    G4LogicalVolume* sapphire_window_logic =
      new G4LogicalVolume(sapphire_window_solid, sapphire, "SAPPHIRE_WINDOW");

    G4double window_zpos = -pmt_hole_length_/2. + sapphire_window_thickness_/2. ;
    new G4PVPlacement(0, G4ThreeVector(0., 0., window_zpos),
                      sapphire_window_logic, "SAPPHIRE_WINDOW",
                      pmt_hole_logic, false, 0, false);

    /// TPB coating on pedot ///
    G4Tubs* tpb_solid =
      new G4Tubs("SAPPHIRE_WNDW_TPB", 0., sapphire_window_diam_/2,
                 tpb_thickness_/2., 0., twopi);
    G4LogicalVolume* tpb_logic =
      new G4LogicalVolume(tpb_solid, tpb, "SAPPHIRE_WNDW_TPB");

    G4double tpb_posz = - sapphire_window_thickness_/2. + tpb_thickness_/2.;
    new G4PVPlacement(0, G4ThreeVector(0., 0., tpb_posz), tpb_logic,
                      "SAPPHIRE_WNDW_TPB", sapphire_window_logic,
                      false, 0, false);

    /// PEDOT coating on sapphire windows ///
    G4Tubs* pedot_coating_solid =
      new G4Tubs("SAPPHIRE_WNDW_PEDOT", 0., sapphire_window_diam_/2.,
                 pedot_coating_thickness_/2., 0., twopi);

    G4LogicalVolume* pedot_coating_logic =
      new G4LogicalVolume(pedot_coating_solid, pedot, "SAPPHIRE_WNDW_PEDOT");

    G4double pedot_coating_zpos =
      - sapphire_window_thickness_/2. + tpb_thickness_ + pedot_coating_thickness_/2.;
    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., pedot_coating_zpos),
                      pedot_coating_logic, "SAPPHIRE_WNDW_PEDOT",
                      sapphire_window_logic, false, 0, false);

    /// Optical pad ///
    G4Tubs* optical_pad_solid =
      new G4Tubs("OPTICAL_PAD", 0., sapphire_window_diam_/2.,
                 optical_pad_thickness_/2., 0., twopi);

    G4LogicalVolume* optical_pad_logic =
      new G4LogicalVolume(optical_pad_solid, optical_coupler, "OPTICAL_PAD");

    G4double pad_zpos =
      window_zpos + sapphire_window_thickness_/2. + optical_pad_thickness_/2.;
    new G4PVPlacement(0, G4ThreeVector(0.,0.,pad_zpos), optical_pad_logic,
		      "OPTICAL_PAD", pmt_hole_logic, false, 0, false);


    /// PMT ///
    pmt_->SetSensorDepth(4);
    pmt_->Construct();
    G4LogicalVolume* pmt_logic = pmt_->GetLogicalVolume();
    G4double pmt_rel_zpos = pmt_->GetRelPosition().z();
    G4double pmt_zpos = pad_zpos + optical_pad_thickness_/2. + pmt_rel_zpos;
    G4ThreeVector pmt_pos = G4ThreeVector(0., 0., pmt_zpos);

    G4RotationMatrix* pmt_rot = new G4RotationMatrix();
    pmt_rot->rotateY(pi);

    new G4PVPlacement(G4Transform3D(*pmt_rot, pmt_pos), pmt_logic,
		      "PMT", pmt_hole_logic, false, 0, false);


    /// PMT base ///
    G4Tubs* pmt_base_solid =
      new G4Tubs("PMT_BASE", 0., pmt_base_diam_/2., pmt_base_thickness_,
                 0.,twopi);
    G4LogicalVolume* pmt_base_logic =
      new G4LogicalVolume(pmt_base_solid,
                          G4NistManager::Instance()->FindOrBuildMaterial("G4_KAPTON"),
			  "PMT_BASE");

    G4double pmt_base_pos = pmt_hole_length_/2. - pmt_base_thickness_;
    new G4PVPlacement(0, G4ThreeVector(0.,0., pmt_base_pos),
		      pmt_base_logic, "PMT_BASE", pmt_hole_logic,
                      false, 0, false);


    G4double pmt_hole_zpos = GetELzCoord() + gate_sapphire_dist_
      + pmt_hole_length_/2.;

    G4ThreeVector pos;
    G4ThreeVector tpb_pos;
    if (verbosity_) G4cout << "*** XY position of PMTs ***" << G4endl;

    for (int i=0; i<num_PMTs_; i++) {
      pos = pmt_positions_[i];
      if (verbosity_) G4cout << pos.getX() << ", " << pos.getY() << G4endl;
      pos.setZ(pmt_hole_zpos);
      new G4PVPlacement(0, pos, pmt_hole_logic,
			"PMT_HOLE", mother_logic_, false, i, false);
    }


    /// Visibilities ///
    pmt_hole_logic->SetVisAttributes(G4VisAttributes::Invisible);
    if (visibility_) {
      G4VisAttributes vacuum_col = nexus::Red();
      pmt_hole_logic->SetVisAttributes(vacuum_col);
      G4VisAttributes copper_col = CopperBrown();
      // copper_col.SetForceSolid(true);
      carrier_plate_logic->SetVisAttributes(copper_col);
      pmt_flange_logic->SetVisAttributes(copper_col);
      G4VisAttributes sapphire_col = nexus::Lilla();
      sapphire_col.SetForceSolid(true);
      sapphire_window_logic->SetVisAttributes(sapphire_col);
      G4VisAttributes pad_col = nexus::LightGreen();
      pad_col.SetForceSolid(true);
      optical_pad_logic->SetVisAttributes(pad_col);
      G4VisAttributes base_col = nexus::Yellow();
      base_col.SetForceSolid(true);
      pmt_base_logic->SetVisAttributes(base_col);
      G4VisAttributes tpb_col = nexus::LightBlue();
      tpb_col.SetForceSolid(true);
      tpb_logic->SetVisAttributes(tpb_col);
    } else {
      carrier_plate_logic->SetVisAttributes(G4VisAttributes::Invisible);
      pmt_flange_logic->SetVisAttributes(G4VisAttributes::Invisible);
      sapphire_window_logic->SetVisAttributes(G4VisAttributes::Invisible);
      optical_pad_logic->SetVisAttributes(G4VisAttributes::Invisible);
      pmt_base_logic->SetVisAttributes(G4VisAttributes::Invisible);
      tpb_logic->SetVisAttributes(G4VisAttributes::Invisible);
    }
  }


  void NextDemoEnergyPlane::GeneratePmtPositions()
  {
    /// Compute and store the XY positions of PMTs in the support plate ///
    G4int total_positions = 0;
    G4ThreeVector position(0.,0.,0.);

    G4double rad = 7 * cm;
    G4int num_places = 3;
    G4int step_deg = 360.0 / num_places;
    for (G4int place=0; place<num_places; place++) {
	G4double angle = place * step_deg + 36. ;
	position.setX(-rad * sin(angle * deg));
	position.setY(rad * cos(angle * deg));
	pmt_positions_.push_back(position);
	total_positions++;
    }

    if (total_positions != num_PMTs_) {
      G4cout << "\n\nERROR: Number of PMTs doesn't match with number of positions calculated\n";
      exit(0);
    }

  }


}