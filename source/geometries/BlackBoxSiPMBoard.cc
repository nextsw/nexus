// -----------------------------------------------------------------------------
// nexus | BlackBoxSiPMBoard.cc
//
// SiPM Board used in BlackBox.
// -----------------------------------------------------------------------------

#include "BlackBoxSiPMBoard.h"

#include "MaterialsList.h"
#include "SiPMSensl.h"
#include "OpticalMaterialProperties.h"
#include "BoxPointSampler.h"
#include "Visibilities.h"

#include <G4GenericMessenger.hh>
#include <G4Box.hh>
#include <G4Tubs.hh>
#include <G4VisAttributes.hh>
#include <G4LogicalVolume.hh>
#include <G4SubtractionSolid.hh>
#include <G4PVPlacement.hh>
#include <G4RotationMatrix.hh>
#include <G4Material.hh>
#include <G4NistManager.hh>
#include <G4OpticalSurface.hh>
#include <G4LogicalSkinSurface.hh>
#include <G4LogicalBorderSurface.hh>

#include <CLHEP/Units/SystemOfUnits.h>

using namespace nexus;


BlackBoxSiPMBoard::BlackBoxSiPMBoard():
  GeometryBase     (),
  verbosity_       (true),
  sipm_verbosity_  (true),
  visibility_      (true),
  num_columns_     (8),
  num_rows_        (8),
  num_sipms_       (num_rows_ * num_columns_),
  sipm_pitch_      (10.  * mm),
  side_reduction_  (0.5  * mm),
  kapton_thickn_   (0.3  * mm),
  mask_thickn_     (),
  membrane_thickn_ (),
  coating_thickn_  (),
  hole_diam_       (),
  hole_x_          (),
  hole_y_          (),
  mother_phys_     (nullptr),
  kapton_gen_      (nullptr)
{
  msg_ = new G4GenericMessenger(this, "/Geometry/BlackBox/",
                                "Control commands of the BlackBox geometry.");

  msg_->DeclareProperty("sipm_board_verbosity", verbosity_,
                        "BlackBoxSiPMBoard verbosity");

  msg_->DeclareProperty("sipm_verbosity", sipm_verbosity_,
                        "BlackBox SiPMs verbosity");

  msg_->DeclareProperty("sipm_board_vis", visibility_,
                        "BlackBoxSiPMBoard visibility.");

  sipm_ = new SiPMSensl;
}


BlackBoxSiPMBoard::~BlackBoxSiPMBoard()
{
  delete msg_;
  delete kapton_gen_;
  delete sipm_;
}


