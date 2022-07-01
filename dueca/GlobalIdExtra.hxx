/* ------------------------------------------------------------------   */
/*      item            : GlobalIdExtra.hxx
        made by         : Rene' van Paassen
        date            : 1301002
        category        : additional header code
        description     :
        changes         :
        language        : C++
        copyright       : (c) 2013 TUDelft-AE-C&S
        copyright       : (c) 2022 René van Paassen
        license         : EUPL-1.2
*/

  /** Return the object id. */
  inline ObjectId getObjectId() const { return object; }

  /** Return the location/node id. */
  inline LocationId getLocationId() const { return location; }

  /** flags invalid/max for object */
  static const uint16_t invalid_object_id = 0xffff;

  /** flags invalid/max for location */
  static const uint8_t invalid_location_id = 0xff;

  /** Invalid ID's also exist (for example for incomplete channels),
      this returns true if the ID is valid. */
  inline bool validId() const
  { return location != invalid_location_id && object != invalid_object_id; }

  /** Convert this to a comma-separated string of location and object */
  std::string printid() const;
