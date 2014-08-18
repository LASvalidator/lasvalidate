/*
===============================================================================

  FILE:  crscheck.hpp
  
  CONTENTS:
  
    Functions to validate whether the GEOTIFF tags specify a valid geo-coding
    and (someday) whether they are in agreement to the OGC WKT string.
  
  PROGRAMMERS:

    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
  
  COPYRIGHT:
  
    (c) 2007-2014, martin isenburg, rapidlasso - tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    3 July 2014 -- allowing unknown ESPG codes to pass CRS check with warning
    8 September 2013 -- it started raining on the beach after TREEMAPS meeting
  
===============================================================================
*/
#ifndef CRS_CHECK_HPP
#define CRS_CHECK_HPP

#include "lasheader.hpp"

class CRSprojectionEllipsoid
{
public:
  I32 id;
  const CHAR* name;
  F64 equatorial_radius;
  F64 polar_radius;
  F64 eccentricity_squared;
  F64 inverse_flattening;
  F64 eccentricity_prime_squared;
  F64 eccentricity;
  F64 eccentricity_e1;
};

class CRSprojectionParameters
{
public:
  I32 type;
  CHAR name[256];
  I16 geokey;
  CRSprojectionParameters() { type = -1; memset(name, 0, 256); geokey = 0; };
};

class CRSprojectionParametersUTM : public CRSprojectionParameters
{
public:
  I32 utm_zone_number;
  CHAR utm_zone_letter;
  BOOL utm_northern_hemisphere;
  I32 utm_long_origin;
};

class CRSprojectionParametersLCC : public CRSprojectionParameters
{
public:
  F64 lcc_false_easting_meter;
  F64 lcc_false_northing_meter;
  F64 lcc_lat_origin_degree;
  F64 lcc_long_meridian_degree;
  F64 lcc_first_std_parallel_degree;
  F64 lcc_second_std_parallel_degree;
  F64 lcc_lat_origin_radian;
  F64 lcc_long_meridian_radian;
  F64 lcc_first_std_parallel_radian;
  F64 lcc_second_std_parallel_radian;
  F64 lcc_n;
  F64 lcc_aF;
  F64 lcc_rho0;
};

class CRSprojectionParametersTM : public CRSprojectionParameters
{
public:
  F64 tm_false_easting_meter;
  F64 tm_false_northing_meter;
  F64 tm_lat_origin_degree;
  F64 tm_long_meridian_degree;
  F64 tm_scale_factor;
  F64 tm_lat_origin_radian;
  F64 tm_long_meridian_radian;
  F64 tm_ap;
  F64 tm_bp;
  F64 tm_cp;
  F64 tm_dp;
  F64 tm_ep;
};

class CRScheck
{
public:
  void check(LASheader* lasheader, CHAR* description);
  CRScheck();
  ~CRScheck();

private:
  U32 coordinate_units[2];
  U32 elevation_units[2];
  U32 vertical_epsg[2];
  CRSprojectionEllipsoid* ellipsoids[2];
  CRSprojectionParameters* projections[2];

  void set_coordinates_in_survey_feet(const BOOL from_geokeys);
  void set_coordinates_in_feet(const BOOL from_geokeys);
  void set_coordinates_in_meter(const BOOL from_geokeys);
  void set_elevation_in_survey_feet(const BOOL from_geokeys);
  void set_elevation_in_feet(const BOOL from_geokeys);
  void set_elevation_in_meter(const BOOL from_geokeys);
  BOOL set_ellipsoid(const I32 ellipsoid_id, const BOOL from_geokeys, char* description=0);
  void set_projection(CRSprojectionParameters* projection, const BOOL from_geokeys);
  BOOL set_latlong_projection(const BOOL from_geokeys, CHAR* description=0);
  BOOL set_longlat_projection(const BOOL from_geokeys, CHAR* description=0);
  BOOL set_ecef_projection(const BOOL from_geokeys, CHAR* description=0);
  BOOL set_utm_projection(const CHAR* zone, const BOOL from_geokeys, CHAR* description=0);
  BOOL set_utm_projection(const I32 zone, const BOOL northern, const const BOOL from_geokeys, CHAR* description=0);
  BOOL set_mga_projection(const I32 zone, const BOOL northern, const const BOOL from_geokeys, CHAR* description=0);
  void set_lambert_conformal_conic_projection(const F64 falseEasting, const F64 falseNorthing, const F64 latOriginDegree, const F64 longMeridianDegree, const F64 firstStdParallelDegree, const F64 secondStdParallelDegree, const BOOL from_geokeys, CHAR* description=0);
  void set_transverse_mercator_projection(const F64 falseEasting, const F64 falseNorthing, const F64 latOriginDegree, const F64 longMeridianDegree, const F64 scaleFactor, const BOOL from_geokeys, CHAR* description=0);
  BOOL set_state_plane_nad27_lcc(const CHAR* zone, const BOOL from_geokeys, CHAR* description=0);
  BOOL set_state_plane_nad83_lcc(const CHAR* zone, const BOOL from_geokeys, CHAR* description=0);
  BOOL set_state_plane_nad27_tm(const CHAR* zone, const BOOL from_geokeys, CHAR* description=0);
  BOOL set_state_plane_nad83_tm(const CHAR* zone, const BOOL from_geokeys, CHAR* description=0);
  BOOL set_coordinates_from_ProjLinearUnitsGeoKey(U16 value);
  BOOL set_elevation_from_VerticalUnitsGeoKey(U16 value);
  BOOL set_vertical_from_VerticalCSTypeGeoKey(U16 value);
  BOOL set_projection_from_ProjectedCSTypeGeoKey(const U16 value, CHAR* description=0);

  BOOL check_geokeys(LASheader* lasheader, CHAR* description);
};

#endif
