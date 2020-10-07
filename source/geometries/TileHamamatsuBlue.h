// ----------------------------------------------------------------------------
// nexus | TileHamamatsuBlue.cc
//
// Hamamatsu Blue sensor tile geometry.
//
// The NEXT Collaboration
// ----------------------------------------------------------------------------

#ifndef TILE_HAMAMATSU_BLUE_H
#define TILE_HAMAMATSU_BLUE_H

#include "BaseGeometry.h"
#include <G4ThreeVector.hh>

class G4GenericMessenger;
namespace nexus {class SiPMHamamatsuBlue;}

namespace nexus {

  class TileHamamatsuBlue: public BaseGeometry
  {
  public:
    /// Constructor
    TileHamamatsuBlue();
    /// Destructor
    ~TileHamamatsuBlue();

    /// Return dimensions of the SiPM
    //G4ThreeVector GetDimensions() const;

    /// Invoke this method to build the volumes of the geometry
    void Construct();

    G4ThreeVector GetDimensions();

  private:
    //G4ThreeVector _dimensions; ///< external dimensions of the SiPMpet

    // Visibility of the tracking plane
    G4bool visibility_;

    // Reflectivity of the tile
    G4double reflectivity_;

    // Tile dimensions
    G4double tile_x_, tile_y_, tile_z_;

    // SiPM pitch
    G4double sipm_pitch_;

    // Rows and columns of SiPMs
    G4int n_rows_, n_columns_;

     // Messenger for the definition of control commands
    G4GenericMessenger* msg_;

    SiPMHamamatsuBlue* sipm_;

  };


} // end namespace nexus

#endif