void BlackBoxSiPMBoard::Construct()
{

  /// Make sure the mother physical volume is actually valid
  if (!mother_phys_)
    G4Exception("[BlackBoxSiPMBoard]", "Construct()",
                FatalException, "Mother physical volume is a nullptr.");

  G4Material* mother_gas = mother_phys_->GetLogicalVolume()->GetMaterial();


  /// Board configuration checks
  // Coating require membranes
  if (coating_thickn_ > 0.)
    if (membrane_thickn_ == 0.)
      G4Exception("[BlackBoxSiPMBoard]", "Construct()", FatalException,
      "Coating require membranes");

  // Membranes require masks
  if (membrane_thickn_ > 0.)
    if (mask_thickn_ == 0.)
      G4Exception("[BlackBoxSiPMBoard]", "Construct()", FatalException,
      "Membranes require masks");

  // Masks require holes
  if (mask_thickn_ > 0.)
    if (hole_diam_ == 0. && hole_x_ ==0.)
      G4Exception("[BlackBoxSiPMBoard]", "Construct()", FatalException,
      "Masks require holes");

  sipm_->SetSensorDepth(2);
  sipm_->SetMotherDepth(3);
  sipm_->SetNamingOrder(1000);
  sipm_->Construct();

  /// Board-Wrapper volume that contains all other elements
  G4String board_name = "SIPM_BOARD";

  // Calculating board wrapper dimensions
  G4double board_size_x = num_columns_ * sipm_pitch_ - 2. * side_reduction_;
  G4double board_size_y = num_rows_    * sipm_pitch_ - 2. * side_reduction_;
  G4double board_size_z = kapton_thickn_ + coating_thickn_ +
                          std::max(sipm_->GetDimensions().z(), mask_thickn_);

  board_size_ = G4ThreeVector(board_size_x, board_size_y, board_size_z);
  G4cout << "board_size " << board_size_ << G4endl;
  G4cout << "sipm thickness " << sipm_->GetDimensions().z() << G4endl;
  G4Box* board_solid = new G4Box(board_name, board_size_x/2.,
                                 board_size_y/2., board_size_z/2.);

  G4LogicalVolume* board_logic =
    new G4LogicalVolume(board_solid, mother_gas, board_name);

  GeometryBase::SetLogicalVolume(board_logic);


  /// Kapton
  G4String kapton_name = "KAPTON_BOARD";

  G4double kapton_posz = - board_size_z/2. + kapton_thickn_/2.;

  G4Box* kapton_solid = new G4Box(kapton_name, board_size_x/2.,
                                  board_size_y/2., kapton_thickn_/2.);

  G4LogicalVolume* kapton_logic =
    new G4LogicalVolume(kapton_solid,
                        G4NistManager::Instance()->FindOrBuildMaterial("G4_KAPTON"),
                        kapton_name);

  new G4PVPlacement(nullptr, G4ThreeVector(0., 0., kapton_posz), kapton_logic,
                    kapton_name, board_logic, false, 0, true);

  // Generate SiPM positions
  GenerateSiPMPositions();

  // SiPM placement.
  G4RotationMatrix* sipm_rot = new G4RotationMatrix();
  sipm_rot->rotateY(pi);
  G4double sipm_posz = - board_size_z/2. + kapton_thickn_ + sipm_->GetDimensions().z()/2.;
  G4ThreeVector sipm_pos;
  for (G4int sipm_id=0; sipm_id<num_sipms_; sipm_id++) {
       sipm_pos = sipm_positions_[sipm_id] + G4ThreeVector(0., 0., sipm_posz);
       G4cout << "SiPM" << sipm_id << ":" << sipm_pos << G4endl;
       //new G4PVPlacement(nullptr, sipm_pos, sipm_->GetLogicalVolume(),
        //                 sipm_->GetLogicalVolume()->GetName(), board_logic, false, sipm_id, true);
       new G4PVPlacement(sipm_rot, sipm_pos, sipm_->GetLogicalVolume(),
                         sipm_->GetLogicalVolume()->GetName(), board_logic, false, sipm_id, true);
  }

  /// Teflon Masks
  G4LogicalVolume* mask_logic;
  G4LogicalVolume* membrane_logic;
  G4LogicalVolume* coating_logic;

  if (mask_thickn_!=0.){
    /// Create solid mask (no holes for sipms)
    G4String mask_name = "BOARD_MASK";

    G4double mask_posz = - board_size_z/2. + kapton_thickn_ + mask_thickn_/2.;

    G4Box* mask_solid = new G4Box(mask_name, board_size_x/2.,
                                board_size_y/2., mask_thickn_/2.);

    /// Define mask holes.

    G4VSolid* hole_solid = nullptr;

    if (hole_diam_!=0.){
      //hole_solid = new G4Tubs("HOLE", 0., hole_diam_/2., mask_thickn_/2., 0., 360.*deg);}
      hole_solid = new G4Tubs("HOLE", 0., hole_diam_/2., mask_thickn_, 0., 360.*deg);}

    else {
      //hole_solid = new G4Box("HOLE", hole_x_/2., hole_y_/2., mask_thickn_/2.);}
      hole_solid = new G4Box("HOLE", hole_x_/2., hole_y_/2., mask_thickn_);}


    /// Create mask with holes.
    G4ThreeVector hole_pos = sipm_positions_[0];
    G4SubtractionSolid* mask_with_holes =
                        new G4SubtractionSolid("BOARD_MASK", mask_solid,
                                               hole_solid, 0, hole_pos);
    for (G4int sipm_id=1; sipm_id<num_sipms_; sipm_id++) {
         hole_pos = sipm_positions_[sipm_id];
         G4cout << "hole" << sipm_id << ":" << hole_pos << G4endl;

         mask_with_holes = new G4SubtractionSolid("BOARD_MASK", mask_with_holes,
                                                  hole_solid, 0, hole_pos);
    }
    mask_logic =
      new G4LogicalVolume(mask_with_holes, G4NistManager::Instance()->FindOrBuildMaterial("G4_TEFLON"),
                          mask_name);

    // Adding the optical surface
    G4OpticalSurface* mask_opsurf =
      new G4OpticalSurface(mask_name, unified, ground, dielectric_metal);
    mask_opsurf->SetMaterialPropertiesTable(OpticalMaterialProperties::PTFE());
    new G4LogicalSkinSurface(mask_name + "_OPSURF", mask_logic, mask_opsurf);

    new G4PVPlacement(nullptr, G4ThreeVector(0., 0., mask_posz), mask_logic,
                      mask_name, board_logic, false, 0, true);

    /// Membranes
    if (membrane_thickn_ > 0. && mask_thickn_ >= membrane_thickn_+ sipm_->GetDimensions().z()){
        G4String membrane_name = "BOARD_MASK_MEMB";

        //G4double membrane_posz = mask_thickn_/2. - membrane_thickn_/2.;
        G4double membrane_posz = - board_size_z/2. + kapton_thickn_ + mask_thickn_ - membrane_thickn_/2.;

        G4Tubs* membrane_solid = new G4Tubs(membrane_name, 0., hole_diam_/2.,
                                          membrane_thickn_/2., 0, 360.*deg);

        membrane_logic =
          //new G4LogicalVolume(membrane_solid, G4NistManager::Instance()->FindOrBuildMaterial("G4_TEFLON"),
          //                    membrane_name);
          new G4LogicalVolume(membrane_solid, mother_gas, membrane_name);

        G4ThreeVector membrane_pos;
        for (G4int sipm_id=0; sipm_id<num_sipms_; sipm_id++) {
             membrane_pos = sipm_positions_[sipm_id]+G4ThreeVector(0., 0., membrane_posz);
             new G4PVPlacement(nullptr, membrane_pos, membrane_logic,
                               membrane_name, board_logic, false, 0, true);
        }
     }
     /// Coating
     if (coating_thickn_ > 0.) {
         G4String coating_name = "BOARD_COATING";

         G4double coating_posz = -board_size_z/2. + kapton_thickn_ + mask_thickn_ + coating_thickn_/2.;

         G4Box* coating_solid = new G4Box(coating_name, board_size_x/2.,
                                       board_size_y/2., coating_thickn_/2.);

         G4Material* tpb = MaterialsList::TPB();
         tpb->SetMaterialPropertiesTable(OpticalMaterialProperties::TPB());

         coating_logic = new G4LogicalVolume(coating_solid, tpb, coating_name);

         G4PVPlacement* coating_phys =
           new G4PVPlacement(nullptr, G4ThreeVector(0.,0.,coating_posz), coating_logic,
                             coating_name, board_logic, false, 0, true);

         // Optical surface
         G4OpticalSurface* coating_opsurf =
           new G4OpticalSurface(coating_name + "_OPSURF", glisur, ground,
                            dielectric_dielectric, .01);

         new G4LogicalBorderSurface("TEFLON_WLS_GAS_OPSURF", coating_phys,
                                   mother_phys_, coating_opsurf);
         new G4LogicalBorderSurface("GAS_TEFLON_WLS_OPSURF", mother_phys_,
                                   coating_phys, coating_opsurf);
     }
  }


  /// VERTEX GENERATOR
  kapton_gen_ = new BoxPointSampler(board_size_x, board_size_y, kapton_thickn_, 0.,
                                    G4ThreeVector(0., 0., -board_size_z/2. + kapton_thickn_/2.),
                                    nullptr);


  /// VERBOSITY
  if (verbosity_)
    G4cout << "* SiPM board size:    " << board_size_      << G4endl;
    G4cout << "* " << num_sipms_ << " SiPMs from Sensl"    << G4endl;

    //if (sipm_verbosity_) {
    //  for (G4int sipm_num=0; sipm_num<num_sipms_; sipm_num++)
    //    G4cout << "* SiPM " << sipm_num << " position: " << sipm_positions_[sipm_num] << G4endl;
    //}

    G4cout << "* Kapton thickness:   " << kapton_thickn_   << G4endl;
    G4cout << "* Mask thickness:     " << mask_thickn_     << G4endl;
    G4cout << "* Mask hole diameter: " << hole_diam_       << G4endl;
    G4cout << "* Membrane thickness: " << membrane_thickn_ << G4endl;
    G4cout << "* Coating thickness: "  << coating_thickn_  << G4endl;


  /// VISIBILITIES
  if (visibility_) {
    board_logic  ->SetVisAttributes(Red());
    kapton_logic ->SetVisAttributes(Blue());
    if (mask_thickn_!=0){
      mask_logic   ->SetVisAttributes(LightBlue());
      if (membrane_thickn_ > 0.) membrane_logic->SetVisAttributes(Brown());
      if (coating_thickn_  > 0.) coating_logic ->SetVisAttributes(G4Color::Green());
      //hole_logic   ->SetVisAttributes(G4VisAttributes::Invisible);
    }
  }
  else{
    board_logic  ->SetVisAttributes(G4VisAttributes::Invisible);
    kapton_logic ->SetVisAttributes(G4VisAttributes::Invisible);
    if (mask_thickn_!=0){
      mask_logic   ->SetVisAttributes(G4VisAttributes::Invisible);
      if (membrane_thickn_ > 0.) membrane_logic->SetVisAttributes(G4VisAttributes::Invisible);
      if (coating_thickn_  > 0.) coating_logic ->SetVisAttributes(G4VisAttributes::Invisible);
      //hole_logic   ->SetVisAttributes(G4VisAttributes::Invisible);
    }
  }
  //if (mask_thickn_!=0){
    //if (membrane_thickn_ > 0.) membrane_logic->SetVisAttributes(G4VisAttributes::Invisible);
    //if (coating_thickn_  > 0.) coating_logic ->SetVisAttributes(G4VisAttributes::Invisible);
  //}
}




G4ThreeVector BlackBoxSiPMBoard::GenerateVertex(const G4String&) const
{
  // Only one generation region available at the moment
  return kapton_gen_->GenerateVertex("INSIDE");
}

void BlackBoxSiPMBoard::GenerateSiPMPositions()
{
  G4double margin    = sipm_pitch_/2. - side_reduction_;

  for (auto i=0; i<8; i++) {
    G4double sipm_posy = -board_size_.y()/2. + margin + i * sipm_pitch_;

    for (auto j=0; j<8; j++) {
      G4double sipm_posx = +board_size_.x()/2. - margin - j * sipm_pitch_;

      sipm_positions_.push_back(G4ThreeVector(sipm_posx, sipm_posy, 0.));
    }
  }
}
