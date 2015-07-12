/*
===============================================================================

  FILE:  crscheck.cpp
  
  CONTENTS:
  
    see corresponding header file

  PROGRAMMERS:
  
    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
  
  COPYRIGHT:
  
    (c) 2013, martin isenburg, rapidlasso - fast tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    see corresponding header file
  
===============================================================================
*/

#include <time.h>
#include <string.h>
#include <math.h>

#include "crscheck.hpp"

static const F64 PI = 3.141592653589793238462643383279502884197169;
static const F64 TWO_PI = PI * 2;
static const F64 PI_OVER_2 = PI / 2;
static const F64 PI_OVER_4 = PI / 4;
static const F64 deg2rad = PI / 180.0;
static const F64 rad2deg = 180.0 / PI;

static const F64 feet2meter = 0.3048;
static const F64 surveyfeet2meter = 0.3048006096012;

static const int CRS_PROJECTION_UTM      = 0;
static const int CRS_PROJECTION_LCC      = 1;
static const int CRS_PROJECTION_TM       = 2;
static const int CRS_PROJECTION_LONG_LAT = 3;
static const int CRS_PROJECTION_LAT_LONG = 4;
static const int CRS_PROJECTION_ECEF     = 5;
static const int CRS_PROJECTION_NONE     = 6;

#define CRS_ELLIPSOID_NAD27 5
#define CRS_ELLIPSOID_NAD83 11
#define CRS_ELLIPSOID_Inter 14
#define CRS_ELLIPSOID_SAD69 19
#define CRS_ELLIPSOID_WGS72 22
#define CRS_ELLIPSOID_WGS84 23
#define CRS_ELLIPSOID_ID74  24
#define CRS_ELLIPSOID_GDA94 CRS_ELLIPSOID_NAD83

#define CRS_VERTICAL_WGS84   5030
#define CRS_VERTICAL_NAVD29  5102
#define CRS_VERTICAL_NAVD88  5103

class ReferenceEllipsoid
{
public:
  ReferenceEllipsoid(const I32 id, const CHAR* name, const F64 equatorialRadius, const F64 eccentricitySquared, const F64 inverseFlattening)
  {
    this->id = id;
    this->name = name; 
    this->equatorialRadius = equatorialRadius;
    this->eccentricitySquared = eccentricitySquared;
    this->inverseFlattening = inverseFlattening;
  }
  I32 id;
  const CHAR* name;
  F64 equatorialRadius; 
  F64 eccentricitySquared;  
  F64 inverseFlattening;  
};

static const ReferenceEllipsoid ellipsoid_list[] = 
{
  //  d, Ellipsoid name, Equatorial Radius, square of eccentricity, inverse flattening  
  ReferenceEllipsoid( -1, "Placeholder", 0, 0, 0),  //placeholder to allow array indices to match id numbers
  ReferenceEllipsoid( 1, "Airy", 6377563.396, 0.00667054, 299.3249646),
  ReferenceEllipsoid( 2, "Australian National", 6378160.0, 0.006694542, 298.25),
  ReferenceEllipsoid( 3, "Bessel 1841", 6377397.155, 0.006674372, 299.1528128),
  ReferenceEllipsoid( 4, "Bessel 1841 (Nambia) ", 6377483.865, 0.006674372, 299.1528128),
  ReferenceEllipsoid( 5, "Clarke 1866 (NAD-27)", 6378206.4, 0.006768658, 294.9786982),
  ReferenceEllipsoid( 6, "Clarke 1880", 6378249.145, 0.006803511, 293.465),
  ReferenceEllipsoid( 7, "Everest 1830", 6377276.345, 0.006637847, 300.8017),
  ReferenceEllipsoid( 8, "Fischer 1960 (Mercury) ", 6378166, 0.006693422, 298.3),
  ReferenceEllipsoid( 9, "Fischer 1968", 6378150, 0.006693422, 298.3),
  ReferenceEllipsoid( 10, "GRS 1967", 6378160, 0.006694605, 298.247167427),
  ReferenceEllipsoid( 11, "GRS 1980 (NAD-83,GDA-94)", 6378137, 0.00669438002290, 298.257222101),
  ReferenceEllipsoid( 12, "Helmert 1906", 6378200, 0.006693422, 298.3),
  ReferenceEllipsoid( 13, "Hough", 6378270, 0.00672267, 297.0),
  ReferenceEllipsoid( 14, "International", 6378388, 0.00672267, 297.0),
  ReferenceEllipsoid( 15, "Krassovsky", 6378245, 0.006693422, 298.3),
  ReferenceEllipsoid( 16, "Modified Airy", 6377340.189, 0.00667054, 299.3249646),
  ReferenceEllipsoid( 17, "Modified Everest", 6377304.063, 0.006637847, 300.8017),
  ReferenceEllipsoid( 18, "Modified Fischer 1960", 6378155, 0.006693422, 298.3),
  ReferenceEllipsoid( 19, "South American 1969", 6378160, 0.006694542, 298.25),
  ReferenceEllipsoid( 20, "WGS 60", 6378165, 0.006693422, 298.3),
  ReferenceEllipsoid( 21, "WGS 66", 6378145, 0.006694542, 298.25),
  ReferenceEllipsoid( 22, "WGS-72", 6378135, 0.006694318, 298.26),
  ReferenceEllipsoid( 23, "WGS-84", 6378137, 0.00669437999013, 298.257223563),
  ReferenceEllipsoid( 24, "Indonesian National 1974", 6378160, 0.0066946091071419115, 298.2469988070381)
  ,
};

static const int PCS_NAD27_Alabama_East = 26729;
static const int PCS_NAD27_Alabama_West = 26730;
static const int PCS_NAD27_Alaska_zone_1 = 26731; /* Hotine Oblique Mercator Projection not supported*/
static const int PCS_NAD27_Alaska_zone_2 = 26732;
static const int PCS_NAD27_Alaska_zone_3 = 26733;
static const int PCS_NAD27_Alaska_zone_4 = 26734;
static const int PCS_NAD27_Alaska_zone_5 = 26735;
static const int PCS_NAD27_Alaska_zone_6 = 26736;
static const int PCS_NAD27_Alaska_zone_7 = 26737;
static const int PCS_NAD27_Alaska_zone_8 = 26738;
static const int PCS_NAD27_Alaska_zone_9 = 26739;
static const int PCS_NAD27_Alaska_zone_10 = 26740;
static const int PCS_NAD27_California_I = 26741;
static const int PCS_NAD27_California_II = 26742;
static const int PCS_NAD27_California_III = 26743;
static const int PCS_NAD27_California_IV = 26744;
static const int PCS_NAD27_California_V = 26745;
static const int PCS_NAD27_California_VI = 26746;
static const int PCS_NAD27_California_VII = 26747;
static const int PCS_NAD27_Arizona_East = 26748;
static const int PCS_NAD27_Arizona_Central = 26749;
static const int PCS_NAD27_Arizona_West = 26750;
static const int PCS_NAD27_Arkansas_North = 26751;
static const int PCS_NAD27_Arkansas_South = 26752;
static const int PCS_NAD27_Colorado_North = 26753;
static const int PCS_NAD27_Colorado_Central = 26754;
static const int PCS_NAD27_Colorado_South = 26755;
static const int PCS_NAD27_Connecticut = 26756;
static const int PCS_NAD27_Delaware = 26757;
static const int PCS_NAD27_Florida_East = 26758;
static const int PCS_NAD27_Florida_West = 26759;
static const int PCS_NAD27_Florida_North = 26760;
static const int PCS_NAD27_Hawaii_zone_1 = 26761;
static const int PCS_NAD27_Hawaii_zone_2 = 26762;
static const int PCS_NAD27_Hawaii_zone_3 = 26763;
static const int PCS_NAD27_Hawaii_zone_4 = 26764;
static const int PCS_NAD27_Hawaii_zone_5 = 26765;
static const int PCS_NAD27_Georgia_East = 26766;
static const int PCS_NAD27_Georgia_West = 26767;
static const int PCS_NAD27_Idaho_East = 26768;
static const int PCS_NAD27_Idaho_Central = 26769;
static const int PCS_NAD27_Idaho_West = 26770;
static const int PCS_NAD27_Illinois_East = 26771;
static const int PCS_NAD27_Illinois_West = 26772;
static const int PCS_NAD27_Indiana_East = 26773;
static const int PCS_NAD27_Indiana_West = 26774;
static const int PCS_NAD27_Iowa_North = 26775;
static const int PCS_NAD27_Iowa_South = 26776;
static const int PCS_NAD27_Kansas_North = 26777;
static const int PCS_NAD27_Kansas_South = 26778;
static const int PCS_NAD27_Kentucky_North = 26779;
static const int PCS_NAD27_Kentucky_South = 26780;
static const int PCS_NAD27_Louisiana_North = 26781;
static const int PCS_NAD27_Louisiana_South = 26782;
static const int PCS_NAD27_Maine_East = 26783;
static const int PCS_NAD27_Maine_West = 26784;
static const int PCS_NAD27_Maryland = 26785;
static const int PCS_NAD27_Massachusetts = 26786;
static const int PCS_NAD27_Massachusetts_Is = 26787;
static const int PCS_NAD27_Michigan_North = 26788;
static const int PCS_NAD27_Michigan_Central = 26789;
static const int PCS_NAD27_Michigan_South = 26790;
static const int PCS_NAD27_Minnesota_North = 26791;
static const int PCS_NAD27_Minnesota_Central = 26792;
static const int PCS_NAD27_Minnesota_South = 26793;
static const int PCS_NAD27_Mississippi_East = 26794;
static const int PCS_NAD27_Mississippi_West = 26795;
static const int PCS_NAD27_Missouri_East = 26796;
static const int PCS_NAD27_Missouri_Central = 26797;
static const int PCS_NAD27_Missouri_West = 26798;
static const int PCS_NAD27_Montana_North = 32001;
static const int PCS_NAD27_Montana_Central = 32002;
static const int PCS_NAD27_Montana_South = 32003;
static const int PCS_NAD27_Nebraska_North = 32005;
static const int PCS_NAD27_Nebraska_South = 32006;
static const int PCS_NAD27_Nevada_East = 32007;
static const int PCS_NAD27_Nevada_Central = 32008;
static const int PCS_NAD27_Nevada_West = 32009;
static const int PCS_NAD27_New_Hampshire = 32010;
static const int PCS_NAD27_New_Jersey = 32011;
static const int PCS_NAD27_New_Mexico_East = 32012;
static const int PCS_NAD27_New_Mexico_Central = 32013;
static const int PCS_NAD27_New_Mexico_West = 32014;
static const int PCS_NAD27_New_York_East = 32015;
static const int PCS_NAD27_New_York_Central = 32016;
static const int PCS_NAD27_New_York_West = 32017;
static const int PCS_NAD27_New_York_Long_Is = 32018;
static const int PCS_NAD27_North_Carolina = 32019;
static const int PCS_NAD27_North_Dakota_N = 32020;
static const int PCS_NAD27_North_Dakota_S = 32021;
static const int PCS_NAD27_Ohio_North = 32022;
static const int PCS_NAD27_Ohio_South = 32023;
static const int PCS_NAD27_Oklahoma_North = 32024;
static const int PCS_NAD27_Oklahoma_South = 32025;
static const int PCS_NAD27_Oregon_North = 32026;
static const int PCS_NAD27_Oregon_South = 32027;
static const int PCS_NAD27_Pennsylvania_N = 32028;
static const int PCS_NAD27_Pennsylvania_S = 32029;
static const int PCS_NAD27_Rhode_Island = 32030;
static const int PCS_NAD27_South_Carolina_N = 32031;
static const int PCS_NAD27_South_Carolina_S = 32033;
static const int PCS_NAD27_South_Dakota_N = 32034;
static const int PCS_NAD27_South_Dakota_S = 32035;
static const int PCS_NAD27_Tennessee = 2204;
static const int PCS_NAD27_Texas_North = 32037;
static const int PCS_NAD27_Texas_North_Central = 32038;
static const int PCS_NAD27_Texas_Central = 32039;
static const int PCS_NAD27_Texas_South_Central = 32040;
static const int PCS_NAD27_Texas_South = 32041;
static const int PCS_NAD27_Utah_North = 32042;
static const int PCS_NAD27_Utah_Central = 32043;
static const int PCS_NAD27_Utah_South = 32044;
static const int PCS_NAD27_Vermont = 32045;
static const int PCS_NAD27_Virginia_North = 32046;
static const int PCS_NAD27_Virginia_South = 32047;
static const int PCS_NAD27_Washington_North = 32048;
static const int PCS_NAD27_Washington_South = 32049;
static const int PCS_NAD27_West_Virginia_N = 32050;
static const int PCS_NAD27_West_Virginia_S = 32051;
static const int PCS_NAD27_Wisconsin_North = 32052;
static const int PCS_NAD27_Wisconsin_Central = 32053;
static const int PCS_NAD27_Wisconsin_South = 32054;
static const int PCS_NAD27_Wyoming_East = 32055;
static const int PCS_NAD27_Wyoming_East_Central = 32056;
static const int PCS_NAD27_Wyoming_West_Central = 32057;
static const int PCS_NAD27_Wyoming_West = 32058;
static const int PCS_NAD27_Puerto_Rico = 32059;
static const int PCS_NAD27_St_Croix = 32060;

static const int PCS_NAD83_Alabama_East = 26929;
static const int PCS_NAD83_Alabama_West = 26930;
static const int PCS_NAD83_Alaska_zone_1 = 26931; /* Hotine Oblique Mercator Projection not supported*/
static const int PCS_NAD83_Alaska_zone_2 = 26932;
static const int PCS_NAD83_Alaska_zone_3 = 26933;
static const int PCS_NAD83_Alaska_zone_4 = 26934;
static const int PCS_NAD83_Alaska_zone_5 = 26935;
static const int PCS_NAD83_Alaska_zone_6 = 26936;
static const int PCS_NAD83_Alaska_zone_7 = 26937;
static const int PCS_NAD83_Alaska_zone_8 = 26938;
static const int PCS_NAD83_Alaska_zone_9 = 26939;
static const int PCS_NAD83_Alaska_zone_10 = 26940;
static const int PCS_NAD83_California_1 = 26941;
static const int PCS_NAD83_California_2 = 26942;
static const int PCS_NAD83_California_3 = 26943;
static const int PCS_NAD83_California_4 = 26944;
static const int PCS_NAD83_California_5 = 26945;
static const int PCS_NAD83_California_6 = 26946;
static const int PCS_NAD83_Arizona_East = 26948;
static const int PCS_NAD83_Arizona_Central = 26949;
static const int PCS_NAD83_Arizona_West = 26950;
static const int PCS_NAD83_Arkansas_North = 26951;
static const int PCS_NAD83_Arkansas_South = 26952;
static const int PCS_NAD83_Colorado_North = 26953;
static const int PCS_NAD83_Colorado_Central = 26954;
static const int PCS_NAD83_Colorado_South = 26955;
static const int PCS_NAD83_Connecticut = 26956;
static const int PCS_NAD83_Delaware = 26957;
static const int PCS_NAD83_Florida_East = 26958;
static const int PCS_NAD83_Florida_West = 26959;
static const int PCS_NAD83_Florida_North = 26960;
static const int PCS_NAD83_Hawaii_zone_1 = 26961;
static const int PCS_NAD83_Hawaii_zone_2 = 26962;
static const int PCS_NAD83_Hawaii_zone_3 = 26963;
static const int PCS_NAD83_Hawaii_zone_4 = 26964;
static const int PCS_NAD83_Hawaii_zone_5 = 26965;
static const int PCS_NAD83_Georgia_East = 26966;
static const int PCS_NAD83_Georgia_West = 26967;
static const int PCS_NAD83_Idaho_East = 26968;
static const int PCS_NAD83_Idaho_Central = 26969;
static const int PCS_NAD83_Idaho_West = 26970;
static const int PCS_NAD83_Illinois_East = 26971;
static const int PCS_NAD83_Illinois_West = 26972;
static const int PCS_NAD83_Indiana_East = 26973;
static const int PCS_NAD83_Indiana_West = 26974;
static const int PCS_NAD83_Iowa_North = 26975;
static const int PCS_NAD83_Iowa_South = 26976;
static const int PCS_NAD83_Kansas_North = 26977;
static const int PCS_NAD83_Kansas_South = 26978;
static const int PCS_NAD83_Kentucky_North = 2205;
static const int PCS_NAD83_Kentucky_South = 26980;
static const int PCS_NAD83_Louisiana_North = 26981;
static const int PCS_NAD83_Louisiana_South = 26982;
static const int PCS_NAD83_Maine_East = 26983;
static const int PCS_NAD83_Maine_West = 26984;
static const int PCS_NAD83_Maryland = 26985;
static const int PCS_NAD83_Massachusetts = 26986;
static const int PCS_NAD83_Massachusetts_Is = 26987;
static const int PCS_NAD83_Michigan_North = 26988;
static const int PCS_NAD83_Michigan_Central = 26989;
static const int PCS_NAD83_Michigan_South = 26990;
static const int PCS_NAD83_Minnesota_North = 26991;
static const int PCS_NAD83_Minnesota_Central = 26992;
static const int PCS_NAD83_Minnesota_South = 26993;
static const int PCS_NAD83_Mississippi_East = 26994;
static const int PCS_NAD83_Mississippi_West = 26995;
static const int PCS_NAD83_Missouri_East = 26996;
static const int PCS_NAD83_Missouri_Central = 26997;
static const int PCS_NAD83_Missouri_West = 26998;
static const int PCS_NAD83_Montana = 32100;
static const int PCS_NAD83_Nebraska = 32104;
static const int PCS_NAD83_Nevada_East = 32107;
static const int PCS_NAD83_Nevada_Central = 32108;
static const int PCS_NAD83_Nevada_West = 32109;
static const int PCS_NAD83_New_Hampshire = 32110;
static const int PCS_NAD83_New_Jersey = 32111;
static const int PCS_NAD83_New_Mexico_East = 32112;
static const int PCS_NAD83_New_Mexico_Central = 32113;
static const int PCS_NAD83_New_Mexico_West = 32114;
static const int PCS_NAD83_New_York_East = 32115;
static const int PCS_NAD83_New_York_Central = 32116;
static const int PCS_NAD83_New_York_West = 32117;
static const int PCS_NAD83_New_York_Long_Is = 32118;
static const int PCS_NAD83_North_Carolina = 32119;
static const int PCS_NAD83_North_Dakota_N = 32120;
static const int PCS_NAD83_North_Dakota_S = 32121;
static const int PCS_NAD83_Ohio_North = 32122;
static const int PCS_NAD83_Ohio_South = 32123;
static const int PCS_NAD83_Oklahoma_North = 32124;
static const int PCS_NAD83_Oklahoma_South = 32125;
static const int PCS_NAD83_Oregon_North = 32126;
static const int PCS_NAD83_Oregon_South = 32127;
static const int PCS_NAD83_Pennsylvania_N = 32128;
static const int PCS_NAD83_Pennsylvania_S = 32129;
static const int PCS_NAD83_Rhode_Island = 32130;
static const int PCS_NAD83_South_Carolina = 32133;
static const int PCS_NAD83_South_Dakota_N = 32134;
static const int PCS_NAD83_South_Dakota_S = 32135;
static const int PCS_NAD83_Tennessee = 32136;
static const int PCS_NAD83_Texas_North = 32137;
static const int PCS_NAD83_Texas_North_Central = 32138;
static const int PCS_NAD83_Texas_Central = 32139;
static const int PCS_NAD83_Texas_South_Central = 32140;
static const int PCS_NAD83_Texas_South = 32141;
static const int PCS_NAD83_Utah_North = 32142;
static const int PCS_NAD83_Utah_Central = 32143;
static const int PCS_NAD83_Utah_South = 32144;
static const int PCS_NAD83_Vermont = 32145;
static const int PCS_NAD83_Virginia_North = 32146;
static const int PCS_NAD83_Virginia_South = 32147;
static const int PCS_NAD83_Washington_North = 32148;
static const int PCS_NAD83_Washington_South = 32149;
static const int PCS_NAD83_West_Virginia_N = 32150;
static const int PCS_NAD83_West_Virginia_S = 32151;
static const int PCS_NAD83_Wisconsin_North = 32152;
static const int PCS_NAD83_Wisconsin_Central = 32153;
static const int PCS_NAD83_Wisconsin_South = 32154;
static const int PCS_NAD83_Wyoming_East = 32155;
static const int PCS_NAD83_Wyoming_East_Central = 32156;
static const int PCS_NAD83_Wyoming_West_Central = 32157;
static const int PCS_NAD83_Wyoming_West = 32158;
static const int PCS_NAD83_Puerto_Rico = 32161;

class StatePlaneLCC
{
public:
  StatePlaneLCC(short geokey, char* zone, F64 falseEasting, F64 falseNorthing, F64 latOriginDegree, F64 longMeridianDegree, F64 firstStdParallelDegree, F64 secondStdParallelDegree)
  {
    this->geokey = geokey;
    this->zone = zone;
    this->falseEasting = falseEasting;
    this->falseNorthing = falseNorthing;
    this->latOriginDegree = latOriginDegree;
    this->longMeridianDegree = longMeridianDegree;
    this->firstStdParallelDegree = firstStdParallelDegree;
    this->secondStdParallelDegree = secondStdParallelDegree;
  }
  short geokey;
  char* zone;
  F64 falseEasting;
  F64 falseNorthing;
  F64 latOriginDegree;
  F64 longMeridianDegree;
  F64 firstStdParallelDegree;
  F64 secondStdParallelDegree;
};

static const StatePlaneLCC state_plane_lcc_nad27_list[] =
{
  // zone, false east [m], false north [m], ProjOrig(Lat), CentMerid(Long), 1st std para, 2nd std para 
  StatePlaneLCC(PCS_NAD27_Alaska_zone_10, "AK_10",914401.8288,0,51,-176,51.83333333,53.83333333),
  StatePlaneLCC(PCS_NAD27_Arkansas_North, "AR_N",609601.2192,0,34.33333333,-92,34.93333333,36.23333333),
  StatePlaneLCC(PCS_NAD27_Arkansas_South, "AR_S",609601.2192,0,32.66666667,-92,33.3,34.76666667),
  StatePlaneLCC(PCS_NAD27_California_I, "CA_I",609601.2192,0,39.33333333,-122,40,41.66666667),
  StatePlaneLCC(PCS_NAD27_California_II, "CA_II",609601.2192,0,37.66666667,-122,38.33333333,39.83333333),
  StatePlaneLCC(PCS_NAD27_California_III, "CA_III",609601.2192,0,36.5,-120.5,37.06666667,38.43333333),
  StatePlaneLCC(PCS_NAD27_California_IV, "CA_IV",609601.2192,0,35.33333333,-119,36,37.25),
  StatePlaneLCC(PCS_NAD27_California_V, "CA_V",609601.2192,0,33.5,-118,34.03333333,35.46666667),
  StatePlaneLCC(PCS_NAD27_California_VI, "CA_VI",609601.2192,0,32.16666667,-116.25,32.78333333,33.88333333),
  StatePlaneLCC(PCS_NAD27_California_VII, "CA_VII",1276106.451,1268253.007,34.13333333,-118.3333333,33.86666667,34.41666667),
  StatePlaneLCC(PCS_NAD27_Colorado_North, "CO_N",609601.2192,0,39.33333333,-105.5,39.71666667,40.78333333),
  StatePlaneLCC(PCS_NAD27_Colorado_Central, "CO_C",609601.2192,0,37.83333333,-105.5,38.45,39.75),
  StatePlaneLCC(PCS_NAD27_Colorado_South, "CO_S",609601.2192,0,36.66666667,-105.5,37.23333333,38.43333333),
  StatePlaneLCC(PCS_NAD27_Connecticut, "CT",182880.3658,0,40.83333333,-72.75,41.2,41.86666667),
  StatePlaneLCC(PCS_NAD27_Florida_North, "FL_N",609601.2192,0,29,-84.5,29.58333333,30.75),
  StatePlaneLCC(PCS_NAD27_Iowa_North, "IA_N",609601.2192,0,41.5,-93.5,42.06666667,43.26666667),
  StatePlaneLCC(PCS_NAD27_Iowa_South, "IA_S",609601.2192,0,40,-93.5,40.61666667,41.78333333),
  StatePlaneLCC(PCS_NAD27_Kansas_North, "KS_N",609601.2192,0,38.33333333,-98,38.71666667,39.78333333),
  StatePlaneLCC(PCS_NAD27_Kansas_South, "KS_S",609601.2192,0,36.66666667,-98.5,37.26666667,38.56666667),
  StatePlaneLCC(PCS_NAD27_Kentucky_North, "KY_N",609601.2192,0,37.5,-84.25,37.96666667,38.96666667),
  StatePlaneLCC(PCS_NAD27_Kentucky_South, "KY_S",609601.2192,0,36.33333333,-85.75,36.73333333,37.93333333),
  StatePlaneLCC(PCS_NAD27_Louisiana_North, "LA_N",609601.2192,0,30.66666667,-92.5,31.16666667,32.66666667),
  StatePlaneLCC(PCS_NAD27_Louisiana_South, "LA_S",609601.2192,0,28.66666667,-91.33333333,29.3,30.7),
  StatePlaneLCC(PCS_NAD27_Maryland, "MD",243840.4877,0,37.83333333,-77,38.3,39.45),
  StatePlaneLCC(PCS_NAD27_Massachusetts, "MA_M",182880.3658,0,41,-71.5,41.71666667,42.68333333),
  StatePlaneLCC(PCS_NAD27_Massachusetts_Is, "MA_I",60960.12192,0,41,-70.5,41.28333333,41.48333333),
  StatePlaneLCC(PCS_NAD27_Michigan_North, "MI_N",609601.2192,0,44.78333333,-87,45.48333333,47.08333333),
  StatePlaneLCC(PCS_NAD27_Michigan_Central, "MI_C",609601.2192,0,43.31666667,-84.33333333,44.18333333,45.7),
  StatePlaneLCC(PCS_NAD27_Michigan_South, "MI_S",609601.2192,0,41.5,-84.33333333,42.1,43.66666667),
  StatePlaneLCC(PCS_NAD27_Minnesota_North, "MN_N",609601.2192,0,46.5,-93.1,47.03333333,48.63333333),
  StatePlaneLCC(PCS_NAD27_Minnesota_Central, "MN_C",609601.2192,0,45,-94.25,45.61666667,47.05),
  StatePlaneLCC(PCS_NAD27_Minnesota_South, "MN_S",609601.2192,0,43,-94,43.78333333,45.21666667),
  StatePlaneLCC(PCS_NAD27_Montana_North, "MT_N",609601.2192,0,47,-109.5,47.85,48.71666667),
  StatePlaneLCC(PCS_NAD27_Montana_Central, "MT_C",609601.2192,0,45.83333333,-109.5,46.45,47.88333333),
  StatePlaneLCC(PCS_NAD27_Montana_South, "MT_S",609601.2192,0,44,-109.5,44.86666667,46.4),
  StatePlaneLCC(PCS_NAD27_Nebraska_North, "NE_N",609601.2192,0,41.33333333,-100,41.85,42.81666667),
  StatePlaneLCC(PCS_NAD27_Nebraska_South, "NE_S",609601.2192,0,39.66666667,-99.5,40.28333333,41.71666667),
  StatePlaneLCC(PCS_NAD27_New_York_Long_Is, "NY_LI",609601.2192,30480.06096,40.5,-74,40.66666667,41.03333333),
  StatePlaneLCC(PCS_NAD27_North_Carolina, "NC",609601.2192,0,33.75,-79,34.33333333,36.16666667),
  StatePlaneLCC(PCS_NAD27_North_Dakota_N, "ND_N",609601.2192,0,47,-100.5,47.43333333,48.73333333),
  StatePlaneLCC(PCS_NAD27_North_Dakota_S, "ND_S",609601.2192,0,45.66666667,-100.5,46.18333333,47.48333333),
  StatePlaneLCC(PCS_NAD27_Ohio_North, "OH_N",609601.2192,0,39.66666667,-82.5,40.43333333,41.7),
  StatePlaneLCC(PCS_NAD27_Ohio_South, "OH_S",609601.2192,0,38,-82.5,38.73333333,40.03333333),
  StatePlaneLCC(PCS_NAD27_Oklahoma_North, "OK_N",609601.2192,0,35,-98,35.56666667,36.76666667),
  StatePlaneLCC(PCS_NAD27_Oklahoma_South, "OK_S",609601.2192,0,33.33333333,-98,33.93333333,35.23333333),
  StatePlaneLCC(PCS_NAD27_Oregon_North, "OR_N",609601.2192,0,43.66666667,-120.5,44.33333333,46),
  StatePlaneLCC(PCS_NAD27_Oregon_South, "OR_S",609601.2192,0,41.66666667,-120.5,42.33333333,44),
  StatePlaneLCC(PCS_NAD27_Pennsylvania_N, "PA_N",609601.2192,0,40.16666667,-77.75,40.88333333,41.95),
  StatePlaneLCC(PCS_NAD27_Pennsylvania_S, "PA_S",609601.2192,0,39.33333333,-77.75,39.93333333,40.96666667),
  StatePlaneLCC(PCS_NAD27_Puerto_Rico, "PR",152400.3048,0,17.83333333,-66.43333333,18.03333333,18.43333333),
  StatePlaneLCC(PCS_NAD27_St_Croix, "St.Croix",152400.3048,30480.06096,17.83333333,-66.43333333,18.03333333,18.43333333),
  StatePlaneLCC(PCS_NAD27_South_Carolina_N, "SC_N",609601.2192,0,33,-81,33.76666667,34.96666667),
  StatePlaneLCC(PCS_NAD27_South_Carolina_S, "SC_S",609601.2192,0,31.83333333,-81,32.33333333,33.66666667),
  StatePlaneLCC(PCS_NAD27_South_Dakota_N, "SD_N",609601.2192,0,43.83333333,-100,44.41666667,45.68333333),
  StatePlaneLCC(PCS_NAD27_South_Dakota_S, "SD_S",609601.2192,0,42.33333333,-100.3333333,42.83333333,44.4),
  StatePlaneLCC(PCS_NAD27_Tennessee, "TN",609601.2192,30480.06096,34.66666667,-86,35.25,36.41666667),
  StatePlaneLCC(PCS_NAD27_Texas_North, "TX_N",609601.2192,0,34,-101.5,34.65,36.18333333),
  StatePlaneLCC(PCS_NAD27_Texas_North_Central, "TX_NC",609601.2192,0,31.66666667,-97.5,32.13333333,33.96666667),
  StatePlaneLCC(PCS_NAD27_Texas_Central, "TX_C",609601.2192,0,29.66666667,-100.3333333,30.11666667,31.88333333),
  StatePlaneLCC(PCS_NAD27_Texas_South_Central, "TX_SC",609601.2192,0,27.83333333,-99,28.38333333,30.28333333),
  StatePlaneLCC(PCS_NAD27_Texas_South, "TX_S",609601.2192,0,25.66666667,-98.5,26.16666667,27.83333333),
  StatePlaneLCC(PCS_NAD27_Utah_North, "UT_N",609601.2192,0,40.33333333,-111.5,40.71666667,41.78333333),
  StatePlaneLCC(PCS_NAD27_Utah_Central, "UT_C",609601.2192,0,38.33333333,-111.5,39.01666667,40.65),
  StatePlaneLCC(PCS_NAD27_Utah_South, "UT_S",609601.2192,0,36.66666667,-111.5,37.21666667,38.35),
  StatePlaneLCC(PCS_NAD27_Virginia_North, "VA_N",609601.2192,0,37.66666667,-78.5,38.03333333,39.2),
  StatePlaneLCC(PCS_NAD27_Virginia_South, "VA_S",609601.2192,0,36.33333333,-78.5,36.76666667,37.96666667),
  StatePlaneLCC(PCS_NAD27_Washington_North, "WA_N",609601.2192,0,47,-120.8333333,47.5,48.73333333),
  StatePlaneLCC(PCS_NAD27_Washington_South, "WA_S",609601.2192,0,45.33333333,-120.5,45.83333333,47.33333333),
  StatePlaneLCC(PCS_NAD27_West_Virginia_N, "WV_N",609601.2192,0,38.5,-79.5,39,40.25),
  StatePlaneLCC(PCS_NAD27_West_Virginia_S, "WV_S",609601.2192,0,37,-81,37.48333333,38.88333333),
  StatePlaneLCC(PCS_NAD27_Wisconsin_North, "WI_N",609601.2192,0,45.16666667,-90,45.56666667,46.76666667),
  StatePlaneLCC(PCS_NAD27_Wisconsin_Central, "WI_C",609601.2192,0,43.83333333,-90,44.25,45.5),
  StatePlaneLCC(PCS_NAD27_Wisconsin_South, "WI_S",609601.2192,0,42,-90,42.73333333,44.06666667),
  StatePlaneLCC(0,0,-1,-1,-1,-1,-1,-1)
};

static const StatePlaneLCC state_plane_lcc_nad83_list[] =
{
  // geotiff key, zone, false east [m], false north [m], ProjOrig(Lat), CentMerid(Long), 1st std para, 2nd std para 
  StatePlaneLCC(PCS_NAD83_Alaska_zone_10, "AK_10",1000000,0,51.000000,-176.000000,51.833333,53.833333),
  StatePlaneLCC(PCS_NAD83_Arkansas_North, "AR_N",400000,0,34.333333,-92.000000,34.933333,36.233333),
  StatePlaneLCC(PCS_NAD83_Arkansas_South, "AR_S",400000,400000,32.666667,-92.000000,33.300000,34.766667),
  StatePlaneLCC(PCS_NAD83_California_1, "CA_I",2000000,500000,39.333333,-122.000000,40.000000,41.666667),
  StatePlaneLCC(PCS_NAD83_California_2, "CA_II",2000000,500000,37.666667,-122.000000,38.333333,39.833333),
  StatePlaneLCC(PCS_NAD83_California_3, "CA_III",2000000,500000,36.500000,-120.500000,37.066667,38.433333),
  StatePlaneLCC(PCS_NAD83_California_4, "CA_IV",2000000,500000,35.333333,-119.000000,36.000000,37.250000),
  StatePlaneLCC(PCS_NAD83_California_5, "CA_V",2000000,500000,33.500000,-118.000000,34.033333,35.466667),
  StatePlaneLCC(PCS_NAD83_California_6, "CA_VI",2000000,500000,32.166667,-116.250000,32.783333,33.883333),
  StatePlaneLCC(PCS_NAD83_Colorado_North, "CO_N",914401.8289,304800.6096,39.333333,-105.500000,39.716667,40.783333),
  StatePlaneLCC(PCS_NAD83_Colorado_Central, "CO_C",914401.8289,304800.6096,37.833333,-105.500000,38.450000,39.750000),
  StatePlaneLCC(PCS_NAD83_Colorado_South, "CO_S",914401.8289,304800.6096,36.666667,-105.500000,37.233333,38.433333),
  StatePlaneLCC(PCS_NAD83_Connecticut, "CT",304800.6096,152400.3048,40.833333,-72.750000,41.200000,41.866667),
  StatePlaneLCC(PCS_NAD83_Florida_North, "FL_N",600000,0,29.000000,-84.500000,29.583333,30.750000),
  StatePlaneLCC(PCS_NAD83_Iowa_North, "IA_N",1500000,1000000,41.500000,-93.500000,42.066667,43.266667),
  StatePlaneLCC(PCS_NAD83_Iowa_South, "IA_S",500000,0,40.000000,-93.500000,40.616667,41.783333),
  StatePlaneLCC(PCS_NAD83_Kansas_North, "KS_N",400000,0,38.333333,-98.000000,38.716667,39.783333),
  StatePlaneLCC(PCS_NAD83_Kansas_South, "KS_S",400000,400000,36.666667,-98.500000,37.266667,38.566667),
  StatePlaneLCC(PCS_NAD83_Kentucky_North, "KY_N",500000,0,37.500000,-84.250000,37.966667,38.966667),
  StatePlaneLCC(PCS_NAD83_Kentucky_South, "KY_S",500000,500000,36.333333,-85.750000,36.733333,37.933333),
  StatePlaneLCC(PCS_NAD83_Louisiana_North, "LA_N",1000000,0,30.500000,-92.500000,31.166667,32.666667),
  StatePlaneLCC(PCS_NAD83_Louisiana_South, "LA_S",1000000,0,28.500000,-91.333333,29.300000,30.700000),
  StatePlaneLCC(PCS_NAD83_Maryland, "MD",400000,0,37.666667,-77.000000,38.300000,39.450000),
  StatePlaneLCC(PCS_NAD83_Massachusetts, "MA_M",200000,750000,41.000000,-71.500000,41.716667,42.683333),
  StatePlaneLCC(PCS_NAD83_Massachusetts_Is, "MA_I",500000,0,41.000000,-70.500000,41.283333,41.483333),
  StatePlaneLCC(PCS_NAD83_Michigan_North, "MI_N",8000000,0,44.783333,-87.000000,45.483333,47.083333),
  StatePlaneLCC(PCS_NAD83_Michigan_Central, "MI_C",6000000,0,43.316667,-84.366667,44.183333,45.700000),
  StatePlaneLCC(PCS_NAD83_Michigan_South, "MI_S",4000000,0,41.500000,-84.366667,42.100000,43.666667),
  StatePlaneLCC(PCS_NAD83_Minnesota_North, "MN_N",800000,100000,46.500000,-93.100000,47.033333,48.633333),
  StatePlaneLCC(PCS_NAD83_Minnesota_Central, "MN_C",800000,100000,45.000000,-94.250000,45.616667,47.050000),
  StatePlaneLCC(PCS_NAD83_Minnesota_South, "MN_S",800000,100000,43.000000,-94.000000,43.783333,45.216667),
  StatePlaneLCC(PCS_NAD83_Montana, "MT",600000,0,44.250000,-109.500000,45.000000,49.000000),
  StatePlaneLCC(PCS_NAD83_Nebraska, "NE",500000,0,39.833333,-100.000000,40.000000,43.000000),
  StatePlaneLCC(PCS_NAD83_New_York_Long_Is, "NY_LI",300000,0,40.166667,-74.000000,40.666667,41.033333),
  StatePlaneLCC(PCS_NAD83_North_Carolina, "NC",609601.22,0,33.750000,-79.000000,34.333333,36.166667),
  StatePlaneLCC(PCS_NAD83_North_Dakota_N, "ND_N",600000,0,47.000000,-100.500000,47.433333,48.733333),
  StatePlaneLCC(PCS_NAD83_North_Dakota_S, "ND_S",600000,0,45.666667,-100.500000,46.183333,47.483333),
  StatePlaneLCC(PCS_NAD83_Ohio_North, "OH_N",600000,0,39.666667,-82.500000,40.433333,41.700000),
  StatePlaneLCC(PCS_NAD83_Ohio_South, "OH_S",600000,0,38.000000,-82.500000,38.733333,40.033333),
  StatePlaneLCC(PCS_NAD83_Oklahoma_North, "OK_N",600000,0,35.000000,-98.000000,35.566667,36.766667),
  StatePlaneLCC(PCS_NAD83_Oklahoma_South, "OK_S",600000,0,33.333333,-98.000000,33.933333,35.233333),
  StatePlaneLCC(PCS_NAD83_Oregon_North, "OR_N",2500000,0,43.666667,-120.500000,44.333333,46.000000),
  StatePlaneLCC(PCS_NAD83_Oregon_South, "OR_S",1500000,0,41.666667,-120.500000,42.333333,44.000000),
  StatePlaneLCC(PCS_NAD83_Pennsylvania_N, "PA_N",600000,0,40.166667,-77.750000,40.883333,41.950000),
  StatePlaneLCC(PCS_NAD83_Pennsylvania_S, "PA_S",600000,0,39.333333,-77.750000,39.933333,40.966667),
  StatePlaneLCC(PCS_NAD83_Puerto_Rico, "PR",200000,200000,17.833333,-66.433333,18.033333,18.433333),
  StatePlaneLCC(PCS_NAD83_South_Carolina, "SC",609600,0,31.833333,-81.000000,32.500000,34.833333),
  StatePlaneLCC(PCS_NAD83_South_Dakota_N, "SD_N",600000,0,43.833333,-100.000000,44.416667,45.683333),
  StatePlaneLCC(PCS_NAD83_South_Dakota_S, "SD_S",600000,0,42.333333,-100.333333,42.833333,44.400000),
  StatePlaneLCC(PCS_NAD83_Tennessee, "TN",600000,0,34.333333,-86.000000,35.250000,36.416667),
  StatePlaneLCC(PCS_NAD83_Texas_North, "TX_N",200000,1000000,34.000000,-101.500000,34.650000,36.183333),
  StatePlaneLCC(PCS_NAD83_Texas_North_Central, "TX_NC",600000,2000000,31.666667,-98.500000,32.133333,33.966667),
  StatePlaneLCC(PCS_NAD83_Texas_Central, "TX_C",700000,3000000,29.666667,-100.333333,30.116667,31.883333),
  StatePlaneLCC(PCS_NAD83_Texas_South_Central, "TX_SC",600000,4000000,27.833333,-99.000000,28.383333,30.283333),
  StatePlaneLCC(PCS_NAD83_Texas_South, "TX_S",300000,5000000,25.666667,-98.500000,26.166667,27.833333),
  StatePlaneLCC(PCS_NAD83_Utah_North, "UT_N",500000,1000000,40.333333,-111.500000,40.716667,41.783333),
  StatePlaneLCC(PCS_NAD83_Utah_Central, "UT_C",500000,2000000,38.333333,-111.500000,39.016667,40.650000),
  StatePlaneLCC(PCS_NAD83_Utah_South, "UT_S",500000,3000000,36.666667,-111.500000,37.216667,38.350000),
  StatePlaneLCC(PCS_NAD83_Virginia_North, "VA_N",3500000,2000000,37.666667,-78.500000,38.033333,39.200000),
  StatePlaneLCC(PCS_NAD83_Virginia_South, "VA_S",3500000,1000000,36.333333,-78.500000,36.766667,37.966667),
  StatePlaneLCC(PCS_NAD83_Washington_North, "WA_N",500000,0,47.000000,-120.833333,47.500000,48.733333),
  StatePlaneLCC(PCS_NAD83_Washington_South, "WA_S",500000,0,45.333333,-120.500000,45.833333,47.333333),
  StatePlaneLCC(PCS_NAD83_West_Virginia_N, "WV_N",600000,0,38.500000,-79.500000,39.000000,40.250000),
  StatePlaneLCC(PCS_NAD83_West_Virginia_S, "WV_S",600000,0,37.000000,-81.000000,37.483333,38.883333),
  StatePlaneLCC(PCS_NAD83_Wisconsin_North, "WI_N",600000,0,45.166667,-90.000000,45.566667,46.766667),
  StatePlaneLCC(PCS_NAD83_Wisconsin_Central, "WI_C",600000,0,43.833333,-90.000000,44.250000,45.500000),
  StatePlaneLCC(PCS_NAD83_Wisconsin_South, "WI_S",600000,0,42.000000,-90.000000,42.733333,44.066667),
  StatePlaneLCC(0,0,-1,-1,-1,-1,-1,-1)
};

class StatePlaneTM
{
public:
  StatePlaneTM(short geokey, char* zone, F64 falseEasting, F64 falseNorthing, F64 latOriginDegree, F64 longMeridianDegree, F64 scaleFactor)
  {
    this->geokey = geokey;
    this->zone = zone;
    this->falseEasting = falseEasting;
    this->falseNorthing = falseNorthing;
    this->latOriginDegree = latOriginDegree;
    this->longMeridianDegree = longMeridianDegree;
    this->scaleFactor = scaleFactor;
  }
  short geokey;
  char* zone;
  F64 falseEasting;
  F64 falseNorthing;
  F64 latOriginDegree;
  F64 longMeridianDegree;
  F64 scaleFactor;
};

static const StatePlaneTM state_plane_tm_nad27_list[] =
{
  // geotiff key, zone, false east [m], false north [m], ProjOrig(Lat), CentMerid(Long), scale factor
  StatePlaneTM(PCS_NAD27_Alabama_East, "AL_E",152400.3048,0,30.5,-85.83333333,0.99996),
  StatePlaneTM(PCS_NAD27_Alabama_West, "AL_W",152400.3048,0,30,-87.5,0.999933333),
  StatePlaneTM(PCS_NAD27_Alaska_zone_2, "AK_2",152400.3048,0,54,-142,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_3, "AK_3",152400.3048,0,54,-146,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_4, "AK_4",152400.3048,0,54,-150,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_5, "AK_5",152400.3048,0,54,-154,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_6, "AK_6",152400.3048,0,54,-158,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_7, "AK_7",213360.4267,0,54,-162,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_8, "AK_8",152400.3048,0,54,-166,0.9999),
  StatePlaneTM(PCS_NAD27_Alaska_zone_9, "AK_9",182880.3658,0,54,-170,0.9999),
  StatePlaneTM(PCS_NAD27_Arizona_East, "AZ_E",152400.3048,0,31,-110.1666667,0.9999),
  StatePlaneTM(PCS_NAD27_Arizona_Central, "AZ_C",152400.3048,0,31,-111.9166667,0.9999),
  StatePlaneTM(PCS_NAD27_Arizona_West, "AZ_W",152400.3048,0,31,-113.75,0.999933333),
  StatePlaneTM(PCS_NAD27_Delaware, "DE",152400.3048,0,38,-75.41666667,0.999995),
  StatePlaneTM(PCS_NAD27_Florida_East, "FL_E",152400.3048,0,24.33333333,-81,0.999941177),
  StatePlaneTM(PCS_NAD27_Florida_West, "FL_W",152400.3048,0,24.33333333,-82,0.999941177),
  StatePlaneTM(PCS_NAD27_Georgia_East, "GA_E",152400.3048,0,30,-82.16666667,0.9999),
  StatePlaneTM(PCS_NAD27_Georgia_West, "GA_W",152400.3048,0,30,-84.16666667,0.9999),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_1, "HI_1",152400.3048,0,18.83333333,-155.5,0.999966667),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_2, "HI_2",152400.3048,0,20.33333333,-156.6666667,0.999966667),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_3, "HI_3",152400.3048,0,21.16666667,-158,0.99999),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_4, "HI_4",152400.3048,0,21.83333333,-159.5,0.99999),
  StatePlaneTM(PCS_NAD27_Hawaii_zone_5, "HI_5",152400.3048,0,21.66666667,-160.1666667,1),
  StatePlaneTM(PCS_NAD27_Idaho_East, "ID_E",152400.3048,0,41.66666667,-112.1666667,0.999947368),
  StatePlaneTM(PCS_NAD27_Idaho_Central, "ID_C",152400.3048,0,41.66666667,-114,0.999947368),
  StatePlaneTM(PCS_NAD27_Idaho_West, "ID_W",152400.3048,0,41.66666667,-115.75,0.999933333),
  StatePlaneTM(PCS_NAD27_Illinois_East, "IL_E",152400.3048,0,36.66666667,-88.33333333,0.999975),
  StatePlaneTM(PCS_NAD27_Illinois_West, "IL_W",152400.3048,0,36.66666667,-90.16666667,0.999941177),
  StatePlaneTM(PCS_NAD27_Indiana_East, "IN_E",152400.3048,0,37.5,-85.66666667,0.999966667),
  StatePlaneTM(PCS_NAD27_Indiana_West, "IN_W",152400.3048,0,37.5,-87.08333333,0.999966667),
  StatePlaneTM(PCS_NAD27_Maine_East, "ME_E",152400.3048,0,43.83333333,-68.5,0.9999),
  StatePlaneTM(PCS_NAD27_Maine_West, "ME_W",152400.3048,0,42.83333333,-70.16666667,0.999966667),
  StatePlaneTM(PCS_NAD27_Mississippi_East, "MS_E",152400.3048,0,29.66666667,-88.83333333,0.99996),
  StatePlaneTM(PCS_NAD27_Mississippi_West, "MS_W",152400.3048,0,30.5,-90.33333333,0.999941177),
  StatePlaneTM(PCS_NAD27_Missouri_East, "MO_E",152400.3048,0,35.83333333,-90.5,0.999933333),
  StatePlaneTM(PCS_NAD27_Missouri_Central, "MO_C",152400.3048,0,35.83333333,-92.5,0.999933333),
  StatePlaneTM(PCS_NAD27_Missouri_West, "MO_W",152400.3048,0,36.16666667,-94.5,0.999941177),
  StatePlaneTM(PCS_NAD27_Nevada_East, "NV_E",152400.3048,0,34.75,-115.5833333,0.9999),
  StatePlaneTM(PCS_NAD27_Nevada_Central, "NV_C",152400.3048,0,34.75,-116.6666667,0.9999),
  StatePlaneTM(PCS_NAD27_Nevada_West, "NV_W",152400.3048,0,34.75,-118.5833333,0.9999),
  StatePlaneTM(PCS_NAD27_New_Hampshire, "NH",152400.3048,0,42.5,-71.66666667,0.999966667),
  StatePlaneTM(PCS_NAD27_New_Jersey, "NJ",609601.2192,0,38.83333333,-74.66666667,0.999975),
  StatePlaneTM(PCS_NAD27_New_Mexico_East, "NM_E",152400.3048,0,31,-104.3333333,0.999909091),
  StatePlaneTM(PCS_NAD27_New_Mexico_Central, "NM_C",152400.3048,0,31,-106.25,0.9999),
  StatePlaneTM(PCS_NAD27_New_Mexico_West, "NM_W",152400.3048,0,31,-107.8333333,0.999916667),
  StatePlaneTM(PCS_NAD27_New_York_East, "NY_E",152400.3048,0,40,-74.33333333,0.999966667),
  StatePlaneTM(PCS_NAD27_New_York_Central, "NY_C",152400.3048,0,40,-76.58333333,0.9999375),
  StatePlaneTM(PCS_NAD27_New_York_West, "NY_W",152400.3048,0,40,-78.58333333,0.9999375),
  StatePlaneTM(PCS_NAD27_Rhode_Island, "RI",152400.3048,0,41.08333333,-71.5,0.99999375),
  StatePlaneTM(PCS_NAD27_Vermont, "VT",152400.3048,0,42.5,-72.5,0.999964286),
  StatePlaneTM(PCS_NAD27_Wyoming_East, "WY_E",152400.3048,0,40.66666667,-105.1666667,0.999941177),
  StatePlaneTM(PCS_NAD27_Wyoming_East_Central, "WY_EC",152400.3048,0,40.66666667,-107.3333333,0.999941177),
  StatePlaneTM(PCS_NAD27_Wyoming_West_Central, "WY_WC",152400.3048,0,40.66666667,-108.75,0.999941177),
  StatePlaneTM(PCS_NAD27_Wyoming_West, "WY_W",152400.3048,0,40.66666667,-110.0833333,0.999941177),
  StatePlaneTM(0,0,-1,-1,-1,-1,-1)
};

static const StatePlaneTM state_plane_tm_nad83_list[] =
{
  // geotiff key, zone, false east [m], false north [m], ProjOrig(Lat), CentMerid(Long), scale factor
  StatePlaneTM(PCS_NAD83_Alabama_East, "AL_E",200000,0,30.5,-85.83333333,0.99996),
  StatePlaneTM(PCS_NAD83_Alabama_West, "AL_W",600000,0,30,-87.5,0.999933333),
  StatePlaneTM(PCS_NAD83_Alaska_zone_2, "AK_2",500000,0,54,-142,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_3, "AK_3",500000,0,54,-146,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_4, "AK_4",500000,0,54,-150,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_5, "AK_5",500000,0,54,-154,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_6, "AK_6",500000,0,54,-158,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_7, "AK_7",500000,0,54,-162,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_8, "AK_8",500000,0,54,-166,0.9999),
  StatePlaneTM(PCS_NAD83_Alaska_zone_9, "AK_9",500000,0,54,-170,0.9999),
  StatePlaneTM(PCS_NAD83_Arizona_East, "AZ_E",213360,0,31,-110.1666667,0.9999),
  StatePlaneTM(PCS_NAD83_Arizona_Central, "AZ_C",213360,0,31,-111.9166667,0.9999),
  StatePlaneTM(PCS_NAD83_Arizona_West, "AZ_W",213360,0,31,-113.75,0.999933333),
  StatePlaneTM(PCS_NAD83_Delaware, "DE",200000,0,38,-75.41666667,0.999995),
  StatePlaneTM(PCS_NAD83_Florida_East, "FL_E",200000,0,24.33333333,-81,0.999941177),
  StatePlaneTM(PCS_NAD83_Florida_West, "FL_W",200000,0,24.33333333,-82,0.999941177),
  StatePlaneTM(PCS_NAD83_Georgia_East, "GA_E",200000,0,30,-82.16666667,0.9999),
  StatePlaneTM(PCS_NAD83_Georgia_West, "GA_W",700000,0,30,-84.16666667,0.9999),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_1, "HI_1",500000,0,18.83333333,-155.5,0.999966667),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_2, "HI_2",500000,0,20.33333333,-156.6666667,0.999966667),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_3, "HI_3",500000,0,21.16666667,-158,0.99999),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_4, "HI_4",500000,0,21.83333333,-159.5,0.99999),
  StatePlaneTM(PCS_NAD83_Hawaii_zone_5, "HI_5",500000,0,21.66666667,-160.1666667,1),
  StatePlaneTM(PCS_NAD83_Idaho_East, "ID_E",200000,0,41.66666667,-112.1666667,0.999947368),
  StatePlaneTM(PCS_NAD83_Idaho_Central, "ID_C",500000,0,41.66666667,-114,0.999947368),
  StatePlaneTM(PCS_NAD83_Idaho_West, "ID_W",800000,0,41.66666667,-115.75,0.999933333),
  StatePlaneTM(PCS_NAD83_Illinois_East, "IL_E",300000,0,36.66666667,-88.33333333,0.999975),
  StatePlaneTM(PCS_NAD83_Illinois_West, "IL_W",700000,0,36.66666667,-90.16666667,0.999941177),
  StatePlaneTM(PCS_NAD83_Indiana_East, "IN_E",100000,250000,37.5,-85.66666667,0.999966667),
  StatePlaneTM(PCS_NAD83_Indiana_West, "IN_W",900000,250000,37.5,-87.08333333,0.999966667),
  StatePlaneTM(PCS_NAD83_Maine_East, "ME_E",300000,0,43.66666667,-68.5,0.9999),
  StatePlaneTM(PCS_NAD83_Maine_West, "ME_W",900000,0,42.83333333,-70.16666667,0.999966667),
  StatePlaneTM(PCS_NAD83_Mississippi_East, "MS_E",300000,0,29.5,-88.83333333,0.99995),
  StatePlaneTM(PCS_NAD83_Mississippi_West, "MS_W",700000,0,29.5,-90.33333333,0.99995),
  StatePlaneTM(PCS_NAD83_Missouri_East, "MO_E",250000,0,35.83333333,-90.5,0.999933333),
  StatePlaneTM(PCS_NAD83_Missouri_Central, "MO_C",500000,0,35.83333333,-92.5,0.999933333),
  StatePlaneTM(PCS_NAD83_Missouri_West, "MO_W",850000,0,36.16666667,-94.5,0.999941177),
  StatePlaneTM(PCS_NAD83_Nevada_East, "NV_E",200000,8000000,34.75,-115.5833333,0.9999),
  StatePlaneTM(PCS_NAD83_Nevada_Central, "NV_C",500000,6000000,34.75,-116.6666667,0.9999),
  StatePlaneTM(PCS_NAD83_Nevada_West, "NV_W",800000,4000000,34.75,-118.5833333,0.9999),
  StatePlaneTM(PCS_NAD83_New_Hampshire, "NH",300000,0,42.5,-71.66666667,0.999966667),
  StatePlaneTM(PCS_NAD83_New_Jersey, "NJ",150000,0,38.83333333,-74.5,0.9999),
  StatePlaneTM(PCS_NAD83_New_Mexico_East, "NM_E",165000,0,31,-104.3333333,0.999909091),
  StatePlaneTM(PCS_NAD83_New_Mexico_Central, "NM_C",500000,0,31,-106.25,0.9999),
  StatePlaneTM(PCS_NAD83_New_Mexico_West, "NM_W",830000,0,31,-107.8333333,0.999916667),
  StatePlaneTM(PCS_NAD83_New_York_East, "NY_E",150000,0,38.83333333,-74.5,0.9999),
  StatePlaneTM(PCS_NAD83_New_York_Central, "NY_C",250000,0,40,-76.58333333,0.9999375),
  StatePlaneTM(PCS_NAD83_New_York_West, "NY_W",350000,0,40,-78.58333333,0.9999375),
  StatePlaneTM(PCS_NAD83_Rhode_Island, "RI",100000,0,41.08333333,-71.5,0.99999375),
  StatePlaneTM(PCS_NAD83_Vermont, "VT",500000,0,42.5,-72.5,0.999964286),
  StatePlaneTM(PCS_NAD83_Wyoming_East, "WY_E",200000,0,40.5,-105.1666667,0.9999375),
  StatePlaneTM(PCS_NAD83_Wyoming_East_Central, "WY_EC",400000,100000,40.5,-107.3333333,0.9999375),
  StatePlaneTM(PCS_NAD83_Wyoming_West_Central, "WY_WC",600000,0,40.5,-108.75,0.9999375),
  StatePlaneTM(PCS_NAD83_Wyoming_West, "WY_W",800000,100000,40.5,-110.0833333,0.9999375),
  StatePlaneTM(0,0,-1,-1,-1,-1,-1)
};

static I32 lidardouble2string(CHAR* string, F64 value)
{
  I32 len;
  len = sprintf(string, "%.15f", value) - 1;
  while (string[len] == '0') len--;
  if (string[len] != '.') len++;
  string[len] = '\0';
  return len;
};

static I32 lidardouble2string(CHAR* string, F64 value, F64 scale)
{
  I32 decimal_digits = 0;
  while (scale < 1.0)
  {
    scale *= 10;
    decimal_digits++;
  }
  if (decimal_digits == 0)
    sprintf(string, "%d", (I32)value);
  else if (decimal_digits == 1)
    sprintf(string, "%.1f", value);
  else if (decimal_digits == 2)
    sprintf(string, "%.2f", value);
  else if (decimal_digits == 3)
    sprintf(string, "%.3f", value);
  else if (decimal_digits == 4)
    sprintf(string, "%.4f", value);
  else if (decimal_digits == 5)
    sprintf(string, "%.5f", value);
  else if (decimal_digits == 6)
    sprintf(string, "%.6f", value);
  else if (decimal_digits == 7)
    sprintf(string, "%.7f", value);
  else if (decimal_digits == 8)
    sprintf(string, "%.8f", value);
  else
    return lidardouble2string(string, value);
  return strlen(string)-1;
};

void CRScheck::set_coordinates_in_survey_feet(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 3;
  else
    coordinate_units[1] = 3;
}

void CRScheck::set_coordinates_in_feet(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 2;
  else
    coordinate_units[1] = 2;
}

void CRScheck::set_coordinates_in_meter(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 1;
  else
    coordinate_units[1] = 1;
}

void CRScheck::set_elevation_in_survey_feet(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 3;
  else
    coordinate_units[1] = 3;
}

void CRScheck::set_elevation_in_feet(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 2;
  else
    coordinate_units[1] = 2;
}

void CRScheck::set_elevation_in_meter(const BOOL from_geokeys)
{
  if (from_geokeys)
    coordinate_units[0] = 1;
  else
    coordinate_units[1] = 1;
}

BOOL CRScheck::set_ellipsoid(const I32 ellipsoid_id, const BOOL from_geokeys, char* description)
{
  if ((ellipsoid_id <= 0) || (ellipsoid_id >= 25))
  {
    return FALSE;
  }

  CRSprojectionEllipsoid* ellipsoid;

  if (from_geokeys)
  {
    if (ellipsoids[0] == 0)
    {
      ellipsoids[0] = new CRSprojectionEllipsoid();
    }
    ellipsoid = ellipsoids[0];
  }
  else
  {
    if (ellipsoids[1] == 0)
    {
      ellipsoids[1] = new CRSprojectionEllipsoid();
    }
    ellipsoid = ellipsoids[1];
  }

  ellipsoid->id = ellipsoid_id;
  ellipsoid->name = ellipsoid_list[ellipsoid_id].name;
  ellipsoid->equatorial_radius = ellipsoid_list[ellipsoid_id].equatorialRadius;
  ellipsoid->eccentricity_squared = ellipsoid_list[ellipsoid_id].eccentricitySquared;
  ellipsoid->eccentricity_prime_squared = (ellipsoid->eccentricity_squared)/(1-ellipsoid->eccentricity_squared);
  ellipsoid->polar_radius = ellipsoid->equatorial_radius*sqrt(1-ellipsoid->eccentricity_squared);    
  ellipsoid->eccentricity = sqrt(ellipsoid->eccentricity_squared);
  ellipsoid->eccentricity_e1 = (1-sqrt(1-ellipsoid->eccentricity_squared))/(1+sqrt(1-ellipsoid->eccentricity_squared));

  if (description)
  {
    sprintf(description, "%2d - %s (%g %g)", ellipsoid->id, ellipsoid->name, ellipsoid->equatorial_radius, ellipsoid->eccentricity_squared);
  }

  return TRUE;
}

void CRScheck::set_projection(CRSprojectionParameters* projection, const BOOL from_geokeys)
{
  if (from_geokeys)
  {
    if (projections[0]) delete projections[0];
    projections[0] = projection;
  }
  else
  {
    if (projections[1]) delete projections[1];
    projections[1] = projection;
  }
}

BOOL CRScheck::set_no_projection(const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParameters* no = new CRSprojectionParameters();
  no->type = CRS_PROJECTION_NONE;
  sprintf(no->name, "intentionally no projection");
  set_projection(no, from_geokeys);
  if (description)
  {
    sprintf(description, "%s", no->name);
  }
  return TRUE;
}

BOOL CRScheck::set_latlong_projection(const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParameters* latlong = new CRSprojectionParameters();
  latlong->type = CRS_PROJECTION_LAT_LONG;
  sprintf(latlong->name, "latitude/longitude");
  set_projection(latlong, from_geokeys);
  if (description)
  {
    sprintf(description, "%s", latlong->name);
  }
  return TRUE;
}

BOOL CRScheck::set_longlat_projection(const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParameters* longlat = new CRSprojectionParameters();
  longlat->type = CRS_PROJECTION_LONG_LAT;
  sprintf(longlat->name, "longitude/latitude");
  set_projection(longlat, from_geokeys);
  if (description)
  {
    sprintf(description, "%s", longlat->name);
  }
  return TRUE;
}

BOOL CRScheck::set_ecef_projection(const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParameters* ecef = new CRSprojectionParameters();
  ecef->type = CRS_PROJECTION_ECEF;
  sprintf(ecef->name, "ECEF");
  set_projection(ecef, from_geokeys);
  if (description)
  {
    sprintf(description, "%s", ecef->name);
  }
  return TRUE;
}

BOOL CRScheck::set_utm_projection(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 zone_number;
  CHAR* zone_letter;
  zone_number = strtoul(zone, &zone_letter, 10);
  if ((*zone_letter < 'C') || (*zone_letter > 'X'))
  {
    return FALSE;
  }
  if ((zone_number < 0) || (zone_number > 60))
  {
    return FALSE;
  }
  CRSprojectionParametersUTM* utm = new CRSprojectionParametersUTM();
  utm->type = CRS_PROJECTION_UTM;
  utm->utm_zone_number = zone_number;
  utm->utm_zone_letter = *zone_letter;
  if((*zone_letter - 'N') >= 0)
  {
    utm->utm_northern_hemisphere = TRUE; // point is in northern hemisphere
  }
  else
  {
    utm->utm_northern_hemisphere = FALSE; //point is in southern hemisphere
  }
  sprintf(utm->name, "UTM zone %s (%s)", zone, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  utm->utm_long_origin = (zone_number - 1) * 6 - 180 + 3; // + 3 puts origin in middle of zone
  set_projection(utm, from_geokeys);
  if (description)
  {
    sprintf(description, "UTM %d %s", zone_number, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  }
  return TRUE;
}

BOOL CRScheck::set_utm_projection(const I32 zone_number, const BOOL northern, const BOOL from_geokeys, CHAR* description)
{
  if ((zone_number < 0) || (zone_number > 60))
  {
    return FALSE;
  }
  CRSprojectionParametersUTM* utm = new CRSprojectionParametersUTM();
  utm->type = CRS_PROJECTION_UTM;
  utm->utm_zone_number = zone_number;
  utm->utm_zone_letter = ' ';
  utm->utm_northern_hemisphere = northern;
  sprintf(utm->name, "UTM zone %d (%s)", zone_number, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  utm->utm_long_origin = (zone_number - 1) * 6 - 180 + 3;  // + 3 puts origin in middle of zone
  set_projection(utm, from_geokeys);
  if (description)
  {
    sprintf(description, "UTM %d %s", zone_number, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  }
  return TRUE;
}

BOOL CRScheck::set_mga_projection(const I32 zone_number, const BOOL northern, const BOOL from_geokeys, CHAR* description)
{
  if ((zone_number < 0) || (zone_number > 60))
  {
    return FALSE;
  }
  CRSprojectionParametersUTM* utm = new CRSprojectionParametersUTM();
  utm->type = CRS_PROJECTION_UTM;
  utm->utm_zone_number = zone_number;
  utm->utm_zone_letter = ' ';
  utm->utm_northern_hemisphere = northern;
  sprintf(utm->name, "MGA zone %d (%s)", zone_number, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  utm->utm_long_origin = (zone_number - 1) * 6 - 180 + 3;  // + 3 puts origin in middle of zone
  set_projection(utm, from_geokeys);
  if (description)
  {
    sprintf(description, "MGA %d %s", zone_number, (utm->utm_northern_hemisphere ? "northern hemisphere" : "southern hemisphere"));
  }
  return TRUE;
}

// Configure a Lambert Conic Conformal Projection
//
// The function Set_Lambert_Parameters receives the ellipsoid parameters and
// Lambert Conformal Conic projection parameters as inputs, and sets the
// corresponding state variables.
//
// falseEasting & falseNorthing are just an offset in meters added 
// to the final coordinate calculated.
//
// latOriginDegree & longMeridianDegree are the "center" latitiude and
// longitude in decimal degrees of the area being projected. All coordinates
// will be calculated in meters relative to this point on the earth.
//
// firstStdParallelDegree & secondStdParallelDegree are the two lines of
// longitude in decimal degrees (that is they run east-west) that define
// where the "cone" intersects the earth. They bracket the area being projected.
void CRScheck::set_lambert_conformal_conic_projection(const F64 falseEasting, const F64 falseNorthing, const F64 latOriginDegree, const F64 longMeridianDegree, const F64 firstStdParallelDegree, const F64 secondStdParallelDegree, const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParametersLCC* lcc = new CRSprojectionParametersLCC();
  lcc->type = CRS_PROJECTION_LCC;
  sprintf(lcc->name, "Lambert Conformal Conic");
  lcc->lcc_false_easting_meter = falseEasting;
  lcc->lcc_false_northing_meter = falseNorthing;
  lcc->lcc_lat_origin_degree = latOriginDegree;
  lcc->lcc_long_meridian_degree = longMeridianDegree;
  lcc->lcc_first_std_parallel_degree = firstStdParallelDegree;
  lcc->lcc_second_std_parallel_degree = secondStdParallelDegree;
  lcc->lcc_lat_origin_radian = deg2rad*lcc->lcc_lat_origin_degree;
  lcc->lcc_long_meridian_radian = deg2rad*lcc->lcc_long_meridian_degree;
  lcc->lcc_first_std_parallel_radian = deg2rad*lcc->lcc_first_std_parallel_degree;
  lcc->lcc_second_std_parallel_radian = deg2rad*lcc->lcc_second_std_parallel_degree;
  set_projection(lcc, from_geokeys);
  if (description)
  {
    sprintf(description, "false east/north: %g/%g [m], origin lat/ meridian long: %g/%g, parallel 1st/2nd: %g/%g", lcc->lcc_false_easting_meter, lcc->lcc_false_northing_meter, lcc->lcc_lat_origin_degree, lcc->lcc_long_meridian_degree, lcc->lcc_first_std_parallel_degree, lcc->lcc_second_std_parallel_degree);
  }
}

/*
  * The function set_transverse_mercator_projection receives the Tranverse
  * Mercator projection parameters as input and sets the corresponding state
  * variables. 
  * falseEasting   : Easting/X in meters at the center of the projection
  * falseNorthing  : Northing/Y in meters at the center of the projection
  * latOriginDegree     : Latitude in decimal degree at the origin of the projection
  * longMeridianDegree  : Longitude n decimal degree at the center of the projection
  * scaleFactor         : Projection scale factor
*/
void CRScheck::set_transverse_mercator_projection(const F64 falseEasting, const F64 falseNorthing, const F64 latOriginDegree, const F64 longMeridianDegree, const F64 scaleFactor, const BOOL from_geokeys, CHAR* description)
{
  CRSprojectionParametersTM* tm = new CRSprojectionParametersTM();
  tm->type = CRS_PROJECTION_TM;
  sprintf(tm->name, "Transverse Mercator");
  tm->tm_false_easting_meter = falseEasting;
  tm->tm_false_northing_meter = falseNorthing;
  tm->tm_lat_origin_degree = latOriginDegree;
  tm->tm_long_meridian_degree = longMeridianDegree;
  tm->tm_scale_factor = scaleFactor;
  tm->tm_lat_origin_radian = deg2rad*tm->tm_lat_origin_degree;
  tm->tm_long_meridian_radian = deg2rad*tm->tm_long_meridian_degree;
  set_projection(tm, from_geokeys);
  if (description)
  {
    sprintf(description, "false east/north: %g/%g [m], origin lat/meridian long: %g/%g, scale: %g", tm->tm_false_easting_meter, tm->tm_false_northing_meter, tm->tm_lat_origin_degree, tm->tm_long_meridian_degree, tm->tm_scale_factor);
  }
}

BOOL CRScheck::set_state_plane_nad27_lcc(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 i = 0;
  while (state_plane_lcc_nad27_list[i].zone)
  {
    if (strcmp(zone, state_plane_lcc_nad27_list[i].zone) == 0)
    {
      set_ellipsoid(CRS_ELLIPSOID_NAD27, from_geokeys);
      set_lambert_conformal_conic_projection(state_plane_lcc_nad27_list[i].falseEasting, state_plane_lcc_nad27_list[i].falseNorthing, state_plane_lcc_nad27_list[i].latOriginDegree, state_plane_lcc_nad27_list[i].longMeridianDegree, state_plane_lcc_nad27_list[i].firstStdParallelDegree, state_plane_lcc_nad27_list[i].secondStdParallelDegree, from_geokeys);
      if (description)
      {
        sprintf(description, "stateplane27 %s", state_plane_lcc_nad27_list[i].zone);
      }
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

BOOL CRScheck::set_state_plane_nad83_lcc(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 i = 0;
  while (state_plane_lcc_nad83_list[i].zone)
  {
    if (strcmp(zone, state_plane_lcc_nad83_list[i].zone) == 0)
    {
      set_ellipsoid(CRS_ELLIPSOID_NAD83, from_geokeys);
      set_lambert_conformal_conic_projection(state_plane_lcc_nad83_list[i].falseEasting, state_plane_lcc_nad83_list[i].falseNorthing, state_plane_lcc_nad83_list[i].latOriginDegree, state_plane_lcc_nad83_list[i].longMeridianDegree, state_plane_lcc_nad83_list[i].firstStdParallelDegree, state_plane_lcc_nad83_list[i].secondStdParallelDegree, from_geokeys);
      if (description)
      {
        sprintf(description, "stateplane83 %s", state_plane_lcc_nad83_list[i].zone);
      }
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

BOOL CRScheck::set_state_plane_nad27_tm(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 i = 0;
  while (state_plane_tm_nad27_list[i].zone)
  {
    if (strcmp(zone, state_plane_tm_nad27_list[i].zone) == 0)
    {
      set_ellipsoid(CRS_ELLIPSOID_NAD27, from_geokeys);
      set_transverse_mercator_projection(state_plane_tm_nad27_list[i].falseEasting, state_plane_tm_nad27_list[i].falseNorthing, state_plane_tm_nad27_list[i].latOriginDegree, state_plane_tm_nad27_list[i].longMeridianDegree, state_plane_tm_nad27_list[i].scaleFactor, from_geokeys);
      if (description)
      {
        sprintf(description, "stateplane27 %s", state_plane_tm_nad27_list[i].zone);
      }
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

BOOL CRScheck::set_state_plane_nad83_tm(const CHAR* zone, const BOOL from_geokeys, CHAR* description)
{
  I32 i = 0;
  while (state_plane_tm_nad83_list[i].zone)
  {
    if (strcmp(zone, state_plane_tm_nad83_list[i].zone) == 0)
    {
      set_ellipsoid(CRS_ELLIPSOID_NAD83, from_geokeys);
      set_transverse_mercator_projection(state_plane_tm_nad83_list[i].falseEasting, state_plane_tm_nad83_list[i].falseNorthing, state_plane_tm_nad83_list[i].latOriginDegree, state_plane_tm_nad83_list[i].longMeridianDegree, state_plane_tm_nad83_list[i].scaleFactor, from_geokeys);
      if (description)
      {
        sprintf(description, "stateplane83 %s", state_plane_tm_nad83_list[i].zone);
      }
      return TRUE;
    }
    i++;
  }
  return FALSE;
}

BOOL CRScheck::set_vertical_from_VerticalCSTypeGeoKey(U16 value)
{
  if ((5000 <= value) && (value <= 5099))      // [5000, 5099] = EPSG Ellipsoid Vertical CS Codes
  {
    vertical_epsg[1] = value;
  }
  else if ((5101 <= value) && (value <= 5199)) // [5100, 5199] = EPSG Orthometric Vertical CS Codes
  {
    vertical_epsg[1] = value;
  }
  else if ((5200 <= value) && (value <= 5999)) // [5200, 5999] = Reserved EPSG
  {
    vertical_epsg[1] = value;
  }
  else
  {
//    fprintf(stderr, "set_VerticalCSTypeGeoKey: look-up for %d not implemented\012", value);
    return FALSE;
  }
  return TRUE;
}

BOOL CRScheck::set_coordinates_from_ProjLinearUnitsGeoKey(U16 value)
{
  switch (value)
  {
  case 9001: // Linear_Meter
    set_coordinates_in_meter(TRUE);
    break;
  case 9002: // Linear_Foot
    set_coordinates_in_feet(TRUE);
    break;
  case 9003: // Linear_Foot_US_Survey
    set_coordinates_in_survey_feet(TRUE);
    break;
  default:
//    fprintf(stderr, "set_VerticalUnitsGeoKey: look-up for %d not implemented\n", value);
    return FALSE;
  }
  return TRUE;
}

BOOL CRScheck::set_elevation_from_VerticalUnitsGeoKey(U16 value)
{
  switch (value)
  {
  case 9001: // Linear_Meter
    set_elevation_in_meter(TRUE);
    break;
  case 9002: // Linear_Foot
    set_elevation_in_feet(TRUE);
    break;
  case 9003: // Linear_Foot_US_Survey
    set_elevation_in_survey_feet(TRUE);
    break;
  default:
//    fprintf(stderr, "set_VerticalUnitsGeoKey: look-up for %d not implemented\n", value);
    return FALSE;
  }
  return TRUE;
}

// other supported EPSG codes (with resolving of projection)

static const short EPSG_IRENET95_Irish_Transverse_Mercator = 2157;
static const short EPSG_ETRS89_Poland_CS92 = 2180;
static const short EPSG_NZGD2000 = 2193;
static const short EPSG_NAD83_Maryland_ftUS = 2248;
static const short EPSG_NAD83_HARN_UTM2_South_American_Samoa = 2195;
static const short EPSG_NAD83_HARN_Virginia_North_ftUS = 2924;
static const short EPSG_NAD83_HARN_Virginia_South_ftUS = 2925;
static const short EPSG_Reseau_Geodesique_Francais_Guyane_1995 = 2972;
static const short EPSG_NAD83_Oregon_Lambert = 2991;
static const short EPSG_NAD83_Oregon_Lambert_ft = 2992;
static const short EPSG_SWEREF99_TM = 3006;
static const short EPSG_ETRS89_ETRS_LCC = 3034;
static const short EPSG_ETRS89_ETRS_TM34 = 3046;
static const short EPSG_ETRS89_ETRS_TM35 = 3047;
static const short EPSG_ETRS89_ETRS_TM36 = 3048;
static const short EPSG_ETRS89_ETRS_TM35FIN = 3067;
static const short EPSG_Fiji_1956_UTM60_South = 3141;
static const short EPSG_Fiji_1956_UTM1_South = 3142;
static const short EPSG_Fiji_Map_Grid_1986 = 3460;
static const short EPSG_NAD83_NSRS2007_Maryland_ftUS = 3582;
static const short EPSG_Slovene_National_Grid_1996 = 3794;
static const short EPSG_MGI_1901_Slovene_National_Grid = 3912;
static const short EPSG_RGF93_CC42 = 3942;
static const short EPSG_RGF93_CC43 = 3943;
static const short EPSG_RGF93_CC44 = 3944;
static const short EPSG_RGF93_CC45 = 3945;
static const short EPSG_RGF93_CC46 = 3946;
static const short EPSG_RGF93_CC47 = 3947;
static const short EPSG_RGF93_CC48 = 3948;
static const short EPSG_RGF93_CC49 = 3949;
static const short EPSG_RGF93_CC50 = 3950;
static const short EPSG_ETRS89_DKTM1 = 4093;
static const short EPSG_ETRS89_DKTM2 = 4094;
static const short EPSG_ETRS89_DKTM3 = 4095;
static const short EPSG_ETRS89_DKTM4 = 4096;
static const short EPSG_ETRS89_UTM32_north_zE_N = 4647;
static const short EPSG_ETRS89_NTM_zone_5 = 5105;
static const short EPSG_ETRS89_NTM_zone_6 = 5106;
static const short EPSG_ETRS89_NTM_zone_7 = 5107;
static const short EPSG_ETRS89_NTM_zone_8 = 5108;
static const short EPSG_ETRS89_NTM_zone_9 = 5109;
static const short EPSG_ETRS89_NTM_zone_10 = 5110;
static const short EPSG_ETRS89_NTM_zone_11 = 5111;
static const short EPSG_ETRS89_NTM_zone_12 = 5112;
static const short EPSG_ETRS89_NTM_zone_13 = 5113;
static const short EPSG_ETRS89_NTM_zone_14 = 5114;
static const short EPSG_ETRS89_NTM_zone_15 = 5115;
static const short EPSG_ETRS89_NTM_zone_16 = 5116;
static const short EPSG_ETRS89_NTM_zone_17 = 5117;
static const short EPSG_ETRS89_NTM_zone_18 = 5118;
static const short EPSG_ETRS89_NTM_zone_19 = 5119;
static const short EPSG_ETRS89_NTM_zone_20 = 5120;
static const short EPSG_ETRS89_NTM_zone_21 = 5121;
static const short EPSG_ETRS89_NTM_zone_22 = 5122;
static const short EPSG_ETRS89_NTM_zone_23 = 5123;
static const short EPSG_ETRS89_NTM_zone_24 = 5124;
static const short EPSG_ETRS89_NTM_zone_25 = 5125;
static const short EPSG_ETRS89_NTM_zone_26 = 5126;
static const short EPSG_ETRS89_NTM_zone_27 = 5127;
static const short EPSG_ETRS89_NTM_zone_28 = 5128;
static const short EPSG_ETRS89_NTM_zone_29 = 5129;
static const short EPSG_ETRS89_NTM_zone_30 = 5130;
static const short EPSG_ETRS89_UTM33_north_zE_N = 5650;
static const short EPSG_OSGB_1936 = 27700;
static const short EPSG_Belgian_Lambert_1972 = 31370;

// simple name look-up for all EPSG codes (without resolving of projection)

static const short epsg_codes[] =
{
2000,
2001,
2002,
2003,
2004,
2005,
2006,
2007,
2008,
2009,
2010,
2011,
2012,
2013,
2014,
2015,
2016,
2017,
2018,
2019,
2020,
2021,
2022,
2023,
2024,
2025,
2026,
2027,
2028,
2029,
2030,
2031,
2032,
2033,
2034,
2035,
2036,
2037,
2038,
2039,
2040,
2041,
2042,
2043,
2044,
2045,
2046,
2047,
2048,
2049,
2050,
2051,
2052,
2053,
2054,
2055,
2056,
2057,
2058,
2059,
2060,
2061,
2062,
2063,
2064,
2065,
2066,
2067,
2068,
2069,
2070,
2071,
2072,
2073,
2074,
2075,
2076,
2077,
2078,
2079,
2080,
2081,
2082,
2083,
2084,
2085,
2086,
2087,
2088,
2089,
2090,
2091,
2092,
2093,
2094,
2095,
2096,
2097,
2098,
2099,
2100,
2101,
2102,
2103,
2104,
2105,
2106,
2107,
2108,
2109,
2110,
2111,
2112,
2113,
2114,
2115,
2116,
2117,
2118,
2119,
2120,
2121,
2122,
2123,
2124,
2125,
2126,
2127,
2128,
2129,
2130,
2131,
2132,
2133,
2134,
2135,
2136,
2137,
2138,
2139,
2140,
2141,
2142,
2143,
2144,
2145,
2146,
2147,
2148,
2149,
2150,
2151,
2152,
2153,
2154,
2155,
2156,
2157,
2158,
2159,
2160,
2161,
2162,
2163,
2164,
2165,
2166,
2167,
2168,
2169,
2170,
2171,
2172,
2173,
2174,
2175,
2176,
2177,
2178,
2179,
2180,
2188,
2189,
2190,
2191,
2192,
2193,
2194,
2195,
2196,
2197,
2198,
2199,
2200,
2201,
2202,
2203,
2204,
2205,
2206,
2207,
2208,
2209,
2210,
2211,
2212,
2213,
2214,
2215,
2216,
2217,
2218,
2219,
2220,
2221,
2222,
2223,
2224,
2225,
2226,
2227,
2228,
2229,
2230,
2231,
2232,
2233,
2234,
2235,
2236,
2237,
2238,
2239,
2240,
2241,
2242,
2243,
2244,
2245,
2246,
2247,
2248,
2249,
2250,
2251,
2252,
2253,
2254,
2255,
2256,
2257,
2258,
2259,
2260,
2261,
2262,
2263,
2264,
2265,
2266,
2267,
2268,
2269,
2270,
2271,
2272,
2273,
2274,
2275,
2276,
2277,
2278,
2279,
2280,
2281,
2282,
2283,
2284,
2285,
2286,
2287,
2288,
2289,
2290,
2291,
2292,
2294,
2295,
2296,
2297,
2298,
2299,
2300,
2301,
2302,
2303,
2304,
2305,
2306,
2307,
2308,
2309,
2310,
2311,
2312,
2313,
2314,
2315,
2316,
2317,
2318,
2319,
2320,
2321,
2322,
2323,
2324,
2325,
2326,
2327,
2328,
2329,
2330,
2331,
2332,
2333,
2334,
2335,
2336,
2337,
2338,
2339,
2340,
2341,
2342,
2343,
2344,
2345,
2346,
2347,
2348,
2349,
2350,
2351,
2352,
2353,
2354,
2355,
2356,
2357,
2358,
2359,
2360,
2361,
2362,
2363,
2364,
2365,
2366,
2367,
2368,
2369,
2370,
2371,
2372,
2373,
2374,
2375,
2376,
2377,
2378,
2379,
2380,
2381,
2382,
2383,
2384,
2385,
2386,
2387,
2388,
2389,
2390,
2391,
2392,
2393,
2394,
2395,
2396,
2397,
2398,
2399,
2400,
2401,
2402,
2403,
2404,
2405,
2406,
2407,
2408,
2409,
2410,
2411,
2412,
2413,
2414,
2415,
2416,
2417,
2418,
2419,
2420,
2421,
2422,
2423,
2424,
2425,
2426,
2427,
2428,
2429,
2430,
2431,
2432,
2433,
2434,
2435,
2436,
2437,
2438,
2439,
2440,
2441,
2442,
2443,
2444,
2445,
2446,
2447,
2448,
2449,
2450,
2451,
2452,
2453,
2454,
2455,
2456,
2457,
2458,
2459,
2460,
2461,
2462,
2463,
2464,
2465,
2466,
2467,
2468,
2469,
2470,
2471,
2472,
2473,
2474,
2475,
2476,
2477,
2478,
2479,
2480,
2481,
2482,
2483,
2484,
2485,
2486,
2487,
2488,
2489,
2490,
2491,
2492,
2493,
2494,
2495,
2496,
2497,
2498,
2499,
2500,
2501,
2502,
2503,
2504,
2505,
2506,
2507,
2508,
2509,
2510,
2511,
2512,
2513,
2514,
2515,
2516,
2517,
2518,
2519,
2520,
2521,
2522,
2523,
2524,
2525,
2526,
2527,
2528,
2529,
2530,
2531,
2532,
2533,
2534,
2535,
2536,
2537,
2538,
2539,
2540,
2541,
2542,
2543,
2544,
2545,
2546,
2547,
2548,
2549,
2550,
2551,
2552,
2553,
2554,
2555,
2556,
2557,
2558,
2559,
2560,
2561,
2562,
2563,
2564,
2565,
2566,
2567,
2568,
2569,
2570,
2571,
2572,
2573,
2574,
2575,
2576,
2577,
2578,
2579,
2580,
2581,
2582,
2583,
2584,
2585,
2586,
2587,
2588,
2589,
2590,
2591,
2592,
2593,
2594,
2595,
2596,
2597,
2598,
2599,
2600,
2601,
2602,
2603,
2604,
2605,
2606,
2607,
2608,
2609,
2610,
2611,
2612,
2613,
2614,
2615,
2616,
2617,
2618,
2619,
2620,
2621,
2622,
2623,
2624,
2625,
2626,
2627,
2628,
2629,
2630,
2631,
2632,
2633,
2634,
2635,
2636,
2637,
2638,
2639,
2640,
2641,
2642,
2643,
2644,
2645,
2646,
2647,
2648,
2649,
2650,
2651,
2652,
2653,
2654,
2655,
2656,
2657,
2658,
2659,
2660,
2661,
2662,
2663,
2664,
2665,
2666,
2667,
2668,
2669,
2670,
2671,
2672,
2673,
2674,
2675,
2676,
2677,
2678,
2679,
2680,
2681,
2682,
2683,
2684,
2685,
2686,
2687,
2688,
2689,
2690,
2691,
2692,
2693,
2694,
2695,
2696,
2697,
2698,
2699,
2700,
2701,
2702,
2703,
2704,
2705,
2706,
2707,
2708,
2709,
2710,
2711,
2712,
2713,
2714,
2715,
2716,
2717,
2718,
2719,
2720,
2721,
2722,
2723,
2724,
2725,
2726,
2727,
2728,
2729,
2730,
2731,
2732,
2733,
2734,
2735,
2736,
2737,
2738,
2739,
2740,
2741,
2742,
2743,
2744,
2745,
2746,
2747,
2748,
2749,
2750,
2751,
2752,
2753,
2754,
2755,
2756,
2757,
2758,
2759,
2760,
2761,
2762,
2763,
2764,
2765,
2766,
2767,
2768,
2769,
2770,
2771,
2772,
2773,
2774,
2775,
2776,
2777,
2778,
2779,
2780,
2781,
2782,
2783,
2784,
2785,
2786,
2787,
2788,
2789,
2790,
2791,
2792,
2793,
2794,
2795,
2796,
2797,
2798,
2799,
2800,
2801,
2802,
2803,
2804,
2805,
2806,
2807,
2808,
2809,
2810,
2811,
2812,
2813,
2814,
2815,
2816,
2817,
2818,
2819,
2820,
2821,
2822,
2823,
2824,
2825,
2826,
2827,
2828,
2829,
2830,
2831,
2832,
2833,
2834,
2835,
2836,
2837,
2838,
2839,
2840,
2841,
2842,
2843,
2844,
2845,
2846,
2847,
2848,
2849,
2850,
2851,
2852,
2853,
2854,
2855,
2856,
2857,
2858,
2859,
2860,
2861,
2862,
2863,
2864,
2865,
2866,
2867,
2868,
2869,
2870,
2871,
2872,
2873,
2874,
2875,
2876,
2877,
2878,
2879,
2880,
2881,
2882,
2883,
2884,
2885,
2886,
2887,
2888,
2889,
2890,
2891,
2892,
2893,
2894,
2895,
2896,
2897,
2898,
2899,
2900,
2901,
2902,
2903,
2904,
2905,
2906,
2907,
2908,
2909,
2910,
2911,
2912,
2913,
2914,
2915,
2916,
2917,
2918,
2919,
2920,
2921,
2922,
2923,
2924,
2925,
2926,
2927,
2928,
2929,
2930,
2931,
2932,
2933,
2934,
2935,
2936,
2937,
2938,
2939,
2940,
2941,
2942,
2943,
2944,
2945,
2946,
2947,
2948,
2949,
2950,
2951,
2952,
2953,
2954,
2955,
2956,
2957,
2958,
2959,
2960,
2961,
2962,
2963,
2964,
2965,
2966,
2967,
2968,
2969,
2970,
2971,
2972,
2973,
2975,
2976,
2977,
2978,
2979,
2980,
2981,
2982,
2983,
2984,
2985,
2986,
2987,
2988,
2989,
2990,
2991,
2992,
2993,
2994,
2995,
2996,
2997,
2998,
2999,
3000,
3001,
3002,
3003,
3004,
3005,
3006,
3007,
3008,
3009,
3010,
3011,
3012,
3013,
3014,
3015,
3016,
3017,
3018,
3019,
3020,
3021,
3022,
3023,
3024,
3025,
3026,
3027,
3028,
3029,
3030,
3031,
3032,
3033,
3034,
3035,
3036,
3037,
3038,
3039,
3040,
3041,
3042,
3043,
3044,
3045,
3046,
3047,
3048,
3049,
3050,
3051,
3052,
3053,
3054,
3055,
3056,
3057,
3058,
3059,
3060,
3061,
3062,
3063,
3064,
3065,
3066,
3067,
3068,
3069,
3070,
3071,
3072,
3073,
3074,
3075,
3076,
3077,
3078,
3079,
3080,
3081,
3082,
3083,
3084,
3085,
3086,
3087,
3088,
3089,
3090,
3091,
3092,
3093,
3094,
3095,
3096,
3097,
3098,
3099,
3100,
3101,
3102,
3103,
3104,
3105,
3106,
3107,
3108,
3109,
3110,
3111,
3112,
3113,
3114,
3115,
3116,
3117,
3118,
3119,
3120,
3121,
3122,
3123,
3124,
3125,
3126,
3127,
3128,
3129,
3130,
3131,
3132,
3133,
3134,
3135,
3136,
3137,
3138,
3139,
3140,
3141,
3142,
3143,
3144,
3145,
3146,
3147,
3148,
3149,
3150,
3151,
3152,
3153,
3154,
3155,
3156,
3157,
3158,
3159,
3160,
3161,
3162,
3163,
3164,
3165,
3166,
3167,
3168,
3169,
3170,
3171,
3172,
3173,
3174,
3175,
3176,
3177,
3178,
3179,
3180,
3181,
3182,
3183,
3184,
3185,
3186,
3187,
3188,
3189,
3190,
3191,
3192,
3193,
3194,
3195,
3196,
3197,
3198,
3199,
3200,
3201,
3202,
3203,
3204,
3205,
3206,
3207,
3208,
3209,
3210,
3211,
3212,
3213,
3214,
3215,
3216,
3217,
3218,
3219,
3220,
3221,
3222,
3223,
3224,
3225,
3226,
3227,
3228,
3229,
3230,
3231,
3232,
3233,
3234,
3235,
3236,
3237,
3238,
3239,
3240,
3241,
3242,
3243,
3244,
3245,
3246,
3247,
3248,
3249,
3250,
3251,
3252,
3253,
3254,
3255,
3256,
3257,
3258,
3259,
3260,
3261,
3262,
3263,
3264,
3265,
3266,
3267,
3268,
3269,
3270,
3271,
3272,
3273,
3274,
3275,
3276,
3277,
3278,
3279,
3280,
3281,
3282,
3283,
3284,
3285,
3286,
3287,
3288,
3289,
3290,
3291,
3292,
3293,
3294,
3295,
3296,
3297,
3298,
3299,
3300,
3301,
3302,
3303,
3304,
3305,
3306,
3307,
3308,
3309,
3310,
3311,
3312,
3313,
3314,
3315,
3316,
3317,
3318,
3319,
3320,
3321,
3322,
3323,
3324,
3325,
3326,
3327,
3328,
3329,
3330,
3331,
3332,
3333,
3334,
3335,
3336,
3337,
3338,
3339,
3340,
3341,
3342,
3343,
3344,
3345,
3346,
3347,
3348,
3349,
3350,
3351,
3352,
3353,
3354,
3355,
3356,
3357,
3358,
3359,
3360,
3361,
3362,
3363,
3364,
3365,
3366,
3367,
3368,
3369,
3370,
3371,
3372,
3373,
3374,
3375,
3376,
3377,
3378,
3379,
3380,
3381,
3382,
3383,
3384,
3385,
3386,
3387,
3388,
3389,
3390,
3391,
3392,
3393,
3394,
3395,
3396,
3397,
3398,
3399,
3400,
3401,
3402,
3403,
3404,
3405,
3406,
3407,
3408,
3409,
3410,
3411,
3412,
3413,
3414,
3415,
3416,
3417,
3418,
3419,
3420,
3421,
3422,
3423,
3424,
3425,
3426,
3427,
3428,
3429,
3430,
3431,
3432,
3433,
3434,
3435,
3436,
3437,
3438,
3439,
3440,
3441,
3442,
3443,
3444,
3445,
3446,
3447,
3448,
3449,
3450,
3451,
3452,
3453,
3454,
3455,
3456,
3457,
3458,
3459,
3460,
3461,
3462,
3463,
3464,
3465,
3466,
3467,
3468,
3469,
3470,
3471,
3472,
3473,
3474,
3475,
3476,
3477,
3478,
3479,
3480,
3481,
3482,
3483,
3484,
3485,
3486,
3487,
3488,
3489,
3490,
3491,
3492,
3493,
3494,
3495,
3496,
3497,
3498,
3499,
3500,
3501,
3502,
3503,
3504,
3505,
3506,
3507,
3508,
3509,
3510,
3511,
3512,
3513,
3514,
3515,
3516,
3517,
3518,
3519,
3520,
3521,
3522,
3523,
3524,
3525,
3526,
3527,
3528,
3529,
3530,
3531,
3532,
3533,
3534,
3535,
3536,
3537,
3538,
3539,
3540,
3541,
3542,
3543,
3544,
3545,
3546,
3547,
3548,
3549,
3550,
3551,
3552,
3553,
3554,
3555,
3556,
3557,
3558,
3559,
3560,
3561,
3562,
3563,
3564,
3565,
3566,
3567,
3568,
3569,
3570,
3571,
3572,
3573,
3574,
3575,
3576,
3577,
3578,
3579,
3580,
3581,
3582,
3583,
3584,
3585,
3586,
3587,
3588,
3589,
3590,
3591,
3592,
3593,
3594,
3595,
3596,
3597,
3598,
3599,
3600,
3601,
3602,
3603,
3604,
3605,
3606,
3607,
3608,
3609,
3610,
3611,
3612,
3613,
3614,
3615,
3616,
3617,
3618,
3619,
3620,
3621,
3622,
3623,
3624,
3625,
3626,
3627,
3628,
3629,
3630,
3631,
3632,
3633,
3634,
3635,
3636,
3637,
3638,
3639,
3640,
3641,
3642,
3643,
3644,
3645,
3646,
3647,
3648,
3649,
3650,
3651,
3652,
3653,
3654,
3655,
3656,
3657,
3658,
3659,
3660,
3661,
3662,
3663,
3664,
3665,
3666,
3667,
3668,
3669,
3670,
3671,
3672,
3673,
3674,
3675,
3676,
3677,
3678,
3679,
3680,
3681,
3682,
3683,
3684,
3685,
3686,
3687,
3688,
3689,
3690,
3691,
3692,
3693,
3694,
3695,
3696,
3697,
3698,
3699,
3700,
3701,
3702,
3703,
3704,
3705,
3706,
3707,
3708,
3709,
3710,
3711,
3712,
3713,
3714,
3715,
3716,
3717,
3718,
3719,
3720,
3721,
3722,
3723,
3724,
3725,
3726,
3727,
3728,
3729,
3730,
3731,
3732,
3733,
3734,
3735,
3736,
3737,
3738,
3739,
3740,
3741,
3742,
3743,
3744,
3745,
3746,
3747,
3748,
3749,
3750,
3751,
3752,
3753,
3754,
3755,
3756,
3757,
3758,
3759,
3760,
3761,
3762,
3763,
3764,
3765,
3766,
3767,
3768,
3769,
3770,
3771,
3772,
3773,
3774,
3775,
3776,
3777,
3778,
3779,
3780,
3781,
3782,
3783,
3784,
3785,
3786,
3787,
3788,
3789,
3790,
3791,
3793,
3794,
3795,
3796,
3797,
3798,
3799,
3800,
3801,
3802,
3812,
3814,
3815,
3816,
3819,
3821,
3822,
3823,
3824,
3825,
3826,
3827,
3828,
3829,
3832,
3833,
3834,
3835,
3836,
3837,
3838,
3839,
3840,
3841,
3842,
3843,
3844,
3845,
3846,
3847,
3848,
3849,
3850,
3851,
3852,
3854,
3855,
3857,
3873,
3874,
3875,
3876,
3877,
3878,
3879,
3880,
3881,
3882,
3883,
3884,
3885,
3886,
3887,
3888,
3889,
3890,
3891,
3892,
3893,
3900,
3901,
3902,
3903,
3906,
3907,
3908,
3909,
3910,
3911,
3912,
3920,
3942,
3943,
3944,
3945,
3946,
3947,
3948,
3949,
3950,
3968,
3969,
3970,
3973,
3974,
3975,
3976,
3978,
3979,
3985,
3986,
3987,
3988,
3989,
3991,
3992,
3993,
3994,
3995,
3996,
3997,
4000,
4001,
4002,
4003,
4004,
4005,
4006,
4007,
4008,
4009,
4010,
4011,
4012,
4013,
4014,
4015,
4016,
4017,
4018,
4019,
4020,
4021,
4022,
4023,
4024,
4025,
4026,
4027,
4028,
4029,
4030,
4031,
4032,
4033,
4034,
4035,
4036,
4037,
4038,
4039,
4040,
4041,
4042,
4043,
4044,
4045,
4046,
4047,
4048,
4049,
4050,
4051,
4052,
4053,
4054,
4055,
4056,
4057,
4058,
4059,
4060,
4061,
4062,
4063,
4071,
4073,
4074,
4075,
4079,
4080,
4081,
4082,
4083,
4087,
4088,
4093,
4094,
4095,
4096,
4097,
4098,
4099,
4100,
4120,
4121,
4122,
4123,
4124,
4125,
4126,
4127,
4128,
4129,
4130,
4131,
4132,
4133,
4134,
4135,
4136,
4137,
4138,
4139,
4140,
4141,
4142,
4143,
4144,
4145,
4146,
4147,
4148,
4149,
4150,
4151,
4152,
4153,
4154,
4155,
4156,
4157,
4158,
4159,
4160,
4161,
4162,
4163,
4164,
4165,
4166,
4167,
4168,
4169,
4170,
4171,
4172,
4173,
4174,
4175,
4176,
4178,
4179,
4180,
4181,
4182,
4183,
4184,
4185,
4188,
4189,
4190,
4191,
4192,
4193,
4194,
4195,
4196,
4197,
4198,
4199,
4200,
4201,
4202,
4203,
4204,
4205,
4206,
4207,
4208,
4209,
4210,
4211,
4212,
4213,
4214,
4215,
4216,
4217,
4218,
4219,
4220,
4221,
4222,
4223,
4224,
4225,
4226,
4227,
4228,
4229,
4230,
4231,
4232,
4233,
4234,
4235,
4236,
4237,
4238,
4239,
4240,
4241,
4242,
4243,
4244,
4245,
4246,
4247,
4248,
4249,
4250,
4251,
4252,
4253,
4254,
4255,
4256,
4257,
4258,
4259,
4260,
4261,
4262,
4263,
4264,
4265,
4266,
4267,
4268,
4269,
4270,
4271,
4272,
4273,
4274,
4275,
4276,
4277,
4278,
4279,
4280,
4281,
4282,
4283,
4284,
4285,
4286,
4287,
4288,
4289,
4291,
4292,
4293,
4294,
4295,
4296,
4297,
4298,
4299,
4300,
4301,
4302,
4303,
4304,
4306,
4307,
4308,
4309,
4310,
4311,
4312,
4313,
4314,
4315,
4316,
4317,
4318,
4319,
4322,
4324,
4326,
4327,
4328,
4329,
4330,
4331,
4332,
4333,
4334,
4335,
4336,
4337,
4338,
4339,
4340,
4341,
4342,
4343,
4344,
4345,
4346,
4347,
4348,
4349,
4350,
4351,
4352,
4353,
4354,
4355,
4356,
4357,
4358,
4359,
4360,
4361,
4362,
4363,
4364,
4365,
4366,
4367,
4368,
4369,
4370,
4371,
4372,
4373,
4374,
4375,
4376,
4377,
4378,
4379,
4380,
4381,
4382,
4383,
4384,
4385,
4386,
4387,
4388,
4389,
4390,
4391,
4392,
4393,
4394,
4395,
4396,
4397,
4398,
4399,
4400,
4401,
4402,
4403,
4404,
4405,
4406,
4407,
4408,
4409,
4410,
4411,
4412,
4413,
4414,
4415,
4417,
4418,
4419,
4420,
4421,
4422,
4423,
4424,
4425,
4426,
4427,
4428,
4429,
4430,
4431,
4432,
4433,
4434,
4437,
4438,
4439,
4440,
4455,
4456,
4457,
4458,
4462,
4463,
4465,
4466,
4467,
4468,
4469,
4470,
4471,
4472,
4473,
4474,
4475,
4479,
4480,
4481,
4482,
4483,
4484,
4485,
4486,
4487,
4488,
4489,
4490,
4491,
4492,
4493,
4494,
4495,
4496,
4497,
4498,
4499,
4500,
4501,
4502,
4503,
4504,
4505,
4506,
4507,
4508,
4509,
4510,
4511,
4512,
4513,
4514,
4515,
4516,
4517,
4518,
4519,
4520,
4521,
4522,
4523,
4524,
4525,
4526,
4527,
4528,
4529,
4530,
4531,
4532,
4533,
4534,
4535,
4536,
4537,
4538,
4539,
4540,
4541,
4542,
4543,
4544,
4545,
4546,
4547,
4548,
4549,
4550,
4551,
4552,
4553,
4554,
4555,
4556,
4557,
4558,
4559,
4568,
4569,
4570,
4571,
4572,
4573,
4574,
4575,
4576,
4577,
4578,
4579,
4580,
4581,
4582,
4583,
4584,
4585,
4586,
4587,
4588,
4589,
4600,
4601,
4602,
4603,
4604,
4605,
4606,
4607,
4608,
4609,
4610,
4611,
4612,
4613,
4614,
4615,
4616,
4617,
4618,
4619,
4620,
4621,
4622,
4623,
4624,
4625,
4626,
4627,
4628,
4629,
4630,
4631,
4632,
4633,
4634,
4635,
4636,
4637,
4638,
4639,
4640,
4641,
4642,
4643,
4644,
4645,
4646,
4647,
4652,
4653,
4654,
4655,
4656,
4657,
4658,
4659,
4660,
4661,
4662,
4663,
4664,
4665,
4666,
4667,
4668,
4669,
4670,
4671,
4672,
4673,
4674,
4675,
4676,
4677,
4678,
4679,
4680,
4681,
4682,
4683,
4684,
4685,
4686,
4687,
4688,
4689,
4690,
4691,
4692,
4693,
4694,
4695,
4696,
4697,
4698,
4699,
4700,
4701,
4702,
4703,
4704,
4705,
4706,
4707,
4708,
4709,
4710,
4711,
4712,
4713,
4714,
4715,
4716,
4717,
4718,
4719,
4720,
4721,
4722,
4723,
4724,
4725,
4726,
4727,
4728,
4729,
4730,
4731,
4732,
4733,
4734,
4735,
4736,
4737,
4738,
4739,
4740,
4741,
4742,
4743,
4744,
4745,
4746,
4747,
4748,
4749,
4750,
4751,
4752,
4753,
4754,
4755,
4756,
4757,
4758,
4759,
4760,
4761,
4762,
4763,
4764,
4765,
4766,
4767,
4768,
4769,
4770,
4771,
4772,
4773,
4774,
4775,
4776,
4777,
4778,
4779,
4780,
4781,
4782,
4783,
4784,
4785,
4786,
4787,
4788,
4789,
4790,
4791,
4792,
4793,
4794,
4795,
4796,
4797,
4798,
4799,
4800,
4801,
4802,
4803,
4804,
4805,
4806,
4807,
4808,
4809,
4810,
4811,
4812,
4813,
4814,
4815,
4816,
4817,
4818,
4819,
4820,
4821,
4822,
4823,
4824,
4826,
4839,
4855,
4856,
4857,
4858,
4859,
4860,
4861,
4862,
4863,
4864,
4865,
4866,
4867,
4868,
4869,
4870,
4871,
4872,
4873,
4874,
4875,
4876,
4877,
4878,
4879,
4880,
4882,
4883,
4884,
4885,
4886,
4887,
4888,
4889,
4890,
4891,
4892,
4893,
4894,
4895,
4896,
4897,
4898,
4899,
4900,
4901,
4902,
4903,
4904,
4906,
4907,
4908,
4909,
4910,
4911,
4912,
4913,
4914,
4915,
4916,
4917,
4918,
4919,
4920,
4921,
4922,
4923,
4924,
4925,
4926,
4927,
4928,
4929,
4930,
4931,
4932,
4933,
4934,
4935,
4936,
4937,
4938,
4939,
4940,
4941,
4942,
4943,
4944,
4945,
4946,
4947,
4948,
4949,
4950,
4951,
4952,
4953,
4954,
4955,
4956,
4957,
4958,
4959,
4960,
4961,
4962,
4963,
4964,
4965,
4966,
4967,
4968,
4969,
4970,
4971,
4972,
4973,
4974,
4975,
4976,
4977,
4978,
4979,
4980,
4981,
4982,
4983,
4984,
4985,
4986,
4987,
4988,
4989,
4990,
4991,
4992,
4993,
4994,
4995,
4996,
4997,
4998,
4999,
5011,
5012,
5013,
5014,
5015,
5016,
5017,
5018,
5041,
5042,
5048,
5069,
5070,
5071,
5072,
5105,
5106,
5107,
5108,
5109,
5110,
5111,
5112,
5113,
5114,
5115,
5116,
5117,
5118,
5119,
5120,
5121,
5122,
5123,
5124,
5125,
5126,
5127,
5128,
5129,
5130,
5132,
5167,
5168,
5169,
5170,
5171,
5172,
5173,
5174,
5175,
5176,
5177,
5178,
5179,
5180,
5181,
5182,
5183,
5184,
5185,
5186,
5187,
5188,
5193,
5195,
5214,
5221,
5223,
5224,
5225,
5228,
5229,
5233,
5234,
5235,
5237,
5243,
5244,
5245,
5246,
5247,
5250,
5251,
5252,
5253,
5254,
5255,
5256,
5257,
5258,
5259,
5262,
5263,
5264,
5266,
5269,
5270,
5271,
5272,
5273,
5274,
5275,
5292,
5293,
5294,
5295,
5296,
5297,
5298,
5299,
5300,
5301,
5302,
5303,
5304,
5305,
5306,
5307,
5308,
5309,
5310,
5311,
5316,
5317,
5318,
5320,
5321,
5322,
5323,
5324,
5325,
5329,
5330,
5331,
5332,
5336,
5337,
5340,
5341,
5342,
5343,
5344,
5345,
5346,
5347,
5348,
5349,
5352,
5353,
5354,
5355,
5356,
5357,
5358,
5359,
5360,
5361,
5362,
5363,
5364,
5365,
5367,
5368,
5369,
5370,
5371,
5372,
5373,
5379,
5380,
5381,
5382,
5383,
5387,
5388,
5389,
5391,
5392,
5393,
5396,
5451,
5456,
5457,
5458,
5459,
5460,
5461,
5462,
5463,
5464,
5466,
5467,
5469,
5472,
5479,
5480,
5481,
5482,
5487,
5488,
5489,
5490,
5498,
5499,
5500,
5513,
5514,
5515,
5516,
5518,
5519,
5520,
5523,
5524,
5527,
5530,
5531,
5532,
5533,
5534,
5535,
5536,
5537,
5538,
5539,
5544,
5545,
5546,
5550,
5551,
5552,
5554,
5555,
5556,
5558,
5559,
5560,
5561,
5562,
5563,
5564,
5565,
5566,
5567,
5568,
5569,
5570,
5571,
5572,
5573,
5574,
5575,
5576,
5577,
5578,
5579,
5580,
5581,
5582,
5583,
5588,
5589,
5591,
5592,
5593,
5596,
5597,
5598,
5600,
5601,
5602,
5603,
5604,
5605,
5606,
5607,
5608,
5609,
5610,
5611,
5612,
5613,
5614,
5615,
5616,
5617,
5618,
5619,
5620,
5621,
5623,
5624,
5625,
5627,
5628,
5629,
5631,
5632,
5633,
5634,
5635,
5636,
5637,
5638,
5639,
5641,
5643,
5644,
5646,
5649,
5650,
5651,
5652,
5653,
5654,
5655,
5659,
5663,
5664,
5665,
5666,
5667,
5668,
5669,
5670,
5671,
5672,
5673,
5674,
5675,
5676,
5677,
5678,
5679,
5680,
5681,
5682,
5683,
5684,
5685,
5698,
5699,
5700,
5701,
5702,
5703,
5704,
5705,
5706,
5707,
5708,
5709,
5710,
5711,
5712,
5713,
5714,
5715,
5716,
5717,
5718,
5719,
5720,
5721,
5722,
5723,
5724,
5725,
5726,
5727,
5728,
5729,
5730,
5731,
5732,
5733,
5734,
5735,
5736,
5737,
5738,
5739,
5740,
5741,
5742,
5743,
5744,
5745,
5746,
5747,
5748,
5749,
5750,
5751,
5752,
5753,
5754,
5755,
5756,
5757,
5758,
5759,
5760,
5761,
5762,
5763,
5764,
5765,
5766,
5767,
5768,
5769,
5770,
5771,
5772,
5773,
5774,
5775,
5776,
5777,
5778,
5779,
5780,
5781,
5782,
5783,
5784,
5785,
5786,
5787,
5788,
5789,
5790,
5791,
5792,
5793,
5794,
5795,
5796,
5797,
5798,
5799,
5800,
5801,
5802,
5803,
5804,
5805,
5806,
5807,
5808,
5809,
5810,
5811,
5812,
5813,
5814,
5815,
5816,
5817,
5818,
5819,
5820,
5821,
5825,
5828,
5829,
5830,
5831,
5832,
5833,
5834,
5835,
5836,
5837,
5839,
5842,
5843,
5844,
5845,
5846,
5847,
5848,
5849,
5850,
5851,
5852,
5853,
5854,
5855,
5856,
5857,
5858,
5859,
5861,
5862,
5863,
5864,
5865,
5866,
5867,
5868,
5869,
5870,
5871,
5872,
5873,
5874,
5875,
5876,
5877,
5879,
5880,
5884,
5885,
5886,
5887,
5890,
5921,
5922,
5923,
5924,
5925,
5926,
5927,
5928,
5929,
5930,
5931,
5932,
5933,
5934,
5935,
5936,
5937,
5938,
5939,
5940,
5941,
5942,
5945,
5946,
5947,
5948,
5949,
5950,
5951,
5952,
5953,
5954,
5955,
5956,
5957,
5958,
5959,
5960,
5961,
5962,
5963,
5964,
5965,
5966,
5967,
5968,
5969,
5970,
5971,
5972,
5973,
5974,
5975,
5976,
6050,
6051,
6052,
6053,
6054,
6055,
6056,
6057,
6058,
6059,
6060,
6061,
6062,
6063,
6064,
6065,
6066,
6067,
6068,
6069,
6070,
6071,
6072,
6073,
6074,
6075,
6076,
6077,
6078,
6079,
6080,
6081,
6082,
6083,
6084,
6085,
6086,
6087,
6088,
6089,
6090,
6091,
6092,
6093,
6094,
6095,
6096,
6097,
6098,
6099,
6100,
6101,
6102,
6103,
6104,
6105,
6106,
6107,
6108,
6109,
6110,
6111,
6112,
6113,
6114,
6115,
6116,
6117,
6118,
6119,
6120,
6121,
6122,
6123,
6124,
6125,
6128,
6129,
6130,
6131,
6132,
6133,
6134,
6135,
6141,
6144,
6145,
6146,
6147,
6148,
6149,
6150,
6151,
6152,
6153,
6154,
6155,
6156,
6157,
6158,
6159,
6160,
6161,
6162,
6163,
6164,
6165,
6166,
6167,
6168,
6169,
6170,
6171,
6172,
6173,
6174,
6175,
6176,
6178,
6179,
6180,
6181,
6182,
6183,
6184,
6185,
6186,
6187,
6190,
6200,
6201,
6202,
6204,
6207,
6210,
6211,
6244,
6245,
6246,
6247,
6248,
6249,
6250,
6251,
6252,
6253,
6254,
6255,
6256,
6257,
6258,
6259,
6260,
6261,
6262,
6263,
6264,
6265,
6266,
6267,
6268,
6269,
6270,
6271,
6272,
6273,
6274,
6275,
6307,
6316,
6317,
6318,
6319,
6320,
6321,
6322,
6323,
6324,
6325,
6328,
6329,
6330,
6331,
6332,
6333,
6334,
6335,
6336,
6337,
6338,
6339,
6340,
6341,
6342,
6343,
6344,
6345,
6346,
6347,
6348,
6349,
6350,
6351,
6352,
6353,
6354,
6355,
6356,
6357,
6358,
6359,
6360,
6362,
6363,
6364,
6365,
6366,
6367,
6368,
6369,
6370,
6371,
6372,
6381,
6382,
6383,
6384,
6385,
6386,
6387,
6391,
6393,
6394,
6395,
6396,
6397,
6398,
6399,
6400,
6401,
6402,
6403,
6404,
6405,
6406,
6407,
6408,
6409,
6410,
6411,
6412,
6413,
6414,
6415,
6416,
6417,
6418,
6419,
6420,
6421,
6422,
6423,
6424,
6425,
6426,
6427,
6428,
6429,
6430,
6431,
6432,
6433,
6434,
6435,
6436,
6437,
6438,
6439,
6440,
6441,
6442,
6443,
6444,
6445,
6446,
6447,
6448,
6449,
6450,
6451,
6452,
6453,
6454,
6455,
6456,
6457,
6458,
6459,
6460,
6461,
6462,
6463,
6464,
6465,
6466,
6467,
6468,
6469,
6470,
6471,
6472,
6473,
6474,
6475,
6476,
6477,
6478,
6479,
6480,
6481,
6482,
6483,
6484,
6485,
6486,
6487,
6488,
6489,
6490,
6491,
6492,
6493,
6494,
6495,
6496,
6497,
6498,
6499,
6500,
6501,
6502,
6503,
6504,
6505,
6506,
6507,
6508,
6509,
6510,
6511,
6512,
6513,
6514,
6515,
6516,
6517,
6518,
6519,
6520,
6521,
6522,
6523,
6524,
6525,
6526,
6527,
6528,
6529,
6530,
6531,
6532,
6533,
6534,
6535,
6536,
6537,
6538,
6539,
6540,
6541,
6542,
6543,
6544,
6545,
6546,
6547,
6548,
6549,
6550,
6551,
6552,
6553,
6554,
6555,
6556,
6557,
6558,
6559,
6560,
6561,
6562,
6563,
6564,
6565,
6566,
6567,
6568,
6569,
6570,
6571,
6572,
6573,
6574,
6575,
6576,
6577,
6578,
6579,
6580,
6581,
6582,
6583,
6584,
6585,
6586,
6587,
6588,
6589,
6590,
6591,
6592,
6593,
6594,
6595,
6596,
6597,
6598,
6599,
6600,
6601,
6602,
6603,
6604,
6605,
6606,
6607,
6608,
6609,
6610,
6611,
6612,
6613,
6614,
6615,
6616,
6617,
6618,
6619,
6620,
6621,
6622,
6623,
6624,
6625,
6626,
6627,
6628,
6629,
6630,
6631,
6632,
6633,
6634,
6635,
6636,
6637,
6638,
6639,
6640,
6641,
6642,
6643,
6644,
6646,
6647,
6649,
6650,
6651,
6652,
6653,
6654,
6655,
6656,
6657,
6658,
6659,
6660,
6661,
6662,
6663,
6664,
6665,
6666,
6667,
6668,
6669,
6670,
6671,
6672,
6673,
6674,
6675,
6676,
6677,
6678,
6679,
6680,
6681,
6682,
6683,
6684,
6685,
6686,
6687,
6688,
6689,
6690,
6691,
6692,
6693,
6694,
6695,
6696,
6697,
6700,
6703,
6704,
6705,
6706,
6707,
6708,
6709,
6715,
6720,
6721,
6722,
6723,
6732,
6733,
6734,
6735,
6736,
6737,
6738,
6781,
6782,
6783,
6784,
6785,
6786,
6787,
6788,
6789,
6790,
6791,
6792,
6793,
6794,
6795,
6796,
6797,
6798,
6799,
6800,
6801,
6802,
6803,
6804,
6805,
6806,
6807,
6808,
6809,
6810,
6811,
6812,
6813,
6814,
6815,
6816,
6817,
6818,
6819,
6820,
6821,
6822,
6823,
6824,
6825,
6826,
6827,
6828,
6829,
6830,
6831,
6832,
6833,
6834,
6835,
6836,
6837,
6838,
6839,
6840,
6841,
6842,
6843,
6844,
6845,
6846,
6847,
6848,
6849,
6850,
6851,
6852,
6853,
6854,
6855,
6856,
6857,
6858,
6859,
6860,
6861,
6862,
6863,
6867,
6868,
6870,
6871,
6875,
6876,
6879,
6880,
6881,
6882,
6883,
6884,
6885,
6886,
6887,
6892,
6893,
6894,
6915,
6916,
6917,
6922,
6923,
6924,
6925,
6927,
6931,
6932,
6933,
6934,
6956,
6957,
6958,
6959,
6962,
6966,
6978,
6979,
6980,
6981,
6982,
6983,
6984,
6985,
6986,
6987,
6988,
6989,
6990,
6991,
6996,
6997,
7005,
7006,
7007,
7400,
7401,
7402,
7403,
7404,
7405,
7406,
7407,
7408,
7409,
7410,
7411,
7412,
7413,
7414,
7415,
7416,
7417,
7418,
7419,
7420,
7421,
7422,
7423,
20004,
20005,
20006,
20007,
20008,
20009,
20010,
20011,
20012,
20013,
20014,
20015,
20016,
20017,
20018,
20019,
20020,
20021,
20022,
20023,
20024,
20025,
20026,
20027,
20028,
20029,
20030,
20031,
20032,
20064,
20065,
20066,
20067,
20068,
20069,
20070,
20071,
20072,
20073,
20074,
20075,
20076,
20077,
20078,
20079,
20080,
20081,
20082,
20083,
20084,
20085,
20086,
20087,
20088,
20089,
20090,
20091,
20092,
20135,
20136,
20137,
20138,
20248,
20249,
20250,
20251,
20252,
20253,
20254,
20255,
20256,
20257,
20258,
20348,
20349,
20350,
20351,
20352,
20353,
20354,
20355,
20356,
20357,
20358,
20436,
20437,
20438,
20439,
20440,
20499,
20538,
20539,
20790,
20791,
20822,
20823,
20824,
20934,
20935,
20936,
21035,
21036,
21037,
21095,
21096,
21097,
21100,
21148,
21149,
21150,
21291,
21292,
21413,
21414,
21415,
21416,
21417,
21418,
21419,
21420,
21421,
21422,
21423,
21453,
21454,
21455,
21456,
21457,
21458,
21459,
21460,
21461,
21462,
21463,
21473,
21474,
21475,
21476,
21477,
21478,
21479,
21480,
21481,
21482,
21483,
21500,
21780,
21781,
21782,
21817,
21818,
21891,
21892,
21893,
21894,
21896,
21897,
21898,
21899,
22032,
22033,
22091,
22092,
22171,
22172,
22173,
22174,
22175,
22176,
22177,
22181,
22182,
22183,
22184,
22185,
22186,
22187,
22191,
22192,
22193,
22194,
22195,
22196,
22197,
22234,
22235,
22236,
22275,
22277,
22279,
22281,
22283,
22285,
22287,
22289,
22291,
22293,
22300,
22332,
22391,
22392,
22521,
22522,
22523,
22524,
22525,
22700,
22770,
22780,
22832,
22991,
22992,
22993,
22994,
23028,
23029,
23030,
23031,
23032,
23033,
23034,
23035,
23036,
23037,
23038,
23090,
23095,
23239,
23240,
23433,
23700,
23830,
23831,
23832,
23833,
23834,
23835,
23836,
23837,
23838,
23839,
23840,
23841,
23842,
23843,
23844,
23845,
23846,
23847,
23848,
23849,
23850,
23851,
23852,
23853,
23866,
23867,
23868,
23869,
23870,
23871,
23872,
23877,
23878,
23879,
23880,
23881,
23882,
23883,
23884,
23886,
23887,
23888,
23889,
23890,
23891,
23892,
23893,
23894,
23946,
23947,
23948,
24047,
24048,
24100,
24200,
24305,
24306,
24311,
24312,
24313,
24342,
24343,
24344,
24345,
24346,
24347,
24370,
24371,
24372,
24373,
24374,
24375,
24376,
24377,
24378,
24379,
24380,
24381,
24382,
24383,
24500,
24547,
24548,
24571,
24600,
24718,
24719,
24720,
24817,
24818,
24819,
24820,
24821,
24877,
24878,
24879,
24880,
24881,
24882,
24891,
24892,
24893,
25000,
25231,
25391,
25392,
25393,
25394,
25395,
25700,
25828,
25829,
25830,
25831,
25832,
25833,
25834,
25835,
25836,
25837,
25838,
25884,
25932,
26191,
26192,
26193,
26194,
26195,
26237,
26331,
26332,
26391,
26392,
26393,
26432,
26591,
26592,
26632,
26692,
26701,
26702,
26703,
26704,
26705,
26706,
26707,
26708,
26709,
26710,
26711,
26712,
26713,
26714,
26715,
26716,
26717,
26718,
26719,
26720,
26721,
26722,
26729,
26730,
26731,
26732,
26733,
26734,
26735,
26736,
26737,
26738,
26739,
26740,
26741,
26742,
26743,
26744,
26745,
26746,
26747,
26748,
26749,
26750,
26751,
26752,
26753,
26754,
26755,
26756,
26757,
26758,
26759,
26760,
26766,
26767,
26768,
26769,
26770,
26771,
26772,
26773,
26774,
26775,
26776,
26777,
26778,
26779,
26780,
26781,
26782,
26783,
26784,
26785,
26786,
26787,
26791,
26792,
26793,
26794,
26795,
26796,
26797,
26798,
26799,
26801,
26802,
26803,
26811,
26812,
26813,
26814,
26815,
26819,
26820,
26821,
26822,
26823,
26824,
26825,
26826,
26830,
26831,
26832,
26833,
26834,
26835,
26836,
26837,
26841,
26842,
26843,
26844,
26845,
26846,
26847,
26848,
26849,
26850,
26851,
26852,
26853,
26854,
26855,
26856,
26857,
26858,
26859,
26860,
26861,
26862,
26863,
26864,
26865,
26866,
26867,
26868,
26869,
26870,
26891,
26892,
26893,
26894,
26895,
26896,
26897,
26898,
26899,
26901,
26902,
26903,
26904,
26905,
26906,
26907,
26908,
26909,
26910,
26911,
26912,
26913,
26914,
26915,
26916,
26917,
26918,
26919,
26920,
26921,
26922,
26923,
26929,
26930,
26931,
26932,
26933,
26934,
26935,
26936,
26937,
26938,
26939,
26940,
26941,
26942,
26943,
26944,
26945,
26946,
26948,
26949,
26950,
26951,
26952,
26953,
26954,
26955,
26956,
26957,
26958,
26959,
26960,
26961,
26962,
26963,
26964,
26965,
26966,
26967,
26968,
26969,
26970,
26971,
26972,
26973,
26974,
26975,
26976,
26977,
26978,
26979,
26980,
26981,
26982,
26983,
26984,
26985,
26986,
26987,
26988,
26989,
26990,
26991,
26992,
26993,
26994,
26995,
26996,
26997,
26998,
27037,
27038,
27039,
27040,
27120,
27200,
27205,
27206,
27207,
27208,
27209,
27210,
27211,
27212,
27213,
27214,
27215,
27216,
27217,
27218,
27219,
27220,
27221,
27222,
27223,
27224,
27225,
27226,
27227,
27228,
27229,
27230,
27231,
27232,
27258,
27259,
27260,
27291,
27292,
27391,
27392,
27393,
27394,
27395,
27396,
27397,
27398,
27429,
27492,
27493,
27500,
27561,
27562,
27563,
27564,
27571,
27572,
27573,
27574,
27581,
27582,
27583,
27584,
27591,
27592,
27593,
27594,
27700,
28191,
28192,
28193,
28232,
28348,
28349,
28350,
28351,
28352,
28353,
28354,
28355,
28356,
28357,
28358,
28402,
28403,
28404,
28405,
28406,
28407,
28408,
28409,
28410,
28411,
28412,
28413,
28414,
28415,
28416,
28417,
28418,
28419,
28420,
28421,
28422,
28423,
28424,
28425,
28426,
28427,
28428,
28429,
28430,
28431,
28432,
28462,
28463,
28464,
28465,
28466,
28467,
28468,
28469,
28470,
28471,
28472,
28473,
28474,
28475,
28476,
28477,
28478,
28479,
28480,
28481,
28482,
28483,
28484,
28485,
28486,
28487,
28488,
28489,
28490,
28491,
28492,
28600,
28991,
28992,
29100,
29101,
29118,
29119,
29120,
29121,
29122,
29168,
29169,
29170,
29171,
29172,
29177,
29178,
29179,
29180,
29181,
29182,
29183,
29184,
29185,
29187,
29188,
29189,
29190,
29191,
29192,
29193,
29194,
29195,
29220,
29221,
29333,
29371,
29373,
29375,
29377,
29379,
29381,
29383,
29385,
29635,
29636,
29700,
29701,
29702,
29738,
29739,
29849,
29850,
29871,
29872,
29873,
29900,
29901,
29902,
29903,
30161,
30162,
30163,
30164,
30165,
30166,
30167,
30168,
30169,
30170,
30171,
30172,
30173,
30174,
30175,
30176,
30177,
30178,
30179,
30200,
30339,
30340,
30491,
30492,
30493,
30494,
30729,
30730,
30731,
30732,
30791,
30792,
30800,
31028,
31121,
31154,
31170,
31171,
31251,
31252,
31253,
31254,
31255,
31256,
31257,
31258,
31259,
31265,
31266,
31267,
31268,
31275,
31276,
31277,
31278,
31279,
31281,
31282,
31283,
31284,
31285,
31286,
31287,
31288,
31289,
31290,
31291,
31292,
31293,
31294,
31295,
31296,
31297,
31300,
31370,
31461,
31462,
31463,
31464,
31465,
31466,
31467,
31468,
31469,
31528,
31529,
31600,
31700,
31838,
31839,
31900,
31901,
31965,
31966,
31967,
31968,
31969,
31970,
31971,
31972,
31973,
31974,
31975,
31976,
31977,
31978,
31979,
31980,
31981,
31982,
31983,
31984,
31985,
31986,
31987,
31988,
31989,
31990,
31991,
31992,
31993,
31994,
31995,
31996,
31997,
31998,
31999,
32000,
32001,
32002,
32003,
32005,
32006,
32007,
32008,
32009,
32010,
32011,
32012,
32013,
32014,
32015,
32016,
32017,
32018,
32019,
32020,
32021,
32022,
32023,
32024,
32025,
32026,
32027,
32028,
32029,
32030,
32031,
32033,
32034,
32035,
32036,
32037,
32038,
32039,
32040,
32041,
32042,
32043,
32044,
32045,
32046,
32047,
32048,
32049,
32050,
32051,
32052,
32053,
32054,
32055,
32056,
32057,
32058,
32061,
32062,
32064,
32065,
32066,
32067,
32074,
32075,
32076,
32077,
32081,
32082,
32083,
32084,
32085,
32086,
32098,
32099,
32100,
32104,
32107,
32108,
32109,
32110,
32111,
32112,
32113,
32114,
32115,
32116,
32117,
32118,
32119,
32120,
32121,
32122,
32123,
32124,
32125,
32126,
32127,
32128,
32129,
32130,
32133,
32134,
32135,
32136,
32137,
32138,
32139,
32140,
32141,
32142,
32143,
32144,
32145,
32146,
32147,
32148,
32149,
32150,
32151,
32152,
32153,
32154,
32155,
32156,
32157,
32158,
32161,
32164,
32165,
32166,
32167,
32180,
32181,
32182,
32183,
32184,
32185,
32186,
32187,
32188,
32189,
32190,
32191,
32192,
32193,
32194,
32195,
32196,
32197,
32198,
32199,
32201,
32202,
32203,
32204,
32205,
32206,
32207,
32208,
32209,
32210,
32211,
32212,
32213,
32214,
32215,
32216,
32217,
32218,
32219,
32220,
32221,
32222,
32223,
32224,
32225,
32226,
32227,
32228,
32229,
32230,
32231,
32232,
32233,
32234,
32235,
32236,
32237,
32238,
32239,
32240,
32241,
32242,
32243,
32244,
32245,
32246,
32247,
32248,
32249,
32250,
32251,
32252,
32253,
32254,
32255,
32256,
32257,
32258,
32259,
32260,
32301,
32302,
32303,
32304,
32305,
32306,
32307,
32308,
32309,
32310,
32311,
32312,
32313,
32314,
32315,
32316,
32317,
32318,
32319,
32320,
32321,
32322,
32323,
32324,
32325,
32326,
32327,
32328,
32329,
32330,
32331,
32332,
32333,
32334,
32335,
32336,
32337,
32338,
32339,
32340,
32341,
32342,
32343,
32344,
32345,
32346,
32347,
32348,
32349,
32350,
32351,
32352,
32353,
32354,
32355,
32356,
32357,
32358,
32359,
32360,
32401,
32402,
32403,
32404,
32405,
32406,
32407,
32408,
32409,
32410,
32411,
32412,
32413,
32414,
32415,
32416,
32417,
32418,
32419,
32420,
32421,
32422,
32423,
32424,
32425,
32426,
32427,
32428,
32429,
32430,
32431,
32432,
32433,
32434,
32435,
32436,
32437,
32438,
32439,
32440,
32441,
32442,
32443,
32444,
32445,
32446,
32447,
32448,
32449,
32450,
32451,
32452,
32453,
32454,
32455,
32456,
32457,
32458,
32459,
32460,
32501,
32502,
32503,
32504,
32505,
32506,
32507,
32508,
32509,
32510,
32511,
32512,
32513,
32514,
32515,
32516,
32517,
32518,
32519,
32520,
32521,
32522,
32523,
32524,
32525,
32526,
32527,
32528,
32529,
32530,
32531,
32532,
32533,
32534,
32535,
32536,
32537,
32538,
32539,
32540,
32541,
32542,
32543,
32544,
32545,
32546,
32547,
32548,
32549,
32550,
32551,
32552,
32553,
32554,
32555,
32556,
32557,
32558,
32559,
32560,
32600,
32601,
32602,
32603,
32604,
32605,
32606,
32607,
32608,
32609,
32610,
32611,
32612,
32613,
32614,
32615,
32616,
32617,
32618,
32619,
32620,
32621,
32622,
32623,
32624,
32625,
32626,
32627,
32628,
32629,
32630,
32631,
32632,
32633,
32634,
32635,
32636,
32637,
32638,
32639,
32640,
32641,
32642,
32643,
32644,
32645,
32646,
32647,
32648,
32649,
32650,
32651,
32652,
32653,
32654,
32655,
32656,
32657,
32658,
32659,
32660,
32661,
32662,
32663,
32664,
32665,
32666,
32667,
32700,
32701,
32702,
32703,
32704,
32705,
32706,
32707,
32708,
32709,
32710,
32711,
32712,
32713,
32714,
32715,
32716,
32717,
32718,
32719,
32720,
32721,
32722,
32723,
32724,
32725,
32726,
32727,
32728,
32729,
32730,
32731,
32732,
32733,
32734,
32735,
32736,
32737,
32738,
32739,
32740,
32741,
32742,
32743,
32744,
32745,
32746,
32747,
32748,
32749,
32750,
32751,
32752,
32753,
32754,
32755,
32756,
32757,
32758,
32759,
32760,
32761,
32766,
0
};

static const char * epsg_descriptions[] =
{
"Anguilla 1957 / British West Indies Grid",
"Antigua 1943 / British West Indies Grid",
"Dominica 1945 / British West Indies Grid",
"Grenada 1953 / British West Indies Grid",
"Montserrat 1958 / British West Indies Grid",
"St. Kitts 1955 / British West Indies Grid",
"St. Lucia 1955 / British West Indies Grid",
"St. Vincent 45 / British West Indies Grid",
"NAD27(CGQ77) / SCoPQ zone 2",
"NAD27(CGQ77) / SCoPQ zone 3",
"NAD27(CGQ77) / SCoPQ zone 4",
"NAD27(CGQ77) / SCoPQ zone 5",
"NAD27(CGQ77) / SCoPQ zone 6",
"NAD27(CGQ77) / SCoPQ zone 7",
"NAD27(CGQ77) / SCoPQ zone 8",
"NAD27(CGQ77) / SCoPQ zone 9",
"NAD27(CGQ77) / SCoPQ zone 10",
"NAD27(76) / MTM zone 8",
"NAD27(76) / MTM zone 9",
"NAD27(76) / MTM zone 10",
"NAD27(76) / MTM zone 11",
"NAD27(76) / MTM zone 12",
"NAD27(76) / MTM zone 13",
"NAD27(76) / MTM zone 14",
"NAD27(76) / MTM zone 15",
"NAD27(76) / MTM zone 16",
"NAD27(76) / MTM zone 17",
"NAD27(76) / UTM zone 15N",
"NAD27(76) / UTM zone 16N",
"NAD27(76) / UTM zone 17N",
"NAD27(76) / UTM zone 18N",
"NAD27(CGQ77) / UTM zone 17N",
"NAD27(CGQ77) / UTM zone 18N",
"NAD27(CGQ77) / UTM zone 19N",
"NAD27(CGQ77) / UTM zone 20N",
"NAD27(CGQ77) / UTM zone 21N",
"NAD83(CSRS98) / New Brunswick Stereo",
"NAD83(CSRS98) / UTM zone 19N",
"NAD83(CSRS98) / UTM zone 20N",
"Israel 1993 / Israeli TM Grid",
"Locodjo 1965 / UTM zone 30N",
"Abidjan 1987 / UTM zone 30N",
"Locodjo 1965 / UTM zone 29N",
"Abidjan 1987 / UTM zone 29N",
"Hanoi 1972 / Gauss-Kruger zone 18",
"Hanoi 1972 / Gauss-Kruger zone 19",
"Hartebeesthoek94 / Lo15",
"Hartebeesthoek94 / Lo17",
"Hartebeesthoek94 / Lo19",
"Hartebeesthoek94 / Lo21",
"Hartebeesthoek94 / Lo23",
"Hartebeesthoek94 / Lo25",
"Hartebeesthoek94 / Lo27",
"Hartebeesthoek94 / Lo29",
"Hartebeesthoek94 / Lo31",
"Hartebeesthoek94 / Lo33",
"CH1903+ / LV95",
"Rassadiran / Nakhl e Taqi",
"ED50(ED77) / UTM zone 38N",
"ED50(ED77) / UTM zone 39N",
"ED50(ED77) / UTM zone 40N",
"ED50(ED77) / UTM zone 41N",
"Madrid 1870 (Madrid) / Spain",
"Dabola 1981 / UTM zone 28N",
"Dabola 1981 / UTM zone 29N",
"S-JTSK (Ferro) / Krovak",
"Mount Dillon / Tobago Grid",
"Naparima 1955 / UTM zone 20N",
"ELD79 / Libya zone 5",
"ELD79 / Libya zone 6",
"ELD79 / Libya zone 7",
"ELD79 / Libya zone 8",
"ELD79 / Libya zone 9",
"ELD79 / Libya zone 10",
"ELD79 / Libya zone 11",
"ELD79 / Libya zone 12",
"ELD79 / Libya zone 13",
"ELD79 / UTM zone 32N",
"ELD79 / UTM zone 33N",
"ELD79 / UTM zone 34N",
"ELD79 / UTM zone 35N",
"Chos Malal 1914 / Argentina 2",
"Pampa del Castillo / Argentina 2",
"Hito XVIII 1963 / Argentina 2",
"Hito XVIII 1963 / UTM zone 19S",
"NAD27 / Cuba Norte",
"NAD27 / Cuba Sur",
"ELD79 / TM 12 NE",
"Carthage / TM 11 NE",
"Yemen NGN96 / UTM zone 38N",
"Yemen NGN96 / UTM zone 39N",
"South Yemen / Gauss Kruger zone 8",
"South Yemen / Gauss Kruger zone 9",
"Hanoi 1972 / GK 106 NE",
"WGS 72BE / TM 106 NE",
"Bissau / UTM zone 28N",
"Korean 1985 / East Belt",
"Korean 1985 / Central Belt",
"Korean 1985 / West Belt",
"Qatar 1948 / Qatar Grid",
"GGRS87 / Greek Grid",
"Lake / Maracaibo Grid M1",
"Lake / Maracaibo Grid",
"Lake / Maracaibo Grid M3",
"Lake / Maracaibo La Rosa Grid",
"NZGD2000 / Mount Eden 2000",
"NZGD2000 / Bay of Plenty 2000",
"NZGD2000 / Poverty Bay 2000",
"NZGD2000 / Hawkes Bay 2000",
"NZGD2000 / Taranaki 2000",
"NZGD2000 / Tuhirangi 2000",
"NZGD2000 / Wanganui 2000",
"NZGD2000 / Wairarapa 2000",
"NZGD2000 / Wellington 2000",
"NZGD2000 / Collingwood 2000",
"NZGD2000 / Nelson 2000",
"NZGD2000 / Karamea 2000",
"NZGD2000 / Buller 2000",
"NZGD2000 / Grey 2000",
"NZGD2000 / Amuri 2000",
"NZGD2000 / Marlborough 2000",
"NZGD2000 / Hokitika 2000",
"NZGD2000 / Okarito 2000",
"NZGD2000 / Jacksons Bay 2000",
"NZGD2000 / Mount Pleasant 2000",
"NZGD2000 / Gawler 2000",
"NZGD2000 / Timaru 2000",
"NZGD2000 / Lindis Peak 2000",
"NZGD2000 / Mount Nicholas 2000",
"NZGD2000 / Mount York 2000",
"NZGD2000 / Observation Point 2000",
"NZGD2000 / North Taieri 2000",
"NZGD2000 / Bluff 2000",
"NZGD2000 / UTM zone 58S",
"NZGD2000 / UTM zone 59S",
"NZGD2000 / UTM zone 60S",
"Accra / Ghana National Grid",
"Accra / TM 1 NW",
"NAD27(CGQ77) / Quebec Lambert",
"NAD83(CSRS98) / SCoPQ zone 2",
"NAD83(CSRS98) / MTM zone 3",
"NAD83(CSRS98) / MTM zone 4",
"NAD83(CSRS98) / MTM zone 5",
"NAD83(CSRS98) / MTM zone 6",
"NAD83(CSRS98) / MTM zone 7",
"NAD83(CSRS98) / MTM zone 8",
"NAD83(CSRS98) / MTM zone 9",
"NAD83(CSRS98) / MTM zone 10",
"NAD83(CSRS98) / UTM zone 21N",
"NAD83(CSRS98) / UTM zone 18N",
"NAD83(CSRS98) / UTM zone 17N",
"NAD83(CSRS98) / UTM zone 13N",
"NAD83(CSRS98) / UTM zone 12N",
"NAD83(CSRS98) / UTM zone 11N",
"RGF93 / Lambert-93",
"American Samoa 1962 / American Samoa Lambert",
"NAD83(HARN) / UTM zone 59S",
"IRENET95 / Irish Transverse Mercator",
"IRENET95 / UTM zone 29N",
"Sierra Leone 1924 / New Colony Grid",
"Sierra Leone 1924 / New War Office Grid",
"Sierra Leone 1968 / UTM zone 28N",
"Sierra Leone 1968 / UTM zone 29N",
"US National Atlas Equal Area",
"Locodjo 1965 / TM 5 NW",
"Abidjan 1987 / TM 5 NW",
"Pulkovo 1942(83) / Gauss Kruger zone 3",
"Pulkovo 1942(83) / Gauss Kruger zone 4",
"Pulkovo 1942(83) / Gauss Kruger zone 5",
"Luxembourg 1930 / Gauss",
"MGI / Slovenia Grid",
"Pulkovo 1942(58) / Poland zone I",
"Pulkovo 1942(58) / Poland zone II",
"Pulkovo 1942(58) / Poland zone III",
"Pulkovo 1942(58) / Poland zone IV",
"Pulkovo 1942(58) / Poland zone V",
"ETRS89 / Poland CS2000 zone 5",
"ETRS89 / Poland CS2000 zone 6",
"ETRS89 / Poland CS2000 zone 7",
"ETRS89 / Poland CS2000 zone 8",
"ETRS89 / Poland CS92",
"Azores Occidental 1939 / UTM zone 25N",
"Azores Central 1948 / UTM zone 26N",
"Azores Oriental 1940 / UTM zone 26N",
"Madeira 1936 / UTM zone 28N",
"ED50 / France EuroLambert",
"NZGD2000 / New Zealand Transverse Mercator 2000",
"American Samoa 1962 / American Samoa Lambert",
"NAD83(HARN) / UTM zone 2S",
"ETRS89 / Kp2000 Jutland",
"ETRS89 / Kp2000 Zealand",
"ETRS89 / Kp2000 Bornholm",
"Albanian 1987 / Gauss Kruger zone 4",
"ATS77 / New Brunswick Stereographic (ATS77)",
"REGVEN / UTM zone 18N",
"REGVEN / UTM zone 19N",
"REGVEN / UTM zone 20N",
"NAD27 / Tennessee",
"NAD83 / Kentucky North",
"ED50 / 3-degree Gauss-Kruger zone 9",
"ED50 / 3-degree Gauss-Kruger zone 10",
"ED50 / 3-degree Gauss-Kruger zone 11",
"ED50 / 3-degree Gauss-Kruger zone 12",
"ED50 / 3-degree Gauss-Kruger zone 13",
"ED50 / 3-degree Gauss-Kruger zone 14",
"ED50 / 3-degree Gauss-Kruger zone 15",
"ETRS89 / TM 30 NE",
"Douala 1948 / AOF west",
"Manoca 1962 / UTM zone 32N",
"Qornoq 1927 / UTM zone 22N",
"Qornoq 1927 / UTM zone 23N",
"Scoresbysund 1952 / Greenland zone 5 east",
"ATS77 / UTM zone 19N",
"ATS77 / UTM zone 20N",
"Scoresbysund 1952 / Greenland zone 6 east",
"NAD83 / Arizona East (ft)",
"NAD83 / Arizona Central (ft)",
"NAD83 / Arizona West (ft)",
"NAD83 / California zone 1 (ftUS)",
"NAD83 / California zone 2 (ftUS)",
"NAD83 / California zone 3 (ftUS)",
"NAD83 / California zone 4 (ftUS)",
"NAD83 / California zone 5 (ftUS)",
"NAD83 / California zone 6 (ftUS)",
"NAD83 / Colorado North (ftUS)",
"NAD83 / Colorado Central (ftUS)",
"NAD83 / Colorado South (ftUS)",
"NAD83 / Connecticut (ftUS)",
"NAD83 / Delaware (ftUS)",
"NAD83 / Florida East (ftUS)",
"NAD83 / Florida West (ftUS)",
"NAD83 / Florida North (ftUS)",
"NAD83 / Georgia East (ftUS)",
"NAD83 / Georgia West (ftUS)",
"NAD83 / Idaho East (ftUS)",
"NAD83 / Idaho Central (ftUS)",
"NAD83 / Idaho West (ftUS)",
"NAD83 / Indiana East (ftUS)",
"NAD83 / Indiana West (ftUS)",
"NAD83 / Kentucky North (ftUS)",
"NAD83 / Kentucky South (ftUS)",
"NAD83 / Maryland (ftUS)",
"NAD83 / Massachusetts Mainland (ftUS)",
"NAD83 / Massachusetts Island (ftUS)",
"NAD83 / Michigan North (ft)",
"NAD83 / Michigan Central (ft)",
"NAD83 / Michigan South (ft)",
"NAD83 / Mississippi East (ftUS)",
"NAD83 / Mississippi West (ftUS)",
"NAD83 / Montana (ft)",
"NAD83 / New Mexico East (ftUS)",
"NAD83 / New Mexico Central (ftUS)",
"NAD83 / New Mexico West (ftUS)",
"NAD83 / New York East (ftUS)",
"NAD83 / New York Central (ftUS)",
"NAD83 / New York West (ftUS)",
"NAD83 / New York Long Island (ftUS)",
"NAD83 / North Carolina (ftUS)",
"NAD83 / North Dakota North (ft)",
"NAD83 / North Dakota South (ft)",
"NAD83 / Oklahoma North (ftUS)",
"NAD83 / Oklahoma South (ftUS)",
"NAD83 / Oregon North (ft)",
"NAD83 / Oregon South (ft)",
"NAD83 / Pennsylvania North (ftUS)",
"NAD83 / Pennsylvania South (ftUS)",
"NAD83 / South Carolina (ft)",
"NAD83 / Tennessee (ftUS)",
"NAD83 / Texas North (ftUS)",
"NAD83 / Texas North Central (ftUS)",
"NAD83 / Texas Central (ftUS)",
"NAD83 / Texas South Central (ftUS)",
"NAD83 / Texas South (ftUS)",
"NAD83 / Utah North (ft)",
"NAD83 / Utah Central (ft)",
"NAD83 / Utah South (ft)",
"NAD83 / Virginia North (ftUS)",
"NAD83 / Virginia South (ftUS)",
"NAD83 / Washington North (ftUS)",
"NAD83 / Washington South (ftUS)",
"NAD83 / Wisconsin North (ftUS)",
"NAD83 / Wisconsin Central (ftUS)",
"NAD83 / Wisconsin South (ftUS)",
"ATS77 / Prince Edward Isl. Stereographic (ATS77)",
"NAD83(CSRS98) / Prince Edward Isl. Stereographic (NAD83)",
"NAD83(CSRS98) / Prince Edward Isl. Stereographic (NAD83)",
"ATS77 / MTM Nova Scotia zone 4",
"ATS77 / MTM Nova Scotia zone 5",
"Ammassalik 1958 / Greenland zone 7 east",
"Qornoq 1927 / Greenland zone 1 east",
"Qornoq 1927 / Greenland zone 2 east",
"Qornoq 1927 / Greenland zone 2 west",
"Qornoq 1927 / Greenland zone 3 east",
"Qornoq 1927 / Greenland zone 3 west",
"Qornoq 1927 / Greenland zone 4 east",
"Qornoq 1927 / Greenland zone 4 west",
"Qornoq 1927 / Greenland zone 5 west",
"Qornoq 1927 / Greenland zone 6 west",
"Qornoq 1927 / Greenland zone 7 west",
"Qornoq 1927 / Greenland zone 8 east",
"Batavia / TM 109 SE",
"WGS 84 / TM 116 SE",
"WGS 84 / TM 132 SE",
"WGS 84 / TM 6 NE",
"Garoua / UTM zone 33N",
"Kousseri / UTM zone 33N",
"Trinidad 1903 / Trinidad Grid (ftCla)",
"Campo Inchauspe / UTM zone 19S",
"Campo Inchauspe / UTM zone 20S",
"PSAD56 / ICN Regional",
"Ain el Abd / Aramco Lambert",
"ED50 / TM27",
"ED50 / TM30",
"ED50 / TM33",
"ED50 / TM36",
"ED50 / TM39",
"ED50 / TM42",
"ED50 / TM45",
"Hong Kong 1980 Grid System",
"Xian 1980 / Gauss-Kruger zone 13",
"Xian 1980 / Gauss-Kruger zone 14",
"Xian 1980 / Gauss-Kruger zone 15",
"Xian 1980 / Gauss-Kruger zone 16",
"Xian 1980 / Gauss-Kruger zone 17",
"Xian 1980 / Gauss-Kruger zone 18",
"Xian 1980 / Gauss-Kruger zone 19",
"Xian 1980 / Gauss-Kruger zone 20",
"Xian 1980 / Gauss-Kruger zone 21",
"Xian 1980 / Gauss-Kruger zone 22",
"Xian 1980 / Gauss-Kruger zone 23",
"Xian 1980 / Gauss-Kruger CM 75E",
"Xian 1980 / Gauss-Kruger CM 81E",
"Xian 1980 / Gauss-Kruger CM 87E",
"Xian 1980 / Gauss-Kruger CM 93E",
"Xian 1980 / Gauss-Kruger CM 99E",
"Xian 1980 / Gauss-Kruger CM 105E",
"Xian 1980 / Gauss-Kruger CM 111E",
"Xian 1980 / Gauss-Kruger CM 117E",
"Xian 1980 / Gauss-Kruger CM 123E",
"Xian 1980 / Gauss-Kruger CM 129E",
"Xian 1980 / Gauss-Kruger CM 135E",
"Xian 1980 / 3-degree Gauss-Kruger zone 25",
"Xian 1980 / 3-degree Gauss-Kruger zone 26",
"Xian 1980 / 3-degree Gauss-Kruger zone 27",
"Xian 1980 / 3-degree Gauss-Kruger zone 28",
"Xian 1980 / 3-degree Gauss-Kruger zone 29",
"Xian 1980 / 3-degree Gauss-Kruger zone 30",
"Xian 1980 / 3-degree Gauss-Kruger zone 31",
"Xian 1980 / 3-degree Gauss-Kruger zone 32",
"Xian 1980 / 3-degree Gauss-Kruger zone 33",
"Xian 1980 / 3-degree Gauss-Kruger zone 34",
"Xian 1980 / 3-degree Gauss-Kruger zone 35",
"Xian 1980 / 3-degree Gauss-Kruger zone 36",
"Xian 1980 / 3-degree Gauss-Kruger zone 37",
"Xian 1980 / 3-degree Gauss-Kruger zone 38",
"Xian 1980 / 3-degree Gauss-Kruger zone 39",
"Xian 1980 / 3-degree Gauss-Kruger zone 40",
"Xian 1980 / 3-degree Gauss-Kruger zone 41",
"Xian 1980 / 3-degree Gauss-Kruger zone 42",
"Xian 1980 / 3-degree Gauss-Kruger zone 43",
"Xian 1980 / 3-degree Gauss-Kruger zone 44",
"Xian 1980 / 3-degree Gauss-Kruger zone 45",
"Xian 1980 / 3-degree Gauss-Kruger CM 75E",
"Xian 1980 / 3-degree Gauss-Kruger CM 78E",
"Xian 1980 / 3-degree Gauss-Kruger CM 81E",
"Xian 1980 / 3-degree Gauss-Kruger CM 84E",
"Xian 1980 / 3-degree Gauss-Kruger CM 87E",
"Xian 1980 / 3-degree Gauss-Kruger CM 90E",
"Xian 1980 / 3-degree Gauss-Kruger CM 93E",
"Xian 1980 / 3-degree Gauss-Kruger CM 96E",
"Xian 1980 / 3-degree Gauss-Kruger CM 99E",
"Xian 1980 / 3-degree Gauss-Kruger CM 102E",
"Xian 1980 / 3-degree Gauss-Kruger CM 105E",
"Xian 1980 / 3-degree Gauss-Kruger CM 108E",
"Xian 1980 / 3-degree Gauss-Kruger CM 111E",
"Xian 1980 / 3-degree Gauss-Kruger CM 114E",
"Xian 1980 / 3-degree Gauss-Kruger CM 117E",
"Xian 1980 / 3-degree Gauss-Kruger CM 120E",
"Xian 1980 / 3-degree Gauss-Kruger CM 123E",
"Xian 1980 / 3-degree Gauss-Kruger CM 126E",
"Xian 1980 / 3-degree Gauss-Kruger CM 129E",
"Xian 1980 / 3-degree Gauss-Kruger CM 132E",
"Xian 1980 / 3-degree Gauss-Kruger CM 135E",
"KKJ / Finland zone 1",
"KKJ / Finland zone 2",
"KKJ / Finland Uniform Coordinate System",
"KKJ / Finland zone 4",
"South Yemen / Gauss-Kruger zone 8",
"South Yemen / Gauss-Kruger zone 9",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 3",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 4",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 5",
"RT90 2.5 gon W",
"Beijing 1954 / 3-degree Gauss-Kruger zone 25",
"Beijing 1954 / 3-degree Gauss-Kruger zone 26",
"Beijing 1954 / 3-degree Gauss-Kruger zone 27",
"Beijing 1954 / 3-degree Gauss-Kruger zone 28",
"Beijing 1954 / 3-degree Gauss-Kruger zone 29",
"Beijing 1954 / 3-degree Gauss-Kruger zone 30",
"Beijing 1954 / 3-degree Gauss-Kruger zone 31",
"Beijing 1954 / 3-degree Gauss-Kruger zone 32",
"Beijing 1954 / 3-degree Gauss-Kruger zone 33",
"Beijing 1954 / 3-degree Gauss-Kruger zone 34",
"Beijing 1954 / 3-degree Gauss-Kruger zone 35",
"Beijing 1954 / 3-degree Gauss-Kruger zone 36",
"Beijing 1954 / 3-degree Gauss-Kruger zone 37",
"Beijing 1954 / 3-degree Gauss-Kruger zone 38",
"Beijing 1954 / 3-degree Gauss-Kruger zone 39",
"Beijing 1954 / 3-degree Gauss-Kruger zone 40",
"Beijing 1954 / 3-degree Gauss-Kruger zone 41",
"Beijing 1954 / 3-degree Gauss-Kruger zone 42",
"Beijing 1954 / 3-degree Gauss-Kruger zone 43",
"Beijing 1954 / 3-degree Gauss-Kruger zone 44",
"Beijing 1954 / 3-degree Gauss-Kruger zone 45",
"Beijing 1954 / 3-degree Gauss-Kruger CM 75E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 78E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 81E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 84E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 87E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 90E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 93E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 96E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 99E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 102E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 105E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 108E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 111E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 114E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 117E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 120E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 123E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 126E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 129E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 132E",
"Beijing 1954 / 3-degree Gauss-Kruger CM 135E",
"JGD2000 / Japan Plane Rectangular CS I",
"JGD2000 / Japan Plane Rectangular CS II",
"JGD2000 / Japan Plane Rectangular CS III",
"JGD2000 / Japan Plane Rectangular CS IV",
"JGD2000 / Japan Plane Rectangular CS V",
"JGD2000 / Japan Plane Rectangular CS VI",
"JGD2000 / Japan Plane Rectangular CS VII",
"JGD2000 / Japan Plane Rectangular CS VIII",
"JGD2000 / Japan Plane Rectangular CS IX",
"JGD2000 / Japan Plane Rectangular CS X",
"JGD2000 / Japan Plane Rectangular CS XI",
"JGD2000 / Japan Plane Rectangular CS XII",
"JGD2000 / Japan Plane Rectangular CS XIII",
"JGD2000 / Japan Plane Rectangular CS XIV",
"JGD2000 / Japan Plane Rectangular CS XV",
"JGD2000 / Japan Plane Rectangular CS XVI",
"JGD2000 / Japan Plane Rectangular CS XVII",
"JGD2000 / Japan Plane Rectangular CS XVIII",
"JGD2000 / Japan Plane Rectangular CS XIX",
"Albanian 1987 / Gauss-Kruger zone 4",
"Pulkovo 1995 / Gauss-Kruger CM 21E",
"Pulkovo 1995 / Gauss-Kruger CM 27E",
"Pulkovo 1995 / Gauss-Kruger CM 33E",
"Pulkovo 1995 / Gauss-Kruger CM 39E",
"Pulkovo 1995 / Gauss-Kruger CM 45E",
"Pulkovo 1995 / Gauss-Kruger CM 51E",
"Pulkovo 1995 / Gauss-Kruger CM 57E",
"Pulkovo 1995 / Gauss-Kruger CM 63E",
"Pulkovo 1995 / Gauss-Kruger CM 69E",
"Pulkovo 1995 / Gauss-Kruger CM 75E",
"Pulkovo 1995 / Gauss-Kruger CM 81E",
"Pulkovo 1995 / Gauss-Kruger CM 87E",
"Pulkovo 1995 / Gauss-Kruger CM 93E",
"Pulkovo 1995 / Gauss-Kruger CM 99E",
"Pulkovo 1995 / Gauss-Kruger CM 105E",
"Pulkovo 1995 / Gauss-Kruger CM 111E",
"Pulkovo 1995 / Gauss-Kruger CM 117E",
"Pulkovo 1995 / Gauss-Kruger CM 123E",
"Pulkovo 1995 / Gauss-Kruger CM 129E",
"Pulkovo 1995 / Gauss-Kruger CM 135E",
"Pulkovo 1995 / Gauss-Kruger CM 141E",
"Pulkovo 1995 / Gauss-Kruger CM 147E",
"Pulkovo 1995 / Gauss-Kruger CM 153E",
"Pulkovo 1995 / Gauss-Kruger CM 159E",
"Pulkovo 1995 / Gauss-Kruger CM 165E",
"Pulkovo 1995 / Gauss-Kruger CM 171E",
"Pulkovo 1995 / Gauss-Kruger CM 177E",
"Pulkovo 1995 / Gauss-Kruger CM 177W",
"Pulkovo 1995 / Gauss-Kruger CM 171W",
"Pulkovo 1942 / Gauss-Kruger CM 9E",
"Pulkovo 1942 / Gauss-Kruger CM 15E",
"Pulkovo 1942 / Gauss-Kruger CM 21E",
"Pulkovo 1942 / Gauss-Kruger CM 27E",
"Pulkovo 1942 / Gauss-Kruger CM 33E",
"Pulkovo 1942 / Gauss-Kruger CM 39E",
"Pulkovo 1942 / Gauss-Kruger CM 45E",
"Pulkovo 1942 / Gauss-Kruger CM 51E",
"Pulkovo 1942 / Gauss-Kruger CM 57E",
"Pulkovo 1942 / Gauss-Kruger CM 63E",
"Pulkovo 1942 / Gauss-Kruger CM 69E",
"Pulkovo 1942 / Gauss-Kruger CM 75E",
"Pulkovo 1942 / Gauss-Kruger CM 81E",
"Pulkovo 1942 / Gauss-Kruger CM 87E",
"Pulkovo 1942 / Gauss-Kruger CM 93E",
"Pulkovo 1942 / Gauss-Kruger CM 99E",
"Pulkovo 1942 / Gauss-Kruger CM 105E",
"Pulkovo 1942 / Gauss-Kruger CM 111E",
"Pulkovo 1942 / Gauss-Kruger CM 117E",
"Pulkovo 1942 / Gauss-Kruger CM 123E",
"Pulkovo 1942 / Gauss-Kruger CM 129E",
"Pulkovo 1942 / Gauss-Kruger CM 135E",
"Pulkovo 1942 / Gauss-Kruger CM 141E",
"Pulkovo 1942 / Gauss-Kruger CM 147E",
"Pulkovo 1942 / Gauss-Kruger CM 153E",
"Pulkovo 1942 / Gauss-Kruger CM 159E",
"Pulkovo 1942 / Gauss-Kruger CM 165E",
"Pulkovo 1942 / Gauss-Kruger CM 171E",
"Pulkovo 1942 / Gauss-Kruger CM 177E",
"Pulkovo 1942 / Gauss-Kruger CM 177W",
"Pulkovo 1942 / Gauss-Kruger CM 171W",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 7",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 8",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 9",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 10",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 11",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 12",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 13",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 14",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 15",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 16",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 17",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 18",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 19",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 20",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 21",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 22",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 23",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 24",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 25",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 26",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 27",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 28",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 29",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 30",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 31",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 32",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 33",
"Samboja / UTM zone 50S",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 34",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 35",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 36",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 37",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 38",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 39",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 40",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 41",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 42",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 43",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 44",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 45",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 46",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 47",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 48",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 49",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 50",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 51",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 52",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 53",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 54",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 55",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 56",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 57",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 58",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 59",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 60",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 61",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 62",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 63",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 64",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 21E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 24E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 27E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 30E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 33E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 36E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 39E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 42E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 45E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 48E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 51E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 54E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 57E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 60E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 63E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 66E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 69E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 72E",
"Lietuvos Koordinoei Sistema 1994",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 75E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 78E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 81E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 84E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 87E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 90E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 93E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 96E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 99E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 102E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 105E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 108E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 111E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 114E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 117E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 120E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 123E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 126E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 129E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 132E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 135E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 138E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 141E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 144E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 147E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 150E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 153E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 156E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 159E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 162E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 165E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 168E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 171E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 174E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 177E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 180E",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 177W",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 174W",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 171W",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 168W",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 7",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 8",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 9",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 10",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 11",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 12",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 13",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 14",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 15",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 16",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 17",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 18",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 19",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 20",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 21",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 22",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 23",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 24",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 25",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 26",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 27",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 28",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 29",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 30",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 31",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 32",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 33",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 34",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 35",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 36",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 37",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 38",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 39",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 40",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 41",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 42",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 43",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 44",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 45",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 46",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 47",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 48",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 49",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 50",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 51",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 52",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 53",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 54",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 55",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 56",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 57",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 58",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 59",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 60",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 61",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 62",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 63",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 64",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 21E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 24E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 27E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 30E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 33E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 36E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 39E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 42E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 45E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 48E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 51E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 54E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 57E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 60E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 63E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 66E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 69E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 72E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 75E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 78E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 81E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 84E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 87E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 90E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 93E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 96E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 99E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 102E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 105E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 108E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 111E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 114E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 117E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 120E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 123E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 126E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 129E",
"Tete / UTM zone 36S",
"Tete / UTM zone 37S",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 132E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 135E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 138E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 141E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 144E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 147E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 150E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 153E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 156E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 159E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 162E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 165E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 168E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 171E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 174E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 177E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 180E",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 177W",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 174W",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 171W",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 168W",
"NAD83(HARN) / Alabama East",
"NAD83(HARN) / Alabama West",
"NAD83(HARN) / Arizona East",
"NAD83(HARN) / Arizona Central",
"NAD83(HARN) / Arizona West",
"NAD83(HARN) / Arkansas North",
"NAD83(HARN) / Arkansas South",
"NAD83(HARN) / California zone 1",
"NAD83(HARN) / California zone 2",
"NAD83(HARN) / California zone 3",
"NAD83(HARN) / California zone 4",
"NAD83(HARN) / California zone 5",
"NAD83(HARN) / California zone 6",
"NAD83(HARN) / Colorado North",
"NAD83(HARN) / Colorado Central",
"NAD83(HARN) / Colorado South",
"NAD83(HARN) / Connecticut",
"NAD83(HARN) / Delaware",
"NAD83(HARN) / Florida East",
"NAD83(HARN) / Florida West",
"NAD83(HARN) / Florida North",
"NAD83(HARN) / Georgia East",
"NAD83(HARN) / Georgia West",
"NAD83(HARN) / Hawaii zone 1",
"NAD83(HARN) / Hawaii zone 2",
"NAD83(HARN) / Hawaii zone 3",
"NAD83(HARN) / Hawaii zone 4",
"NAD83(HARN) / Hawaii zone 5",
"NAD83(HARN) / Idaho East",
"NAD83(HARN) / Idaho Central",
"NAD83(HARN) / Idaho West",
"NAD83(HARN) / Illinois East",
"NAD83(HARN) / Illinois West",
"NAD83(HARN) / Indiana East",
"NAD83(HARN) / Indiana West",
"NAD83(HARN) / Iowa North",
"NAD83(HARN) / Iowa South",
"NAD83(HARN) / Kansas North",
"NAD83(HARN) / Kansas South",
"NAD83(HARN) / Kentucky North",
"NAD83(HARN) / Kentucky South",
"NAD83(HARN) / Louisiana North",
"NAD83(HARN) / Louisiana South",
"NAD83(HARN) / Maine East",
"NAD83(HARN) / Maine West",
"NAD83(HARN) / Maryland",
"NAD83(HARN) / Massachusetts Mainland",
"NAD83(HARN) / Massachusetts Island",
"NAD83(HARN) / Michigan North",
"NAD83(HARN) / Michigan Central",
"NAD83(HARN) / Michigan South",
"NAD83(HARN) / Minnesota North",
"NAD83(HARN) / Minnesota Central",
"NAD83(HARN) / Minnesota South",
"NAD83(HARN) / Mississippi East",
"NAD83(HARN) / Mississippi West",
"NAD83(HARN) / Missouri East",
"NAD83(HARN) / Missouri Central",
"NAD83(HARN) / Missouri West",
"NAD83(HARN) / Montana",
"NAD83(HARN) / Nebraska",
"NAD83(HARN) / Nevada East",
"NAD83(HARN) / Nevada Central",
"NAD83(HARN) / Nevada West",
"NAD83(HARN) / New Hampshire",
"NAD83(HARN) / New Jersey",
"NAD83(HARN) / New Mexico East",
"NAD83(HARN) / New Mexico Central",
"NAD83(HARN) / New Mexico West",
"NAD83(HARN) / New York East",
"NAD83(HARN) / New York Central",
"NAD83(HARN) / New York West",
"NAD83(HARN) / New York Long Island",
"NAD83(HARN) / North Dakota North",
"NAD83(HARN) / North Dakota South",
"NAD83(HARN) / Ohio North",
"NAD83(HARN) / Ohio South",
"NAD83(HARN) / Oklahoma North",
"NAD83(HARN) / Oklahoma South",
"NAD83(HARN) / Oregon North",
"NAD83(HARN) / Oregon South",
"NAD83(HARN) / Rhode Island",
"NAD83(HARN) / South Dakota North",
"NAD83(HARN) / South Dakota South",
"NAD83(HARN) / Tennessee",
"NAD83(HARN) / Texas North",
"NAD83(HARN) / Texas North Central",
"NAD83(HARN) / Texas Central",
"NAD83(HARN) / Texas South Central",
"NAD83(HARN) / Texas South",
"NAD83(HARN) / Utah North",
"NAD83(HARN) / Utah Central",
"NAD83(HARN) / Utah South",
"NAD83(HARN) / Vermont",
"NAD83(HARN) / Virginia North",
"NAD83(HARN) / Virginia South",
"NAD83(HARN) / Washington North",
"NAD83(HARN) / Washington South",
"NAD83(HARN) / West Virginia North",
"NAD83(HARN) / West Virginia South",
"NAD83(HARN) / Wisconsin North",
"NAD83(HARN) / Wisconsin Central",
"NAD83(HARN) / Wisconsin South",
"NAD83(HARN) / Wyoming East",
"NAD83(HARN) / Wyoming East Central",
"NAD83(HARN) / Wyoming West Central",
"NAD83(HARN) / Wyoming West",
"NAD83(HARN) / Puerto Rico and Virgin Is.",
"NAD83(HARN) / Arizona East (ft)",
"NAD83(HARN) / Arizona Central (ft)",
"NAD83(HARN) / Arizona West (ft)",
"NAD83(HARN) / California zone 1 (ftUS)",
"NAD83(HARN) / California zone 2 (ftUS)",
"NAD83(HARN) / California zone 3 (ftUS)",
"NAD83(HARN) / California zone 4 (ftUS)",
"NAD83(HARN) / California zone 5 (ftUS)",
"NAD83(HARN) / California zone 6 (ftUS)",
"NAD83(HARN) / Colorado North (ftUS)",
"NAD83(HARN) / Colorado Central (ftUS)",
"NAD83(HARN) / Colorado South (ftUS)",
"NAD83(HARN) / Connecticut (ftUS)",
"NAD83(HARN) / Delaware (ftUS)",
"NAD83(HARN) / Florida East (ftUS)",
"NAD83(HARN) / Florida West (ftUS)",
"NAD83(HARN) / Florida North (ftUS)",
"NAD83(HARN) / Georgia East (ftUS)",
"NAD83(HARN) / Georgia West (ftUS)",
"NAD83(HARN) / Idaho East (ftUS)",
"NAD83(HARN) / Idaho Central (ftUS)",
"NAD83(HARN) / Idaho West (ftUS)",
"NAD83(HARN) / Indiana East (ftUS)",
"NAD83(HARN) / Indiana West (ftUS)",
"NAD83(HARN) / Kentucky North (ftUS)",
"NAD83(HARN) / Kentucky South (ftUS)",
"NAD83(HARN) / Maryland (ftUS)",
"NAD83(HARN) / Massachusetts Mainland (ftUS)",
"NAD83(HARN) / Massachusetts Island (ftUS)",
"NAD83(HARN) / Michigan North (ft)",
"NAD83(HARN) / Michigan Central (ft)",
"NAD83(HARN) / Michigan South (ft)",
"NAD83(HARN) / Mississippi East (ftUS)",
"NAD83(HARN) / Mississippi West (ftUS)",
"NAD83(HARN) / Montana (ft)",
"NAD83(HARN) / New Mexico East (ftUS)",
"NAD83(HARN) / New Mexico Central (ftUS)",
"NAD83(HARN) / New Mexico West (ftUS)",
"NAD83(HARN) / New York East (ftUS)",
"NAD83(HARN) / New York Central (ftUS)",
"NAD83(HARN) / New York West (ftUS)",
"NAD83(HARN) / New York Long Island (ftUS)",
"NAD83(HARN) / North Dakota North (ft)",
"NAD83(HARN) / North Dakota South (ft)",
"NAD83(HARN) / Oklahoma North (ftUS)",
"NAD83(HARN) / Oklahoma South (ftUS)",
"NAD83(HARN) / Oregon North (ft)",
"NAD83(HARN) / Oregon South (ft)",
"NAD83(HARN) / Tennessee (ftUS)",
"NAD83(HARN) / Texas North (ftUS)",
"NAD83(HARN) / Texas North Central (ftUS)",
"NAD83(HARN) / Texas Central (ftUS)",
"NAD83(HARN) / Texas South Central (ftUS)",
"NAD83(HARN) / Texas South (ftUS)",
"NAD83(HARN) / Utah North (ft)",
"NAD83(HARN) / Utah Central (ft)",
"NAD83(HARN) / Utah South (ft)",
"NAD83(HARN) / Virginia North (ftUS)",
"NAD83(HARN) / Virginia South (ftUS)",
"NAD83(HARN) / Washington North (ftUS)",
"NAD83(HARN) / Washington South (ftUS)",
"NAD83(HARN) / Wisconsin North (ftUS)",
"NAD83(HARN) / Wisconsin Central (ftUS)",
"NAD83(HARN) / Wisconsin South (ftUS)",
"Beduaram / TM 13 NE",
"QND95 / Qatar National Grid",
"Segara / UTM zone 50S",
"Segara (Jakarta) / NEIEZ",
"Pulkovo 1942 / CS63 zone A1",
"Pulkovo 1942 / CS63 zone A2",
"Pulkovo 1942 / CS63 zone A3",
"Pulkovo 1942 / CS63 zone A4",
"Pulkovo 1942 / CS63 zone K2",
"Pulkovo 1942 / CS63 zone K3",
"Pulkovo 1942 / CS63 zone K4",
"Porto Santo / UTM zone 28N",
"Selvagem Grande / UTM zone 28N",
"NAD83(CSRS) / SCoPQ zone 2",
"NAD83(CSRS) / MTM zone 3",
"NAD83(CSRS) / MTM zone 4",
"NAD83(CSRS) / MTM zone 5",
"NAD83(CSRS) / MTM zone 6",
"NAD83(CSRS) / MTM zone 7",
"NAD83(CSRS) / MTM zone 8",
"NAD83(CSRS) / MTM zone 9",
"NAD83(CSRS) / MTM zone 10",
"NAD83(CSRS) / New Brunswick Stereographic",
"NAD83(CSRS) / Prince Edward Isl. Stereographic (NAD83)",
"NAD83(CSRS) / UTM zone 11N",
"NAD83(CSRS) / UTM zone 12N",
"NAD83(CSRS) / UTM zone 13N",
"NAD83(CSRS) / UTM zone 17N",
"NAD83(CSRS) / UTM zone 18N",
"NAD83(CSRS) / UTM zone 19N",
"NAD83(CSRS) / UTM zone 20N",
"NAD83(CSRS) / UTM zone 21N",
"Lisbon 1890 (Lisbon) / Portugal Bonne",
"NAD27 / Alaska Albers",
"NAD83 / Indiana East (ftUS)",
"NAD83 / Indiana West (ftUS)",
"NAD83(HARN) / Indiana East (ftUS)",
"NAD83(HARN) / Indiana West (ftUS)",
"Fort Marigot / UTM zone 20N",
"Guadeloupe 1948 / UTM zone 20N",
"CSG67 / UTM zone 22N",
"RGFG95 / UTM zone 22N",
"Martinique 1938 / UTM zone 20N",
"RGR92 / UTM zone 40S",
"Tahiti 52 / UTM zone 6S",
"Tahaa 54 / UTM zone 5S",
"IGN72 Nuku Hiva / UTM zone 7S",
"K0 1949 / UTM zone 42S",
"Combani 1950 / UTM zone 38S",
"IGN56 Lifou / UTM zone 58S",
"IGN72 Grand Terre / UTM zone 58S",
"ST87 Ouvea / UTM zone 58S",
"RGNC 1991 / Lambert New Caledonia",
"Petrels 1972 / Terre Adelie Polar Stereographic",
"Perroud 1950 / Terre Adelie Polar Stereographic",
"Saint Pierre et Miquelon 1950 / UTM zone 21N",
"MOP78 / UTM zone 1S",
"RRAF 1991 / UTM zone 20N",
"Reunion 1947 / TM Reunion",
"NAD83 / Oregon LCC (m)",
"NAD83 / Oregon GIC Lambert (ft)",
"NAD83(HARN) / Oregon LCC (m)",
"NAD83(HARN) / Oregon GIC Lambert (ft)",
"IGN53 Mare / UTM zone 58S",
"ST84 Ile des Pins / UTM zone 58S",
"ST71 Belep / UTM zone 58S",
"NEA74 Noumea / UTM zone 58S",
"Grand Comoros / UTM zone 38S",
"Segara / NEIEZ",
"Batavia / NEIEZ",
"Makassar / NEIEZ",
"Monte Mario / Italy zone 1",
"Monte Mario / Italy zone 2",
"NAD83 / BC Albers",
"SWEREF99 TM",
"SWEREF99 12 00",
"SWEREF99 13 30",
"SWEREF99 15 00",
"SWEREF99 16 30",
"SWEREF99 18 00",
"SWEREF99 14 15",
"SWEREF99 15 45",
"SWEREF99 17 15",
"SWEREF99 18 45",
"SWEREF99 20 15",
"SWEREF99 21 45",
"SWEREF99 23 15",
"RT90 7.5 gon V",
"RT90 5 gon V",
"RT90 2.5 gon V",
"RT90 0 gon",
"RT90 2.5 gon O",
"RT90 5 gon O",
"RT38 7.5 gon V",
"RT38 5 gon V",
"RT38 2.5 gon V",
"RT38 0 gon",
"RT38 2.5 gon O",
"RT38 5 gon O",
"WGS 84 / Antarctic Polar Stereographic",
"WGS 84 / Australian Antarctic Polar Stereographic",
"WGS 84 / Australian Antarctic Lambert",
"ETRS89 / LCC Europe",
"ETRS89 / LAEA Europe",
"Moznet / UTM zone 36S",
"Moznet / UTM zone 37S",
"ETRS89 / TM26",
"ETRS89 / TM27",
"ETRS89 / UTM zone 28N (N-E)",
"ETRS89 / UTM zone 29N (N-E)",
"ETRS89 / UTM zone 30N (N-E)",
"ETRS89 / UTM zone 31N (N-E)",
"ETRS89 / UTM zone 32N (N-E)",
"ETRS89 / UTM zone 33N (N-E)",
"ETRS89 / UTM zone 34N (N-E)",
"ETRS89 / UTM zone 35N (N-E)",
"ETRS89 / UTM zone 36N (N-E)",
"ETRS89 / UTM zone 37N (N-E)",
"ETRS89 / TM38",
"ETRS89 / TM39",
"Reykjavik 1900 / Lambert 1900",
"Hjorsey 1955 / Lambert 1955",
"Hjorsey 1955 / UTM zone 26N",
"Hjorsey 1955 / UTM zone 27N",
"Hjorsey 1955 / UTM zone 28N",
"ISN93 / Lambert 1993",
"Helle 1954 / Jan Mayen Grid",
"LKS92 / Latvia TM",
"IGN72 Grande Terre / UTM zone 58S",
"Porto Santo 1995 / UTM zone 28N",
"Azores Oriental 1995 / UTM zone 26N",
"Azores Central 1995 / UTM zone 26N",
"IGM95 / UTM zone 32N",
"IGM95 / UTM zone 33N",
"ED50 / Jordan TM",
"ETRS89 / TM35FIN(E,N)",
"DHDN / Soldner Berlin",
"NAD27 / Wisconsin Transverse Mercator",
"NAD83 / Wisconsin Transverse Mercator",
"NAD83(HARN) / Wisconsin Transverse Mercator",
"NAD83 / Maine CS2000 East",
"NAD83 / Maine CS2000 Central",
"NAD83 / Maine CS2000 West",
"NAD83(HARN) / Maine CS2000 East",
"NAD83(HARN) / Maine CS2000 Central",
"NAD83(HARN) / Maine CS2000 West",
"NAD83 / Michigan Oblique Mercator",
"NAD83(HARN) / Michigan Oblique Mercator",
"NAD27 / Shackleford",
"NAD83 / Texas State Mapping System",
"NAD83 / Texas Centric Lambert Conformal",
"NAD83 / Texas Centric Albers Equal Area",
"NAD83(HARN) / Texas Centric Lambert Conformal",
"NAD83(HARN) / Texas Centric Albers Equal Area",
"NAD83 / Florida GDL Albers",
"NAD83(HARN) / Florida GDL Albers",
"NAD83 / Kentucky Single Zone",
"NAD83 / Kentucky Single Zone (ftUS)",
"NAD83(HARN) / Kentucky Single Zone",
"NAD83(HARN) / Kentucky Single Zone (ftUS)",
"Tokyo / UTM zone 51N",
"Tokyo / UTM zone 52N",
"Tokyo / UTM zone 53N",
"Tokyo / UTM zone 54N",
"Tokyo / UTM zone 55N",
"JGD2000 / UTM zone 51N",
"JGD2000 / UTM zone 52N",
"JGD2000 / UTM zone 53N",
"JGD2000 / UTM zone 54N",
"JGD2000 / UTM zone 55N",
"American Samoa 1962 / American Samoa Lambert",
"Mauritania 1999 / UTM zone 28N",
"Mauritania 1999 / UTM zone 29N",
"Mauritania 1999 / UTM zone 30N",
"Gulshan 303 / Bangladesh Transverse Mercator",
"GDA94 / SA Lambert",
"ETRS89 / Guernsey Grid",
"ETRS89 / Jersey Transverse Mercator",
"AGD66 / Vicgrid66",
"GDA94 / Vicgrid94",
"GDA94 / Geoscience Australia Lambert",
"GDA94 / BCSG02",
"MAGNA-SIRGAS / Colombia Far West zone",
"MAGNA-SIRGAS / Colombia West zone",
"MAGNA-SIRGAS / Colombia Bogota zone",
"MAGNA-SIRGAS / Colombia East Central zone",
"MAGNA-SIRGAS / Colombia East zone",
"Douala 1948 / AEF west",
"Pulkovo 1942(58) / Poland zone I",
"PRS92 / Philippines zone 1",
"PRS92 / Philippines zone 2",
"PRS92 / Philippines zone 3",
"PRS92 / Philippines zone 4",
"PRS92 / Philippines zone 5",
"ETRS89 / ETRS-GK19FIN",
"ETRS89 / ETRS-GK20FIN",
"ETRS89 / ETRS-GK21FIN",
"ETRS89 / ETRS-GK22FIN",
"ETRS89 / ETRS-GK23FIN",
"ETRS89 / ETRS-GK24FIN",
"ETRS89 / ETRS-GK25FIN",
"ETRS89 / ETRS-GK26FIN",
"ETRS89 / ETRS-GK27FIN",
"ETRS89 / ETRS-GK28FIN",
"ETRS89 / ETRS-GK29FIN",
"ETRS89 / ETRS-GK30FIN",
"ETRS89 / ETRS-GK31FIN",
"Vanua Levu 1915 / Vanua Levu Grid",
"Viti Levu 1912 / Viti Levu Grid",
"Fiji 1956 / UTM zone 60S",
"Fiji 1956 / UTM zone 1S",
"Fiji 1986 / Fiji Map Grid",
"FD54 / Faroe Lambert",
"ETRS89 / Faroe Lambert",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 6",
"Pulkovo 1942 / 3-degree Gauss-Kruger CM 18E",
"Indian 1960 / UTM zone 48N",
"Indian 1960 / UTM zone 49N",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 6",
"Pulkovo 1995 / 3-degree Gauss-Kruger CM 18E",
"ST74",
"NAD83(CSRS) / BC Albers",
"NAD83(CSRS) / UTM zone 7N",
"NAD83(CSRS) / UTM zone 8N",
"NAD83(CSRS) / UTM zone 9N",
"NAD83(CSRS) / UTM zone 10N",
"NAD83(CSRS) / UTM zone 14N",
"NAD83(CSRS) / UTM zone 15N",
"NAD83(CSRS) / UTM zone 16N",
"NAD83 / Ontario MNR Lambert",
"NAD83(CSRS) / Ontario MNR Lambert",
"RGNC91-93 / Lambert New Caledonia",
"ST87 Ouvea / UTM zone 58S",
"NEA74 Noumea / Noumea Lambert",
"NEA74 Noumea / Noumea Lambert 2",
"Kertau (RSO) / RSO Malaya (ch)",
"Kertau (RSO) / RSO Malaya (m)",
"RGNC91-93 / UTM zone 57S",
"RGNC91-93 / UTM zone 58S",
"RGNC91-93 / UTM zone 59S",
"IGN53 Mare / UTM zone 59S",
"fk89 / Faroe Lambert FK89",
"NAD83 / Great Lakes Albers",
"NAD83 / Great Lakes and St Lawrence Albers",
"Indian 1960 / TM 106 NE",
"LGD2006 / Libya TM",
"GR96 / UTM zone 18N",
"GR96 / UTM zone 19N",
"GR96 / UTM zone 20N",
"GR96 / UTM zone 21N",
"GR96 / UTM zone 22N",
"GR96 / UTM zone 23N",
"GR96 / UTM zone 24N",
"GR96 / UTM zone 25N",
"GR96 / UTM zone 26N",
"GR96 / UTM zone 27N",
"GR96 / UTM zone 28N",
"GR96 / UTM zone 29N",
"LGD2006 / Libya TM zone 5",
"LGD2006 / Libya TM zone 6",
"LGD2006 / Libya TM zone 7",
"LGD2006 / Libya TM zone 8",
"LGD2006 / Libya TM zone 9",
"LGD2006 / Libya TM zone 10",
"LGD2006 / Libya TM zone 11",
"LGD2006 / Libya TM zone 12",
"LGD2006 / Libya TM zone 13",
"LGD2006 / UTM zone 32N",
"FD58 / Iraq zone",
"LGD2006 / UTM zone 33N",
"LGD2006 / UTM zone 34N",
"LGD2006 / UTM zone 35N",
"WGS 84 / SCAR IMW SP19-20",
"WGS 84 / SCAR IMW SP21-22",
"WGS 84 / SCAR IMW SP23-24",
"WGS 84 / SCAR IMW SQ01-02",
"WGS 84 / SCAR IMW SQ19-20",
"WGS 84 / SCAR IMW SQ21-22",
"WGS 84 / SCAR IMW SQ37-38",
"WGS 84 / SCAR IMW SQ39-40",
"WGS 84 / SCAR IMW SQ41-42",
"WGS 84 / SCAR IMW SQ43-44",
"WGS 84 / SCAR IMW SQ45-46",
"WGS 84 / SCAR IMW SQ47-48",
"WGS 84 / SCAR IMW SQ49-50",
"WGS 84 / SCAR IMW SQ51-52",
"WGS 84 / SCAR IMW SQ53-54",
"WGS 84 / SCAR IMW SQ55-56",
"WGS 84 / SCAR IMW SQ57-58",
"WGS 84 / SCAR IMW SR13-14",
"WGS 84 / SCAR IMW SR15-16",
"WGS 84 / SCAR IMW SR17-18",
"WGS 84 / SCAR IMW SR19-20",
"WGS 84 / SCAR IMW SR27-28",
"WGS 84 / SCAR IMW SR29-30",
"WGS 84 / SCAR IMW SR31-32",
"WGS 84 / SCAR IMW SR33-34",
"WGS 84 / SCAR IMW SR35-36",
"WGS 84 / SCAR IMW SR37-38",
"WGS 84 / SCAR IMW SR39-40",
"WGS 84 / SCAR IMW SR41-42",
"WGS 84 / SCAR IMW SR43-44",
"WGS 84 / SCAR IMW SR45-46",
"WGS 84 / SCAR IMW SR47-48",
"WGS 84 / SCAR IMW SR49-50",
"WGS 84 / SCAR IMW SR51-52",
"WGS 84 / SCAR IMW SR53-54",
"WGS 84 / SCAR IMW SR55-56",
"WGS 84 / SCAR IMW SR57-58",
"WGS 84 / SCAR IMW SR59-60",
"WGS 84 / SCAR IMW SS04-06",
"WGS 84 / SCAR IMW SS07-09",
"WGS 84 / SCAR IMW SS10-12",
"WGS 84 / SCAR IMW SS13-15",
"WGS 84 / SCAR IMW SS16-18",
"WGS 84 / SCAR IMW SS19-21",
"WGS 84 / SCAR IMW SS25-27",
"WGS 84 / SCAR IMW SS28-30",
"WGS 84 / SCAR IMW SS31-33",
"WGS 84 / SCAR IMW SS34-36",
"WGS 84 / SCAR IMW SS37-39",
"WGS 84 / SCAR IMW SS40-42",
"WGS 84 / SCAR IMW SS43-45",
"WGS 84 / SCAR IMW SS46-48",
"WGS 84 / SCAR IMW SS49-51",
"WGS 84 / SCAR IMW SS52-54",
"WGS 84 / SCAR IMW SS55-57",
"WGS 84 / SCAR IMW SS58-60",
"WGS 84 / SCAR IMW ST01-04",
"WGS 84 / SCAR IMW ST05-08",
"WGS 84 / SCAR IMW ST09-12",
"WGS 84 / SCAR IMW ST13-16",
"WGS 84 / SCAR IMW ST17-20",
"WGS 84 / SCAR IMW ST21-24",
"WGS 84 / SCAR IMW ST25-28",
"WGS 84 / SCAR IMW ST29-32",
"WGS 84 / SCAR IMW ST33-36",
"WGS 84 / SCAR IMW ST37-40",
"WGS 84 / SCAR IMW ST41-44",
"WGS 84 / SCAR IMW ST45-48",
"WGS 84 / SCAR IMW ST49-52",
"WGS 84 / SCAR IMW ST53-56",
"WGS 84 / SCAR IMW ST57-60",
"WGS 84 / SCAR IMW SU01-05",
"WGS 84 / SCAR IMW SU06-10",
"WGS 84 / SCAR IMW SU11-15",
"WGS 84 / SCAR IMW SU16-20",
"WGS 84 / SCAR IMW SU21-25",
"WGS 84 / SCAR IMW SU26-30",
"WGS 84 / SCAR IMW SU31-35",
"WGS 84 / SCAR IMW SU36-40",
"WGS 84 / SCAR IMW SU41-45",
"WGS 84 / SCAR IMW SU46-50",
"WGS 84 / SCAR IMW SU51-55",
"WGS 84 / SCAR IMW SU56-60",
"WGS 84 / SCAR IMW SV01-10",
"WGS 84 / SCAR IMW SV11-20",
"WGS 84 / SCAR IMW SV21-30",
"WGS 84 / SCAR IMW SV31-40",
"WGS 84 / SCAR IMW SV41-50",
"WGS 84 / SCAR IMW SV51-60",
"WGS 84 / SCAR IMW SW01-60",
"WGS 84 / USGS Transantarctic Mountains",
"Guam 1963 / Yap Islands",
"RGPF / UTM zone 5S",
"RGPF / UTM zone 6S",
"RGPF / UTM zone 7S",
"RGPF / UTM zone 8S",
"Estonian Coordinate System of 1992",
"Estonian Coordinate System of 1997",
"IGN63 Hiva Oa / UTM zone 7S",
"Fatu Iva 72 / UTM zone 7S",
"Tahiti 79 / UTM zone 6S",
"Moorea 87 / UTM zone 6S",
"Maupiti 83 / UTM zone 5S",
"Nakhl-e Ghanem / UTM zone 39N",
"GDA94 / NSW Lambert",
"NAD27 / California Albers",
"NAD83 / California Albers",
"NAD83(HARN) / California Albers",
"CSG67 / UTM zone 21N",
"RGFG95 / UTM zone 21N",
"Katanga 1955 / Katanga Lambert",
"Katanga 1955 / Katanga TM",
"Kasai 1953 / Congo TM zone 22",
"Kasai 1953 / Congo TM zone 24",
"IGC 1962 / Congo TM zone 12",
"IGC 1962 / Congo TM zone 14",
"IGC 1962 / Congo TM zone 16",
"IGC 1962 / Congo TM zone 18",
"IGC 1962 / Congo TM zone 20",
"IGC 1962 / Congo TM zone 22",
"IGC 1962 / Congo TM zone 24",
"IGC 1962 / Congo TM zone 26",
"IGC 1962 / Congo TM zone 28",
"IGC 1962 / Congo TM zone 30",
"Pulkovo 1942(58) / GUGiK-80",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 5",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 6",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 7",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 8",
"Pulkovo 1942(58) / Gauss-Kruger zone 3",
"Pulkovo 1942(58) / Gauss-Kruger zone 4",
"Pulkovo 1942(58) / Gauss-Kruger zone 5",
"IGN 1962 Kerguelen / UTM zone 42S",
"Le Pouce 1934 / Mauritius Grid",
"NAD83 / Alaska Albers",
"IGCB 1955 / Congo TM zone 12",
"IGCB 1955 / Congo TM zone 14",
"IGCB 1955 / Congo TM zone 16",
"IGCB 1955 / UTM zone 33S",
"Mauritania 1999 / UTM zone 28N",
"Mauritania 1999 / UTM zone 29N",
"Mauritania 1999 / UTM zone 30N",
"LKS94 / Lithuania TM",
"NAD83 / Statistics Canada Lambert",
"NAD83(CSRS) / Statistics Canada Lambert",
"WGS 84 / PDC Mercator",
"Pulkovo 1942 / CS63 zone C0",
"Pulkovo 1942 / CS63 zone C1",
"Pulkovo 1942 / CS63 zone C2",
"Mhast (onshore) / UTM zone 32S",
"Mhast (offshore) / UTM zone 32S",
"Egypt Gulf of Suez S-650 TL / Red Belt",
"Grand Cayman 1959 / UTM zone 17N",
"Little Cayman 1961 / UTM zone 17N",
"NAD83(HARN) / North Carolina",
"NAD83(HARN) / North Carolina (ftUS)",
"NAD83(HARN) / South Carolina",
"NAD83(HARN) / South Carolina (ft)",
"NAD83(HARN) / Pennsylvania North",
"NAD83(HARN) / Pennsylvania North (ftUS)",
"NAD83(HARN) / Pennsylvania South",
"NAD83(HARN) / Pennsylvania South (ftUS)",
"Hong Kong 1963 Grid System",
"IGN Astro 1960 / UTM zone 28N",
"IGN Astro 1960 / UTM zone 29N",
"IGN Astro 1960 / UTM zone 30N",
"NAD27 / UTM zone 59N",
"NAD27 / UTM zone 60N",
"NAD83 / UTM zone 59N",
"NAD83 / UTM zone 60N",
"FD54 / UTM zone 29N",
"GDM2000 / Peninsula RSO",
"GDM2000 / East Malaysia BRSO",
"GDM2000 / Johor Grid",
"GDM2000 / Sembilan and Melaka Grid",
"GDM2000 / Pahang Grid",
"GDM2000 / Selangor Grid",
"GDM2000 / Terengganu Grid",
"GDM2000 / Pinang Grid",
"GDM2000 / Kedah and Perlis Grid",
"GDM2000 / Perak Grid",
"GDM2000 / Kelantan Grid",
"KKJ / Finland zone 0",
"KKJ / Finland zone 5",
"Pulkovo 1942 / Caspian Sea Mercator",
"Pulkovo 1942 / 3-degree Gauss-Kruger zone 60",
"Pulkovo 1995 / 3-degree Gauss-Kruger zone 60",
"Karbala 1979 / UTM zone 37N",
"Karbala 1979 / UTM zone 38N",
"Karbala 1979 / UTM zone 39N",
"Nahrwan 1934 / Iraq zone",
"WGS 84 / World Mercator",
"PD/83 / 3-degree Gauss-Kruger zone 3",
"PD/83 / 3-degree Gauss-Kruger zone 4",
"RD/83 / 3-degree Gauss-Kruger zone 4",
"RD/83 / 3-degree Gauss-Kruger zone 5",
"NAD83 / Alberta 10-TM (Forest)",
"NAD83 / Alberta 10-TM (Resource)",
"NAD83(CSRS) / Alberta 10-TM (Forest)",
"NAD83(CSRS) / Alberta 10-TM (Resource)",
"NAD83(HARN) / North Carolina (ftUS)",
"VN-2000 / UTM zone 48N",
"VN-2000 / UTM zone 49N",
"Hong Kong 1963 Grid System",
"NSIDC EASE-Grid North",
"NSIDC EASE-Grid South",
"NSIDC EASE-Grid Global",
"NSIDC Sea Ice Polar Stereographic North",
"NSIDC Sea Ice Polar Stereographic South",
"WGS 84 / NSIDC Sea Ice Polar Stereographic North",
"SVY21 / Singapore TM",
"WGS 72BE / South China Sea Lambert",
"ETRS89 / Austria Lambert",
"NAD83 / Iowa North (ftUS)",
"NAD83 / Iowa South (ftUS)",
"NAD83 / Kansas North (ftUS)",
"NAD83 / Kansas South (ftUS)",
"NAD83 / Nevada East (ftUS)",
"NAD83 / Nevada Central (ftUS)",
"NAD83 / Nevada West (ftUS)",
"NAD83 / New Jersey (ftUS)",
"NAD83(HARN) / Iowa North (ftUS)",
"NAD83(HARN) / Iowa South (ftUS)",
"NAD83(HARN) / Kansas North (ftUS)",
"NAD83(HARN) / Kansas South (ftUS)",
"NAD83(HARN) / Nevada East (ftUS)",
"NAD83(HARN) / Nevada Central (ftUS)",
"NAD83(HARN) / Nevada West (ftUS)",
"NAD83(HARN) / New Jersey (ftUS)",
"NAD83 / Arkansas North (ftUS)",
"NAD83 / Arkansas South (ftUS)",
"NAD83 / Illinois East (ftUS)",
"NAD83 / Illinois West (ftUS)",
"NAD83 / New Hampshire (ftUS)",
"NAD83 / Rhode Island (ftUS)",
"PSD93 / UTM zone 39N",
"PSD93 / UTM zone 40N",
"NAD83(HARN) / Arkansas North (ftUS)",
"NAD83(HARN) / Arkansas South (ftUS)",
"NAD83(HARN) / Illinois East (ftUS)",
"NAD83(HARN) / Illinois West (ftUS)",
"NAD83(HARN) / New Hampshire (ftUS)",
"NAD83(HARN) / Rhode Island (ftUS)",
"ETRS89 / Belgian Lambert 2005",
"JAD2001 / Jamaica Metric Grid",
"JAD2001 / UTM zone 17N",
"JAD2001 / UTM zone 18N",
"NAD83 / Louisiana North (ftUS)",
"NAD83 / Louisiana South (ftUS)",
"NAD83 / Louisiana Offshore (ftUS)",
"NAD83 / South Dakota North (ftUS)",
"NAD83 / South Dakota South (ftUS)",
"NAD83(HARN) / Louisiana North (ftUS)",
"NAD83(HARN) / Louisiana South (ftUS)",
"NAD83(HARN) / South Dakota North (ftUS)",
"NAD83(HARN) / South Dakota South (ftUS)",
"Fiji 1986 / Fiji Map Grid",
"Dabola 1981 / UTM zone 28N",
"Dabola 1981 / UTM zone 29N",
"NAD83 / Maine CS2000 Central",
"NAD83(HARN) / Maine CS2000 Central",
"NAD83(NSRS2007) / Alabama East",
"NAD83(NSRS2007) / Alabama West",
"NAD83(NSRS2007) / Alaska Albers",
"NAD83(NSRS2007) / Alaska zone 1",
"NAD83(NSRS2007) / Alaska zone 2",
"NAD83(NSRS2007) / Alaska zone 3",
"NAD83(NSRS2007) / Alaska zone 4",
"NAD83(NSRS2007) / Alaska zone 5",
"NAD83(NSRS2007) / Alaska zone 6",
"NAD83(NSRS2007) / Alaska zone 7",
"NAD83(NSRS2007) / Alaska zone 8",
"NAD83(NSRS2007) / Alaska zone 9",
"NAD83(NSRS2007) / Alaska zone 10",
"NAD83(NSRS2007) / Arizona Central",
"NAD83(NSRS2007) / Arizona Central (ft)",
"NAD83(NSRS2007) / Arizona East",
"NAD83(NSRS2007) / Arizona East (ft)",
"NAD83(NSRS2007) / Arizona West",
"NAD83(NSRS2007) / Arizona West (ft)",
"NAD83(NSRS2007) / Arkansas North",
"NAD83(NSRS2007) / Arkansas North (ftUS)",
"NAD83(NSRS2007) / Arkansas South",
"NAD83(NSRS2007) / Arkansas South (ftUS)",
"NAD83(NSRS2007) / California Albers",
"NAD83(NSRS2007) / California zone 1",
"NAD83(NSRS2007) / California zone 1 (ftUS)",
"NAD83(NSRS2007) / California zone 2",
"NAD83(NSRS2007) / California zone 2 (ftUS)",
"NAD83(NSRS2007) / California zone 3",
"NAD83(NSRS2007) / California zone 3 (ftUS)",
"NAD83(NSRS2007) / California zone 4",
"NAD83(NSRS2007) / California zone 4 (ftUS)",
"NAD83(NSRS2007) / California zone 5",
"NAD83(NSRS2007) / California zone 5 (ftUS)",
"NAD83(NSRS2007) / California zone 6",
"NAD83(NSRS2007) / California zone 6 (ftUS)",
"NAD83(NSRS2007) / Colorado Central",
"NAD83(NSRS2007) / Colorado Central (ftUS)",
"NAD83(NSRS2007) / Colorado North",
"NAD83(NSRS2007) / Colorado North (ftUS)",
"NAD83(NSRS2007) / Colorado South",
"NAD83(NSRS2007) / Colorado South (ftUS)",
"NAD83(NSRS2007) / Connecticut",
"NAD83(NSRS2007) / Connecticut (ftUS)",
"NAD83(NSRS2007) / Delaware",
"NAD83(NSRS2007) / Delaware (ftUS)",
"NAD83(NSRS2007) / Florida East",
"NAD83(NSRS2007) / Florida East (ftUS)",
"NAD83(NSRS2007) / Florida GDL Albers",
"NAD83(NSRS2007) / Florida North",
"NAD83(NSRS2007) / Florida North (ftUS)",
"NAD83(NSRS2007) / Florida West",
"NAD83(NSRS2007) / Florida West (ftUS)",
"NAD83(NSRS2007) / Georgia East",
"NAD83(NSRS2007) / Georgia East (ftUS)",
"NAD83(NSRS2007) / Georgia West",
"NAD83(NSRS2007) / Georgia West (ftUS)",
"NAD83(NSRS2007) / Idaho Central",
"NAD83(NSRS2007) / Idaho Central (ftUS)",
"NAD83(NSRS2007) / Idaho East",
"NAD83(NSRS2007) / Idaho East (ftUS)",
"NAD83(NSRS2007) / Idaho West",
"NAD83(NSRS2007) / Idaho West (ftUS)",
"NAD83(NSRS2007) / Illinois East",
"NAD83(NSRS2007) / Illinois East (ftUS)",
"NAD83(NSRS2007) / Illinois West",
"NAD83(NSRS2007) / Illinois West (ftUS)",
"NAD83(NSRS2007) / Indiana East",
"NAD83(NSRS2007) / Indiana East (ftUS)",
"NAD83(NSRS2007) / Indiana West",
"NAD83(NSRS2007) / Indiana West (ftUS)",
"NAD83(NSRS2007) / Iowa North",
"NAD83(NSRS2007) / Iowa North (ftUS)",
"NAD83(NSRS2007) / Iowa South",
"NAD83(NSRS2007) / Iowa South (ftUS)",
"NAD83(NSRS2007) / Kansas North",
"NAD83(NSRS2007) / Kansas North (ftUS)",
"NAD83(NSRS2007) / Kansas South",
"NAD83(NSRS2007) / Kansas South (ftUS)",
"NAD83(NSRS2007) / Kentucky North",
"NAD83(NSRS2007) / Kentucky North (ftUS)",
"NAD83(NSRS2007) / Kentucky Single Zone",
"NAD83(NSRS2007) / Kentucky Single Zone (ftUS)",
"NAD83(NSRS2007) / Kentucky South",
"NAD83(NSRS2007) / Kentucky South (ftUS)",
"NAD83(NSRS2007) / Louisiana North",
"NAD83(NSRS2007) / Louisiana North (ftUS)",
"NAD83(NSRS2007) / Louisiana South",
"NAD83(NSRS2007) / Louisiana South (ftUS)",
"NAD83(NSRS2007) / Maine CS2000 Central",
"NAD83(NSRS2007) / Maine CS2000 East",
"NAD83(NSRS2007) / Maine CS2000 West",
"NAD83(NSRS2007) / Maine East",
"NAD83(NSRS2007) / Maine West",
"NAD83(NSRS2007) / Maryland",
"NAD83 / Utah North (ftUS)",
"Old Hawaiian / Hawaii zone 1",
"Old Hawaiian / Hawaii zone 2",
"Old Hawaiian / Hawaii zone 3",
"Old Hawaiian / Hawaii zone 4",
"Old Hawaiian / Hawaii zone 5",
"NAD83 / Utah Central (ftUS)",
"NAD83 / Utah South (ftUS)",
"NAD83(HARN) / Utah North (ftUS)",
"NAD83(HARN) / Utah Central (ftUS)",
"NAD83(HARN) / Utah South (ftUS)",
"WGS 84 / North Pole LAEA Bering Sea",
"WGS 84 / North Pole LAEA Alaska",
"WGS 84 / North Pole LAEA Canada",
"WGS 84 / North Pole LAEA Atlantic",
"WGS 84 / North Pole LAEA Europe",
"WGS 84 / North Pole LAEA Russia",
"GDA94 / Australian Albers",
"NAD83 / Yukon Albers",
"NAD83(CSRS) / Yukon Albers",
"NAD83 / NWT Lambert",
"NAD83(CSRS) / NWT Lambert",
"NAD83(NSRS2007) / Maryland (ftUS)",
"NAD83(NSRS2007) / Massachusetts Island",
"NAD83(NSRS2007) / Massachusetts Island (ftUS)",
"NAD83(NSRS2007) / Massachusetts Mainland",
"NAD83(NSRS2007) / Massachusetts Mainland (ftUS)",
"NAD83(NSRS2007) / Michigan Central",
"NAD83(NSRS2007) / Michigan Central (ft)",
"NAD83(NSRS2007) / Michigan North",
"NAD83(NSRS2007) / Michigan North (ft)",
"NAD83(NSRS2007) / Michigan Oblique Mercator",
"NAD83(NSRS2007) / Michigan South",
"NAD83(NSRS2007) / Michigan South (ft)",
"NAD83(NSRS2007) / Minnesota Central",
"NAD83(NSRS2007) / Minnesota North",
"NAD83(NSRS2007) / Minnesota South",
"NAD83(NSRS2007) / Mississippi East",
"NAD83(NSRS2007) / Mississippi East (ftUS)",
"NAD83(NSRS2007) / Mississippi West",
"NAD83(NSRS2007) / Mississippi West (ftUS)",
"NAD83(NSRS2007) / Missouri Central",
"NAD83(NSRS2007) / Missouri East",
"NAD83(NSRS2007) / Missouri West",
"NAD83(NSRS2007) / Montana",
"NAD83(NSRS2007) / Montana (ft)",
"NAD83(NSRS2007) / Nebraska",
"NAD83(NSRS2007) / Nevada Central",
"NAD83(NSRS2007) / Nevada Central (ftUS)",
"NAD83(NSRS2007) / Nevada East",
"NAD83(NSRS2007) / Nevada East (ftUS)",
"NAD83(NSRS2007) / Nevada West",
"NAD83(NSRS2007) / Nevada West (ftUS)",
"NAD83(NSRS2007) / New Hampshire",
"NAD83(NSRS2007) / New Hampshire (ftUS)",
"NAD83(NSRS2007) / New Jersey",
"NAD83(NSRS2007) / New Jersey (ftUS)",
"NAD83(NSRS2007) / New Mexico Central",
"NAD83(NSRS2007) / New Mexico Central (ftUS)",
"NAD83(NSRS2007) / New Mexico East",
"NAD83(NSRS2007) / New Mexico East (ftUS)",
"NAD83(NSRS2007) / New Mexico West",
"NAD83(NSRS2007) / New Mexico West (ftUS)",
"NAD83(NSRS2007) / New York Central",
"NAD83(NSRS2007) / New York Central (ftUS)",
"NAD83(NSRS2007) / New York East",
"NAD83(NSRS2007) / New York East (ftUS)",
"NAD83(NSRS2007) / New York Long Island",
"NAD83(NSRS2007) / New York Long Island (ftUS)",
"NAD83(NSRS2007) / New York West",
"NAD83(NSRS2007) / New York West (ftUS)",
"NAD83(NSRS2007) / North Carolina",
"NAD83(NSRS2007) / North Carolina (ftUS)",
"NAD83(NSRS2007) / North Dakota North",
"NAD83(NSRS2007) / North Dakota North (ft)",
"NAD83(NSRS2007) / North Dakota South",
"NAD83(NSRS2007) / North Dakota South (ft)",
"NAD83(NSRS2007) / Ohio North",
"NAD83(NSRS2007) / Ohio South",
"NAD83(NSRS2007) / Oklahoma North",
"NAD83(NSRS2007) / Oklahoma North (ftUS)",
"NAD83(NSRS2007) / Oklahoma South",
"NAD83(NSRS2007) / Oklahoma South (ftUS)",
"NAD83(NSRS2007) / Oregon LCC (m)",
"NAD83(NSRS2007) / Oregon GIC Lambert (ft)",
"NAD83(NSRS2007) / Oregon North",
"NAD83(NSRS2007) / Oregon North (ft)",
"NAD83(NSRS2007) / Oregon South",
"NAD83(NSRS2007) / Oregon South (ft)",
"NAD83(NSRS2007) / Pennsylvania North",
"NAD83(NSRS2007) / Pennsylvania North (ftUS)",
"NAD83(NSRS2007) / Pennsylvania South",
"NAD83(NSRS2007) / Pennsylvania South (ftUS)",
"NAD83(NSRS2007) / Rhode Island",
"NAD83(NSRS2007) / Rhode Island (ftUS)",
"NAD83(NSRS2007) / South Carolina",
"NAD83(NSRS2007) / South Carolina (ft)",
"NAD83(NSRS2007) / South Dakota North",
"NAD83(NSRS2007) / South Dakota North (ftUS)",
"NAD83(NSRS2007) / South Dakota South",
"NAD83(NSRS2007) / South Dakota South (ftUS)",
"NAD83(NSRS2007) / Tennessee",
"NAD83(NSRS2007) / Tennessee (ftUS)",
"NAD83(NSRS2007) / Texas Central",
"NAD83(NSRS2007) / Texas Central (ftUS)",
"NAD83(NSRS2007) / Texas Centric Albers Equal Area",
"NAD83(NSRS2007) / Texas Centric Lambert Conformal",
"NAD83(NSRS2007) / Texas North",
"NAD83(NSRS2007) / Texas North (ftUS)",
"NAD83(NSRS2007) / Texas North Central",
"NAD83(NSRS2007) / Texas North Central (ftUS)",
"NAD83(NSRS2007) / Texas South",
"NAD83(NSRS2007) / Texas South (ftUS)",
"NAD83(NSRS2007) / Texas South Central",
"NAD83(NSRS2007) / Texas South Central (ftUS)",
"NAD83(NSRS2007) / Utah Central",
"NAD83(NSRS2007) / Utah Central (ft)",
"NAD83(NSRS2007) / Utah Central (ftUS)",
"NAD83(NSRS2007) / Utah North",
"NAD83(NSRS2007) / Utah North (ft)",
"NAD83(NSRS2007) / Utah North (ftUS)",
"NAD83(NSRS2007) / Utah South",
"NAD83(NSRS2007) / Utah South (ft)",
"NAD83(NSRS2007) / Utah South (ftUS)",
"NAD83(NSRS2007) / Vermont",
"NAD83(NSRS2007) / Virginia North",
"NAD83(NSRS2007) / Virginia North (ftUS)",
"NAD83(NSRS2007) / Virginia South",
"NAD83(NSRS2007) / Virginia South (ftUS)",
"NAD83(NSRS2007) / Washington North",
"NAD83(NSRS2007) / Washington North (ftUS)",
"NAD83(NSRS2007) / Washington South",
"NAD83(NSRS2007) / Washington South (ftUS)",
"NAD83(NSRS2007) / West Virginia North",
"NAD83(NSRS2007) / West Virginia South",
"NAD83(NSRS2007) / Wisconsin Central",
"NAD83(NSRS2007) / Wisconsin Central (ftUS)",
"NAD83(NSRS2007) / Wisconsin North",
"NAD83(NSRS2007) / Wisconsin North (ftUS)",
"NAD83(NSRS2007) / Wisconsin South",
"NAD83(NSRS2007) / Wisconsin South (ftUS)",
"NAD83(NSRS2007) / Wisconsin Transverse Mercator",
"NAD83(NSRS2007) / Wyoming East",
"NAD83(NSRS2007) / Wyoming East Central",
"NAD83(NSRS2007) / Wyoming West Central",
"NAD83(NSRS2007) / Wyoming West",
"NAD83(NSRS2007) / UTM zone 59N",
"NAD83(NSRS2007) / UTM zone 60N",
"NAD83(NSRS2007) / UTM zone 1N",
"NAD83(NSRS2007) / UTM zone 2N",
"NAD83(NSRS2007) / UTM zone 3N",
"NAD83(NSRS2007) / UTM zone 4N",
"NAD83(NSRS2007) / UTM zone 5N",
"NAD83(NSRS2007) / UTM zone 6N",
"NAD83(NSRS2007) / UTM zone 7N",
"NAD83(NSRS2007) / UTM zone 8N",
"NAD83(NSRS2007) / UTM zone 9N",
"NAD83(NSRS2007) / UTM zone 10N",
"NAD83(NSRS2007) / UTM zone 11N",
"NAD83(NSRS2007) / UTM zone 12N",
"NAD83(NSRS2007) / UTM zone 13N",
"NAD83(NSRS2007) / UTM zone 14N",
"NAD83(NSRS2007) / UTM zone 15N",
"NAD83(NSRS2007) / UTM zone 16N",
"NAD83(NSRS2007) / UTM zone 17N",
"NAD83(NSRS2007) / UTM zone 18N",
"NAD83(NSRS2007) / UTM zone 19N",
"Reunion 1947 / TM Reunion",
"NAD83(NSRS2007) / Ohio North (ftUS)",
"NAD83(NSRS2007) / Ohio South (ftUS)",
"NAD83(NSRS2007) / Wyoming East (ftUS)",
"NAD83(NSRS2007) / Wyoming East Central (ftUS)",
"NAD83(NSRS2007) / Wyoming West Central (ftUS)",
"NAD83(NSRS2007) / Wyoming West (ftUS)",
"NAD83 / Ohio North (ftUS)",
"NAD83 / Ohio South (ftUS)",
"NAD83 / Wyoming East (ftUS)",
"NAD83 / Wyoming East Central (ftUS)",
"NAD83 / Wyoming West Central (ftUS)",
"NAD83 / Wyoming West (ftUS)",
"NAD83(HARN) / UTM zone 10N",
"NAD83(HARN) / UTM zone 11N",
"NAD83(HARN) / UTM zone 12N",
"NAD83(HARN) / UTM zone 13N",
"NAD83(HARN) / UTM zone 14N",
"NAD83(HARN) / UTM zone 15N",
"NAD83(HARN) / UTM zone 16N",
"NAD83(HARN) / UTM zone 17N",
"NAD83(HARN) / UTM zone 18N",
"NAD83(HARN) / UTM zone 19N",
"NAD83(HARN) / UTM zone 4N",
"NAD83(HARN) / UTM zone 5N",
"WGS 84 / Mercator 41",
"NAD83(HARN) / Ohio North (ftUS)",
"NAD83(HARN) / Ohio South (ftUS)",
"NAD83(HARN) / Wyoming East (ftUS)",
"NAD83(HARN) / Wyoming East Central (ftUS)",
"NAD83(HARN) / Wyoming West Central (ftUS)",
"NAD83(HARN) / Wyoming West (ftUS)",
"NAD83 / Hawaii zone 3 (ftUS)",
"NAD83(HARN) / Hawaii zone 3 (ftUS)",
"NAD83(CSRS) / UTM zone 22N",
"WGS 84 / South Georgia Lambert",
"ETRS89 / Portugal TM06",
"NZGD2000 / Chatham Island Circuit 2000",
"HTRS96 / Croatia TM",
"HTRS96 / Croatia LCC",
"HTRS96 / UTM zone 33N",
"HTRS96 / UTM zone 34N",
"Bermuda 1957 / UTM zone 20N",
"BDA2000 / Bermuda 2000 National Grid",
"NAD27 / Alberta 3TM ref merid 111 W",
"NAD27 / Alberta 3TM ref merid 114 W",
"NAD27 / Alberta 3TM ref merid 117 W",
"NAD27 / Alberta 3TM ref merid 120 W",
"NAD83 / Alberta 3TM ref merid 111 W",
"NAD83 / Alberta 3TM ref merid 114 W",
"NAD83 / Alberta 3TM ref merid 117 W",
"NAD83 / Alberta 3TM ref merid 120 W",
"NAD83(CSRS) / Alberta 3TM ref merid 111 W",
"NAD83(CSRS) / Alberta 3TM ref merid 114 W",
"NAD83(CSRS) / Alberta 3TM ref merid 117 W",
"NAD83(CSRS) / Alberta 3TM ref merid 120 W",
"Pitcairn 2006 / Pitcairn TM 2006",
"Pitcairn 1967 / UTM zone 9S",
"Popular Visualisation CRS / Mercator",
"World Equidistant Cylindrical (Sphere)",
"MGI / Slovene National Grid",
"NZGD2000 / Auckland Islands TM 2000",
"NZGD2000 / Campbell Island TM 2000",
"NZGD2000 / Antipodes Islands TM 2000",
"NZGD2000 / Raoul Island TM 2000",
"NZGD2000 / Chatham Islands TM 2000",
"Slovenia 1996 / Slovene National Grid",
"NAD27 / Cuba Norte",
"NAD27 / Cuba Sur",
"NAD27 / MTQ Lambert",
"NAD83 / MTQ Lambert",
"NAD83(CSRS) / MTQ Lambert",
"NAD27 / Alberta 3TM ref merid 120 W",
"NAD83 / Alberta 3TM ref merid 120 W",
"NAD83(CSRS) / Alberta 3TM ref merid 120 W",
"ETRS89 / Belgian Lambert 2008",
"NAD83 / Mississippi TM",
"NAD83(HARN) / Mississippi TM",
"NAD83(NSRS2007) / Mississippi TM",
"HD1909",
"TWD67",
"TWD97",
"TWD97",
"TWD97",
"TWD97 / TM2 zone 119",
"TWD97 / TM2 zone 121",
"TWD67 / TM2 zone 119",
"TWD67 / TM2 zone 121",
"Hu Tzu Shan 1950 / UTM zone 51N",
"WGS 84 / PDC Mercator",
"Pulkovo 1942(58) / Gauss-Kruger zone 2",
"Pulkovo 1942(83) / Gauss-Kruger zone 2",
"Pulkovo 1942(83) / Gauss-Kruger zone 3",
"Pulkovo 1942(83) / Gauss-Kruger zone 4",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 3",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 4",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 9",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 10",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 6",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 7",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 8",
"Pulkovo 1942(58) / Stereo70",
"SWEREF99 / RT90 7.5 gon V emulation",
"SWEREF99 / RT90 5 gon V emulation",
"SWEREF99 / RT90 2.5 gon V emulation",
"SWEREF99 / RT90 0 gon emulation",
"SWEREF99 / RT90 2.5 gon O emulation",
"SWEREF99 / RT90 5 gon O emulation",
"NZGD2000 / NZCS2000",
"RSRGD2000 / DGLC2000",
"County ST74",
"EGM2008 geoid height",
"WGS 84 / Pseudo-Mercator",
"ETRS89 / GK19FIN",
"ETRS89 / GK20FIN",
"ETRS89 / GK21FIN",
"ETRS89 / GK22FIN",
"ETRS89 / GK23FIN",
"ETRS89 / GK24FIN",
"ETRS89 / GK25FIN",
"ETRS89 / GK26FIN",
"ETRS89 / GK27FIN",
"ETRS89 / GK28FIN",
"ETRS89 / GK29FIN",
"ETRS89 / GK30FIN",
"ETRS89 / GK31FIN",
"Fao 1979 height",
"IGRS",
"IGRS",
"IGRS",
"IGRS / UTM zone 37N",
"IGRS / UTM zone 38N",
"IGRS / UTM zone 39N",
"ED50 / Iraq National Grid",
"N2000 height",
"KKJ / Finland Uniform Coordinate System + N60 height",
"ETRS89 / TM35FIN(N,E) + N60 height",
"ETRS89 / TM35FIN(N,E) + N2000 height",
"MGI 1901",
"MGI 1901 / Balkans zone 5",
"MGI 1901 / Balkans zone 6",
"MGI 1901 / Balkans zone 7",
"MGI 1901 / Balkans zone 8",
"MGI 1901 / Slovenia Grid",
"MGI 1901 / Slovene National Grid",
"Puerto Rico / UTM zone 20N",
"RGF93 / CC42",
"RGF93 / CC43",
"RGF93 / CC44",
"RGF93 / CC45",
"RGF93 / CC46",
"RGF93 / CC47",
"RGF93 / CC48",
"RGF93 / CC49",
"RGF93 / CC50",
"NAD83 / Virginia Lambert",
"NAD83(HARN) / Virginia Lambert",
"NAD83(NSRS2007) / Virginia Lambert",
"WGS 84 / NSIDC EASE-Grid North",
"WGS 84 / NSIDC EASE-Grid South",
"WGS 84 / NSIDC EASE-Grid Global",
"WGS 84 / NSIDC Sea Ice Polar Stereographic South",
"NAD83 / Canada Atlas Lambert",
"NAD83(CSRS) / Canada Atlas Lambert",
"Katanga 1955 / Katanga Lambert",
"Katanga 1955 / Katanga Gauss zone A",
"Katanga 1955 / Katanga Gauss zone B",
"Katanga 1955 / Katanga Gauss zone C",
"Katanga 1955 / Katanga Gauss zone D",
"Puerto Rico State Plane CS of 1927",
"Puerto Rico / St. Croix",
"Guam 1963 / Guam SPCS",
"WGS 84 / Mercator 41",
"WGS 84 / Arctic Polar Stereographic",
"WGS 84 / IBCAO Polar Stereographic",
"WGS 84 / Dubai Local TM",
"MOLDREF99",
"Unknown datum based upon the Airy 1830 ellipsoid",
"Unknown datum based upon the Airy Modified 1849 ellipsoid",
"Unknown datum based upon the Australian National Spheroid",
"Unknown datum based upon the Bessel 1841 ellipsoid",
"Unknown datum based upon the Bessel Modified ellipsoid",
"Unknown datum based upon the Bessel Namibia ellipsoid",
"Unknown datum based upon the Clarke 1858 ellipsoid",
"Unknown datum based upon the Clarke 1866 ellipsoid",
"Unknown datum based upon the Clarke 1866 Michigan ellipsoid",
"Unknown datum based upon the Clarke 1880 (Benoit) ellipsoid",
"Unknown datum based upon the Clarke 1880 (IGN) ellipsoid",
"Unknown datum based upon the Clarke 1880 (RGS) ellipsoid",
"Unknown datum based upon the Clarke 1880 (Arc) ellipsoid",
"Unknown datum based upon the Clarke 1880 (SGA 1922) ellipsoid",
"Unknown datum based upon the Everest 1830 (1937 Adjustment) ellipsoid",
"Unknown datum based upon the Everest 1830 (1967 Definition) ellipsoid",
"MOLDREF99",
"Unknown datum based upon the Everest 1830 Modified ellipsoid",
"Unknown datum based upon the GRS 1980 ellipsoid",
"Unknown datum based upon the Helmert 1906 ellipsoid",
"Unknown datum based upon the Indonesian National Spheroid",
"Unknown datum based upon the International 1924 ellipsoid",
"MOLDREF99",
"Unknown datum based upon the Krassowsky 1940 ellipsoid",
"Unknown datum based upon the NWL 9D ellipsoid",
"MOLDREF99 / Moldova TM",
"Unknown datum based upon the Plessis 1817 ellipsoid",
"Unknown datum based upon the Struve 1860 ellipsoid",
"Unknown datum based upon the War Office ellipsoid",
"Unknown datum based upon the WGS 84 ellipsoid",
"Unknown datum based upon the GEM 10C ellipsoid",
"Unknown datum based upon the OSU86F ellipsoid",
"Unknown datum based upon the OSU91A ellipsoid",
"Unknown datum based upon the Clarke 1880 ellipsoid",
"Unknown datum based upon the Authalic Sphere",
"Unknown datum based upon the GRS 1967 ellipsoid",
"WGS 84 / TMzn35N",
"WGS 84 / TMzn36N",
"RGRDC 2005",
"RGRDC 2005",
"Unknown datum based upon the Average Terrestrial System 1977 ellipsoid",
"Unknown datum based upon the Everest (1830 Definition) ellipsoid",
"Unknown datum based upon the WGS 72 ellipsoid",
"Unknown datum based upon the Everest 1830 (1962 Definition) ellipsoid",
"Unknown datum based upon the Everest 1830 (1975 Definition) ellipsoid",
"RGRDC 2005",
"Unspecified datum based upon the GRS 1980 Authalic Sphere",
"RGRDC 2005 / Congo TM zone 12",
"RGRDC 2005 / Congo TM zone 14",
"RGRDC 2005 / Congo TM zone 16",
"RGRDC 2005 / Congo TM zone 18",
"Unspecified datum based upon the Clarke 1866 Authalic Sphere",
"Unspecified datum based upon the International 1924 Authalic Sphere",
"Unspecified datum based upon the Hughes 1980 ellipsoid",
"Popular Visualisation CRS",
"RGRDC 2005 / Congo TM zone 20",
"RGRDC 2005 / Congo TM zone 22",
"RGRDC 2005 / Congo TM zone 24",
"RGRDC 2005 / Congo TM zone 26",
"RGRDC 2005 / Congo TM zone 28",
"RGRDC 2005 / UTM zone 33S",
"RGRDC 2005 / UTM zone 34S",
"RGRDC 2005 / UTM zone 35S",
"Chua / UTM zone 23S",
"SREF98",
"SREF98",
"SREF98",
"REGCAN95",
"REGCAN95",
"REGCAN95",
"REGCAN95 / UTM zone 27N",
"REGCAN95 / UTM zone 28N",
"WGS 84 / World Equidistant Cylindrical",
"World Equidistant Cylindrical (Sphere)",
"ETRS89 / DKTM1",
"ETRS89 / DKTM2",
"ETRS89 / DKTM3",
"ETRS89 / DKTM4",
"ETRS89 / DKTM1 + DVR90 height",
"ETRS89 / DKTM2 + DVR90 height",
"ETRS89 / DKTM3 + DVR90 height",
"ETRS89 / DKTM4 + DVR90 height",
"Greek",
"GGRS87",
"ATS77",
"KKJ",
"RT90",
"Samboja",
"LKS94 (ETRS89)",
"Tete",
"Madzansua",
"Observatario",
"Moznet",
"Indian 1960",
"FD58",
"EST92",
"PSD93",
"Old Hawaiian",
"St. Lawrence Island",
"St. Paul Island",
"St. George Island",
"Puerto Rico",
"NAD83(CSRS98)",
"Israel 1993",
"Locodjo 1965",
"Abidjan 1987",
"Kalianpur 1937",
"Kalianpur 1962",
"Kalianpur 1975",
"Hanoi 1972",
"Hartebeesthoek94",
"CH1903",
"CH1903+",
"CHTRF95",
"NAD83(HARN)",
"Rassadiran",
"ED50(ED77)",
"Dabola 1981",
"S-JTSK",
"Mount Dillon",
"Naparima 1955",
"ELD79",
"Chos Malal 1914",
"Pampa del Castillo",
"Korean 1985",
"Yemen NGN96",
"South Yemen",
"Bissau",
"Korean 1995",
"NZGD2000",
"Accra",
"American Samoa 1962",
"SIRGAS 1995",
"RGF93",
"POSGAR",
"IRENET95",
"Sierra Leone 1924",
"Sierra Leone 1968",
"Australian Antarctic",
"Pulkovo 1942(83)",
"Pulkovo 1942(58)",
"EST97",
"Luxembourg 1930",
"Azores Occidental 1939",
"Azores Central 1948",
"Azores Oriental 1940",
"Madeira 1936",
"OSNI 1952",
"REGVEN",
"POSGAR 98",
"Albanian 1987",
"Douala 1948",
"Manoca 1962",
"Qornoq 1927",
"Scoresbysund 1952",
"Ammassalik 1958",
"Garoua",
"Kousseri",
"Egypt 1930",
"Pulkovo 1995",
"Adindan",
"AGD66",
"AGD84",
"Ain el Abd",
"Afgooye",
"Agadez",
"Lisbon",
"Aratu",
"Arc 1950",
"Arc 1960",
"Batavia",
"Barbados 1938",
"Beduaram",
"Beijing 1954",
"Belge 1950",
"Bermuda 1957",
"NAD83 / BLM 59N (ftUS)",
"Bogota 1975",
"Bukit Rimpah",
"Camacupa",
"Campo Inchauspe",
"Cape",
"Carthage",
"Chua",
"Corrego Alegre 1970-72",
"Cote d'Ivoire",
"Deir ez Zor",
"Douala",
"Egypt 1907",
"ED50",
"ED87",
"Fahud",
"Gandajika 1970",
"Garoua",
"Guyane Francaise",
"Hu Tzu Shan 1950",
"HD72",
"ID74",
"Indian 1954",
"Indian 1975",
"Jamaica 1875",
"JAD69",
"Kalianpur 1880",
"Kandawala",
"Kertau 1968",
"KOC",
"La Canoa",
"PSAD56",
"Lake",
"Leigon",
"Liberia 1964",
"Lome",
"Luzon 1911",
"Hito XVIII 1963",
"Herat North",
"Mahe 1971",
"Makassar",
"ETRS89",
"Malongo 1987",
"Manoca",
"Merchich",
"Massawa",
"Minna",
"Mhast",
"Monte Mario",
"M'poraloko",
"NAD27",
"NAD27 Michigan",
"NAD83",
"Nahrwan 1967",
"Naparima 1972",
"NZGD49",
"NGO 1948",
"Datum 73",
"NTF",
"NSWC 9Z-2",
"OSGB 1936",
"OSGB70",
"OS(SN)80",
"Padang",
"Palestine 1923",
"Pointe Noire",
"GDA94",
"Pulkovo 1942",
"Qatar 1974",
"Qatar 1948",
"Qornoq",
"Loma Quintana",
"Amersfoort",
"SAD69",
"Sapper Hill 1943",
"Schwarzeck",
"Segora",
"Serindung",
"Sudan",
"Tananarive",
"Timbalai 1948",
"TM65",
"TM75",
"Tokyo",
"Trinidad 1903",
"TC(1948)",
"Voirol 1875",
"Bern 1938",
"Nord Sahara 1959",
"RT38",
"Yacare",
"Yoff",
"Zanderij",
"MGI",
"Belge 1972",
"DHDN",
"Conakry 1905",
"Dealul Piscului 1930",
"Dealul Piscului 1970",
"NGN",
"KUDAMS",
"WGS 72",
"WGS 72BE",
"WGS 84",
"WGS 84 (geographic 3D)",
"WGS 84 (geocentric)",
"WGS 84 (3D)",
"ITRF88 (geocentric)",
"ITRF89 (geocentric)",
"ITRF90 (geocentric)",
"ITRF91 (geocentric)",
"ITRF92 (geocentric)",
"ITRF93 (geocentric)",
"ITRF94 (geocentric)",
"ITRF96 (geocentric)",
"ITRF97 (geocentric)",
"Australian Antarctic (3D)",
"Australian Antarctic (geocentric)",
"EST97 (3D)",
"EST97 (geocentric)",
"CHTRF95 (3D)",
"CHTRF95 (geocentric)",
"ETRS89 (3D)",
"ETRS89 (geocentric)",
"GDA94 (3D)",
"GDA94 (geocentric)",
"Hartebeesthoek94 (3D)",
"Hartebeesthoek94 (geocentric)",
"IRENET95 (3D)",
"IRENET95 (geocentric)",
"JGD2000 (3D)",
"JGD2000 (geocentric)",
"LKS94 (ETRS89) (3D)",
"LKS94 (ETRS89) (geocentric)",
"Moznet (3D)",
"Moznet (geocentric)",
"NAD83(CSRS) (3D)",
"NAD83(CSRS) (geocentric)",
"NAD83(HARN) (3D)",
"NAD83(HARN) (geocentric)",
"NZGD2000 (3D)",
"NZGD2000 (geocentric)",
"POSGAR 98 (3D)",
"POSGAR 98 (geocentric)",
"REGVEN (3D)",
"REGVEN (geocentric)",
"RGF93 (3D)",
"RGF93 (geocentric)",
"RGFG95 (3D)",
"RGFG95 (geocentric)",
"RGR92 (3D)",
"RGR92 (geocentric)",
"SIRGAS (3D)",
"SIRGAS (geocentric)",
"SWEREF99 (3D)",
"SWEREF99 (geocentric)",
"Yemen NGN96 (3D)",
"Yemen NGN96 (geocentric)",
"RGNC 1991 (3D)",
"RGNC 1991 (geocentric)",
"RRAF 1991 (3D)",
"RRAF 1991 (geocentric)",
"ITRF2000 (geocentric)",
"ISN93 (3D)",
"ISN93 (geocentric)",
"LKS92 (3D)",
"LKS92 (geocentric)",
"Kertau 1968 / Johor Grid",
"Kertau 1968 / Sembilan and Melaka Grid",
"Kertau 1968 / Pahang Grid",
"Kertau 1968 / Selangor Grid",
"Kertau 1968 / Terengganu Grid",
"Kertau 1968 / Pinang Grid",
"Kertau 1968 / Kedah and Perlis Grid",
"Kertau 1968 / Perak Revised Grid",
"Kertau 1968 / Kelantan Grid",
"NAD27 / BLM 59N (ftUS)",
"NAD27 / BLM 60N (ftUS)",
"NAD27 / BLM 1N (ftUS)",
"NAD27 / BLM 2N (ftUS)",
"NAD27 / BLM 3N (ftUS)",
"NAD27 / BLM 4N (ftUS)",
"NAD27 / BLM 5N (ftUS)",
"NAD27 / BLM 6N (ftUS)",
"NAD27 / BLM 7N (ftUS)",
"NAD27 / BLM 8N (ftUS)",
"NAD27 / BLM 9N (ftUS)",
"NAD27 / BLM 10N (ftUS)",
"NAD27 / BLM 11N (ftUS)",
"NAD27 / BLM 12N (ftUS)",
"NAD27 / BLM 13N (ftUS)",
"NAD83(HARN) / Guam Map Grid",
"Katanga 1955 / Katanga Lambert",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 7",
"NAD27 / BLM 18N (ftUS)",
"NAD27 / BLM 19N (ftUS)",
"NAD83 / BLM 60N (ftUS)",
"NAD83 / BLM 1N (ftUS)",
"NAD83 / BLM 2N (ftUS)",
"NAD83 / BLM 3N (ftUS)",
"NAD83 / BLM 4N (ftUS)",
"NAD83 / BLM 5N (ftUS)",
"NAD83 / BLM 6N (ftUS)",
"NAD83 / BLM 7N (ftUS)",
"NAD83 / BLM 8N (ftUS)",
"NAD83 / BLM 9N (ftUS)",
"NAD83 / BLM 10N (ftUS)",
"NAD83 / BLM 11N (ftUS)",
"NAD83 / BLM 12N (ftUS)",
"NAD83 / BLM 13N (ftUS)",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 8",
"NAD83(NSRS2007) / Puerto Rico and Virgin Is.",
"NAD83 / BLM 18N (ftUS)",
"NAD83 / BLM 19N (ftUS)",
"NZVD2009 height",
"NAD27 / Pennsylvania South",
"NAD27 / New York Long Island",
"NAD83 / South Dakota North (ftUS)",
"Dunedin-Bluff 1960 height",
"WGS 84 / Australian Centre for Remote Sensing Lambert",
"RGSPM06",
"RGSPM06",
"RGSPM06",
"RGSPM06 / UTM zone 21N",
"RGM04",
"RGM04",
"RGM04",
"RGM04 / UTM zone 38S",
"Cadastre 1997",
"Cadastre 1997",
"Cadastre 1997 / UTM zone 38S",
"Cadastre 1997",
"China Geodetic Coordinate System 2000",
"China Geodetic Coordinate System 2000",
"Mexico ITRF92",
"Mexico ITRF92",
"Mexico ITRF92",
"Mexico ITRF92 / UTM zone 11N",
"Mexico ITRF92 / UTM zone 12N",
"Mexico ITRF92 / UTM zone 13N",
"Mexico ITRF92 / UTM zone 14N",
"Mexico ITRF92 / UTM zone 15N",
"Mexico ITRF92 / UTM zone 16N",
"China Geodetic Coordinate System 2000",
"CGCS2000 / Gauss-Kruger zone 13",
"CGCS2000 / Gauss-Kruger zone 14",
"CGCS2000 / Gauss-Kruger zone 15",
"CGCS2000 / Gauss-Kruger zone 16",
"CGCS2000 / Gauss-Kruger zone 17",
"CGCS2000 / Gauss-Kruger zone 18",
"CGCS2000 / Gauss-Kruger zone 19",
"CGCS2000 / Gauss-Kruger zone 20",
"CGCS2000 / Gauss-Kruger zone 21",
"CGCS2000 / Gauss-Kruger zone 22",
"CGCS2000 / Gauss-Kruger zone 23",
"CGCS2000 / Gauss-Kruger CM 75E",
"CGCS2000 / Gauss-Kruger CM 81E",
"CGCS2000 / Gauss-Kruger CM 87E",
"CGCS2000 / Gauss-Kruger CM 93E",
"CGCS2000 / Gauss-Kruger CM 99E",
"CGCS2000 / Gauss-Kruger CM 105E",
"CGCS2000 / Gauss-Kruger CM 111E",
"CGCS2000 / Gauss-Kruger CM 117E",
"CGCS2000 / Gauss-Kruger CM 123E",
"CGCS2000 / Gauss-Kruger CM 129E",
"CGCS2000 / Gauss-Kruger CM 135E",
"CGCS2000 / 3-degree Gauss-Kruger zone 25",
"CGCS2000 / 3-degree Gauss-Kruger zone 26",
"CGCS2000 / 3-degree Gauss-Kruger zone 27",
"CGCS2000 / 3-degree Gauss-Kruger zone 28",
"CGCS2000 / 3-degree Gauss-Kruger zone 29",
"CGCS2000 / 3-degree Gauss-Kruger zone 30",
"CGCS2000 / 3-degree Gauss-Kruger zone 31",
"CGCS2000 / 3-degree Gauss-Kruger zone 32",
"CGCS2000 / 3-degree Gauss-Kruger zone 33",
"CGCS2000 / 3-degree Gauss-Kruger zone 34",
"CGCS2000 / 3-degree Gauss-Kruger zone 35",
"CGCS2000 / 3-degree Gauss-Kruger zone 36",
"CGCS2000 / 3-degree Gauss-Kruger zone 37",
"CGCS2000 / 3-degree Gauss-Kruger zone 38",
"CGCS2000 / 3-degree Gauss-Kruger zone 39",
"CGCS2000 / 3-degree Gauss-Kruger zone 40",
"CGCS2000 / 3-degree Gauss-Kruger zone 41",
"CGCS2000 / 3-degree Gauss-Kruger zone 42",
"CGCS2000 / 3-degree Gauss-Kruger zone 43",
"CGCS2000 / 3-degree Gauss-Kruger zone 44",
"CGCS2000 / 3-degree Gauss-Kruger zone 45",
"CGCS2000 / 3-degree Gauss-Kruger CM 75E",
"CGCS2000 / 3-degree Gauss-Kruger CM 78E",
"CGCS2000 / 3-degree Gauss-Kruger CM 81E",
"CGCS2000 / 3-degree Gauss-Kruger CM 84E",
"CGCS2000 / 3-degree Gauss-Kruger CM 87E",
"CGCS2000 / 3-degree Gauss-Kruger CM 90E",
"CGCS2000 / 3-degree Gauss-Kruger CM 93E",
"CGCS2000 / 3-degree Gauss-Kruger CM 96E",
"CGCS2000 / 3-degree Gauss-Kruger CM 99E",
"CGCS2000 / 3-degree Gauss-Kruger CM 102E",
"CGCS2000 / 3-degree Gauss-Kruger CM 105E",
"CGCS2000 / 3-degree Gauss-Kruger CM 108E",
"CGCS2000 / 3-degree Gauss-Kruger CM 111E",
"CGCS2000 / 3-degree Gauss-Kruger CM 114E",
"CGCS2000 / 3-degree Gauss-Kruger CM 117E",
"CGCS2000 / 3-degree Gauss-Kruger CM 120E",
"CGCS2000 / 3-degree Gauss-Kruger CM 123E",
"CGCS2000 / 3-degree Gauss-Kruger CM 126E",
"CGCS2000 / 3-degree Gauss-Kruger CM 129E",
"CGCS2000 / 3-degree Gauss-Kruger CM 132E",
"CGCS2000 / 3-degree Gauss-Kruger CM 135E",
"New Beijing",
"RRAF 1991",
"RRAF 1991",
"RRAF 1991",
"RRAF 1991 / UTM zone 20N",
"New Beijing / Gauss-Kruger zone 13",
"New Beijing / Gauss-Kruger zone 14",
"New Beijing / Gauss-Kruger zone 15",
"New Beijing / Gauss-Kruger zone 16",
"New Beijing / Gauss-Kruger zone 17",
"New Beijing / Gauss-Kruger zone 18",
"New Beijing / Gauss-Kruger zone 19",
"New Beijing / Gauss-Kruger zone 20",
"New Beijing / Gauss-Kruger zone 21",
"New Beijing / Gauss-Kruger zone 22",
"New Beijing / Gauss-Kruger zone 23",
"New Beijing / Gauss-Kruger CM 75E",
"New Beijing / Gauss-Kruger CM 81E",
"New Beijing / Gauss-Kruger CM 87E",
"New Beijing / Gauss-Kruger CM 93E",
"New Beijing / Gauss-Kruger CM 99E",
"New Beijing / Gauss-Kruger CM 105E",
"New Beijing / Gauss-Kruger CM 111E",
"New Beijing / Gauss-Kruger CM 117E",
"New Beijing / Gauss-Kruger CM 123E",
"New Beijing / Gauss-Kruger CM 129E",
"New Beijing / Gauss-Kruger CM 135E",
"Anguilla 1957",
"Antigua 1943",
"Dominica 1945",
"Grenada 1953",
"Montserrat 1958",
"St. Kitts 1955",
"St. Lucia 1955",
"St. Vincent 1945",
"NAD27(76)",
"NAD27(CGQ77)",
"Xian 1980",
"Hong Kong 1980",
"JGD2000",
"Segara",
"QND95",
"Porto Santo",
"Selvagem Grande",
"NAD83(CSRS)",
"SAD69",
"SWEREF99",
"Point 58",
"Fort Marigot",
"Guadeloupe 1948",
"CSG67",
"RGFG95",
"Martinique 1938",
"Reunion 1947",
"RGR92",
"Tahiti 52",
"Tahaa 54",
"IGN72 Nuku Hiva",
"K0 1949",
"Combani 1950",
"IGN56 Lifou",
"IGN72 Grand Terre",
"ST87 Ouvea",
"Petrels 1972",
"Perroud 1950",
"Saint Pierre et Miquelon 1950",
"MOP78",
"RRAF 1991",
"IGN53 Mare",
"ST84 Ile des Pins",
"ST71 Belep",
"NEA74 Noumea",
"RGNC 1991",
"Grand Comoros",
"ETRS89 / UTM zone 32N (zE-N)",
"New Beijing / 3-degree Gauss-Kruger zone 25",
"New Beijing / 3-degree Gauss-Kruger zone 26",
"New Beijing / 3-degree Gauss-Kruger zone 27",
"New Beijing / 3-degree Gauss-Kruger zone 28",
"New Beijing / 3-degree Gauss-Kruger zone 29",
"Reykjavik 1900",
"Hjorsey 1955",
"ISN93",
"Helle 1954",
"LKS92",
"IGN72 Grande Terre",
"Porto Santo 1995",
"Azores Oriental 1995",
"Azores Central 1995",
"Lisbon 1890",
"IKBD-92",
"ED79",
"LKS94",
"IGM95",
"Voirol 1879",
"Chatham Islands 1971",
"Chatham Islands 1979",
"SIRGAS 2000",
"Guam 1963",
"Vientiane 1982",
"Lao 1993",
"Lao 1997",
"Jouik 1961",
"Nouakchott 1965",
"Mauritania 1999",
"Gulshan 303",
"PRS92",
"Gan 1970",
"Gandajika",
"MAGNA-SIRGAS",
"RGPF",
"Fatu Iva 72",
"IGN63 Hiva Oa",
"Tahiti 79",
"Moorea 87",
"Maupiti 83",
"Nakhl-e Ghanem",
"POSGAR 94",
"Katanga 1955",
"Kasai 1953",
"IGC 1962 6th Parallel South",
"IGN 1962 Kerguelen",
"Le Pouce 1934",
"IGN Astro 1960",
"IGCB 1955",
"Mauritania 1999",
"Mhast 1951",
"Mhast (onshore)",
"Mhast (offshore)",
"Egypt Gulf of Suez S-650 TL",
"Tern Island 1961",
"Cocos Islands 1965",
"Iwo Jima 1945",
"St. Helena 1971",
"Marcus Island 1952",
"Ascension Island 1958",
"Ayabelle Lighthouse",
"Bellevue",
"Camp Area Astro",
"Phoenix Islands 1966",
"Cape Canaveral",
"Solomon 1968",
"Easter Island 1967",
"Fiji 1986",
"Fiji 1956",
"South Georgia 1968",
"GCGD59",
"Diego Garcia 1969",
"Johnston Island 1961",
"SIGD61",
"Midway 1961",
"Pico de las Nieves 1984",
"Pitcairn 1967",
"Santo 1965",
"Viti Levu 1916",
"Marshall Islands 1960",
"Wake Island 1952",
"Tristan 1968",
"Kusaie 1951",
"Deception Island",
"Korea 2000",
"Hong Kong 1963",
"Hong Kong 1963(67)",
"PZ-90",
"FD54",
"GDM2000",
"Karbala 1979",
"Nahrwan 1934",
"RD/83",
"PD/83",
"GR96",
"Vanua Levu 1915",
"RGNC91-93",
"ST87 Ouvea",
"Kertau (RSO)",
"Viti Levu 1912",
"fk89",
"LGD2006",
"DGN95",
"VN-2000",
"SVY21",
"JAD2001",
"NAD83(NSRS2007)",
"WGS 66",
"HTRS96",
"BDA2000",
"Pitcairn 2006",
"RSRGD2000",
"Slovenia 1996",
"New Beijing / 3-degree Gauss-Kruger zone 30",
"New Beijing / 3-degree Gauss-Kruger zone 31",
"New Beijing / 3-degree Gauss-Kruger zone 32",
"New Beijing / 3-degree Gauss-Kruger zone 33",
"New Beijing / 3-degree Gauss-Kruger zone 34",
"New Beijing / 3-degree Gauss-Kruger zone 35",
"New Beijing / 3-degree Gauss-Kruger zone 36",
"New Beijing / 3-degree Gauss-Kruger zone 37",
"New Beijing / 3-degree Gauss-Kruger zone 38",
"New Beijing / 3-degree Gauss-Kruger zone 39",
"New Beijing / 3-degree Gauss-Kruger zone 40",
"New Beijing / 3-degree Gauss-Kruger zone 41",
"New Beijing / 3-degree Gauss-Kruger zone 42",
"New Beijing / 3-degree Gauss-Kruger zone 43",
"New Beijing / 3-degree Gauss-Kruger zone 44",
"New Beijing / 3-degree Gauss-Kruger zone 45",
"New Beijing / 3-degree Gauss-Kruger CM 75E",
"New Beijing / 3-degree Gauss-Kruger CM 78E",
"New Beijing / 3-degree Gauss-Kruger CM 81E",
"New Beijing / 3-degree Gauss-Kruger CM 84E",
"New Beijing / 3-degree Gauss-Kruger CM 87E",
"New Beijing / 3-degree Gauss-Kruger CM 90E",
"New Beijing / 3-degree Gauss-Kruger CM 93E",
"New Beijing / 3-degree Gauss-Kruger CM 96E",
"New Beijing / 3-degree Gauss-Kruger CM 99E",
"New Beijing / 3-degree Gauss-Kruger CM 102E",
"New Beijing / 3-degree Gauss-Kruger CM 105E",
"New Beijing / 3-degree Gauss-Kruger CM 108E",
"New Beijing / 3-degree Gauss-Kruger CM 111E",
"New Beijing / 3-degree Gauss-Kruger CM 114E",
"New Beijing / 3-degree Gauss-Kruger CM 117E",
"New Beijing / 3-degree Gauss-Kruger CM 120E",
"New Beijing / 3-degree Gauss-Kruger CM 123E",
"New Beijing / 3-degree Gauss-Kruger CM 126E",
"New Beijing / 3-degree Gauss-Kruger CM 129E",
"Bern 1898 (Bern)",
"Bogota 1975 (Bogota)",
"Lisbon (Lisbon)",
"Makassar (Jakarta)",
"MGI (Ferro)",
"Monte Mario (Rome)",
"NTF (Paris)",
"Padang (Jakarta)",
"Belge 1950 (Brussels)",
"Tananarive (Paris)",
"Voirol 1875 (Paris)",
"New Beijing / 3-degree Gauss-Kruger CM 132E",
"Batavia (Jakarta)",
"RT38 (Stockholm)",
"Greek (Athens)",
"Carthage (Paris)",
"NGO 1948 (Oslo)",
"S-JTSK (Ferro)",
"Nord Sahara 1959 (Paris)",
"Segara (Jakarta)",
"Voirol 1879 (Paris)",
"New Beijing / 3-degree Gauss-Kruger CM 135E",
"Sao Tome",
"Principe",
"WGS 84 / Cape Verde National",
"ETRS89 / LCC Germany (N-E)",
"ETRS89 / NTM zone 5",
"ETRS89 / NTM zone 6",
"ETRS89 / NTM zone 7",
"ETRS89 / NTM zone 8",
"ETRS89 / NTM zone 9",
"ETRS89 / NTM zone 10",
"ETRS89 / NTM zone 11",
"ETRS89 / NTM zone 12",
"ETRS89 / NTM zone 13",
"ETRS89 / NTM zone 14",
"ETRS89 / NTM zone 15",
"ETRS89 / NTM zone 16",
"ETRS89 / NTM zone 17",
"ETRS89 / NTM zone 18",
"ETRS89 / NTM zone 19",
"ETRS89 / NTM zone 20",
"ETRS89 / NTM zone 21",
"ETRS89 / NTM zone 22",
"ETRS89 / NTM zone 23",
"ETRS89 / NTM zone 24",
"ETRS89 / NTM zone 25",
"ETRS89 / NTM zone 26",
"ETRS89 / NTM zone 27",
"ETRS89 / NTM zone 28",
"ETRS89 / NTM zone 29",
"ETRS89 / NTM zone 30",
"Slovenia 1996",
"Slovenia 1996",
"RSRGD2000",
"RSRGD2000",
"BDA2000",
"BDA2000",
"HTRS96",
"HTRS96",
"WGS 66",
"WGS 66",
"NAD83(NSRS2007)",
"NAD83(NSRS2007)",
"JAD2001",
"JAD2001",
"ITRF2005",
"DGN95",
"DGN95",
"LGD2006",
"LGD2006",
"ATF (Paris)",
"NDG (Paris)",
"Madrid 1870 (Madrid)",
"Lisbon 1890 (Lisbon)",
"RGNC91-93",
"RGNC91-93",
"GR96",
"GR96",
"ITRF88",
"ITRF89",
"ITRF90",
"ITRF91",
"ITRF92",
"ITRF93",
"ITRF94",
"ITRF96",
"ITRF97",
"ITRF2000",
"GDM2000",
"GDM2000",
"PZ-90",
"PZ-90",
"Mauritania 1999",
"Mauritania 1999",
"Korea 2000",
"Korea 2000",
"POSGAR 94",
"POSGAR 94",
"Australian Antarctic",
"Australian Antarctic",
"CHTRF95",
"CHTRF95",
"EST97",
"EST97",
"ETRS89",
"ETRS89",
"GDA94",
"GDA94",
"Hartebeesthoek94",
"Hartebeesthoek94",
"IRENET95",
"IRENET95",
"ISN93",
"ISN93",
"JGD2000",
"JGD2000",
"LKS92",
"LKS92",
"LKS94",
"LKS94",
"Moznet",
"Moznet",
"NAD83(CSRS)",
"NAD83(CSRS)",
"NAD83(HARN)",
"NAD83(HARN)",
"NZGD2000",
"NZGD2000",
"POSGAR 98",
"POSGAR 98",
"REGVEN",
"REGVEN",
"RGF93",
"RGF93",
"RGFG95",
"RGFG95",
"RGNC 1991",
"RGNC 1991",
"RGR92",
"RGR92",
"RRAF 1991",
"RRAF 1991",
"SIRGAS 1995",
"SIRGAS 1995",
"SWEREF99",
"SWEREF99",
"WGS 84",
"WGS 84",
"Yemen NGN96",
"Yemen NGN96",
"IGM95",
"IGM95",
"WGS 72",
"WGS 72",
"WGS 72BE",
"WGS 72BE",
"SIRGAS 2000",
"SIRGAS 2000",
"Lao 1993",
"Lao 1993",
"Lao 1997",
"Lao 1997",
"PRS92",
"PRS92",
"MAGNA-SIRGAS",
"MAGNA-SIRGAS",
"RGPF",
"RGPF",
"PTRA08",
"PTRA08",
"PTRA08",
"PTRA08 / UTM zone 25N",
"PTRA08 / UTM zone 26N",
"PTRA08 / UTM zone 28N",
"Lisbon 1890 / Portugal Bonne New",
"Lisbon / Portuguese Grid New",
"WGS 84 / UPS North (E,N)",
"WGS 84 / UPS South (E,N)",
"ETRS89 / TM35FIN(N,E)",
"NAD27 / Conus Albers",
"NAD83 / Conus Albers",
"NAD83(HARN) / Conus Albers",
"NAD83(NSRS2007) / Conus Albers",
"ETRS89 / NTM zone 5",
"ETRS89 / NTM zone 6",
"ETRS89 / NTM zone 7",
"ETRS89 / NTM zone 8",
"ETRS89 / NTM zone 9",
"ETRS89 / NTM zone 10",
"ETRS89 / NTM zone 11",
"ETRS89 / NTM zone 12",
"ETRS89 / NTM zone 13",
"ETRS89 / NTM zone 14",
"ETRS89 / NTM zone 15",
"ETRS89 / NTM zone 16",
"ETRS89 / NTM zone 17",
"ETRS89 / NTM zone 18",
"ETRS89 / NTM zone 19",
"ETRS89 / NTM zone 20",
"ETRS89 / NTM zone 21",
"ETRS89 / NTM zone 22",
"ETRS89 / NTM zone 23",
"ETRS89 / NTM zone 24",
"ETRS89 / NTM zone 25",
"ETRS89 / NTM zone 26",
"ETRS89 / NTM zone 27",
"ETRS89 / NTM zone 28",
"ETRS89 / NTM zone 29",
"ETRS89 / NTM zone 30",
"Tokyo 1892",
"Korean 1985 / East Sea Belt",
"Korean 1985 / Central Belt Jeju",
"Tokyo 1892 / Korea West Belt",
"Tokyo 1892 / Korea Central Belt",
"Tokyo 1892 / Korea East Belt",
"Tokyo 1892 / Korea East Sea Belt",
"Korean 1985 / Modified West Belt",
"Korean 1985 / Modified Central Belt",
"Korean 1985 / Modified Central Belt Jeju",
"Korean 1985 / Modified East Belt",
"Korean 1985 / Modified East Sea Belt",
"Korean 1985 / Unified CS",
"Korea 2000 / Unified CS",
"Korea 2000 / West Belt",
"Korea 2000 / Central Belt",
"Korea 2000 / Central Belt Jeju",
"Korea 2000 / East Belt",
"Korea 2000 / East Sea Belt",
"Korea 2000 / West Belt 2010",
"Korea 2000 / Central Belt 2010",
"Korea 2000 / East Belt 2010",
"Korea 2000 / East Sea Belt 2010",
"Incheon height",
"Trieste height",
"Genoa height",
"S-JTSK (Ferro) / Krovak East North",
"WGS 84 / Gabon TM",
"S-JTSK/05 (Ferro) / Modified Krovak",
"S-JTSK/05 (Ferro) / Modified Krovak East North",
"S-JTSK/05",
"S-JTSK/05 (Ferro)",
"SLD99",
"Kandawala / Sri Lanka Grid",
"SLD99 / Sri Lanka Grid 1999",
"SLVD height",
"ETRS89 / LCC Germany (E-N)",
"GDBD2009",
"GDBD2009",
"GDBD2009",
"GDBD2009 / Brunei BRSO",
"TUREF",
"TUREF",
"TUREF",
"TUREF / TM27",
"TUREF / TM30",
"TUREF / TM33",
"TUREF / TM36",
"TUREF / TM39",
"TUREF / TM42",
"TUREF / TM45",
"DRUKREF 03",
"DRUKREF 03",
"DRUKREF 03",
"DRUKREF 03 / Bhutan National Grid",
"TUREF / 3-degree Gauss-Kruger zone 9",
"TUREF / 3-degree Gauss-Kruger zone 10",
"TUREF / 3-degree Gauss-Kruger zone 11",
"TUREF / 3-degree Gauss-Kruger zone 12",
"TUREF / 3-degree Gauss-Kruger zone 13",
"TUREF / 3-degree Gauss-Kruger zone 14",
"TUREF / 3-degree Gauss-Kruger zone 15",
"DRUKREF 03 / Bumthang TM",
"DRUKREF 03 / Chhukha TM",
"DRUKREF 03 / Dagana TM",
"DRUKREF 03 / Gasa TM",
"DRUKREF 03 / Ha TM",
"DRUKREF 03 / Lhuentse TM",
"DRUKREF 03 / Mongar TM",
"DRUKREF 03 / Paro TM",
"DRUKREF 03 / Pemagatshel TM",
"DRUKREF 03 / Punakha TM",
"DRUKREF 03 / Samdrup Jongkhar TM",
"DRUKREF 03 / Samtse TM",
"DRUKREF 03 / Sarpang TM",
"DRUKREF 03 / Thimphu TM",
"DRUKREF 03 / Trashigang TM",
"DRUKREF 03 / Trongsa TM",
"DRUKREF 03 / Tsirang TM",
"DRUKREF 03 / Wangdue Phodrang TM",
"DRUKREF 03 / Yangtse TM",
"DRUKREF 03 / Zhemgang TM",
"ETRS89 / Faroe TM",
"FVR09 height",
"ETRS89 / Faroe TM + FVR09 height",
"NAD83 / Teranet Ontario Lambert",
"NAD83(CSRS) / Teranet Ontario Lambert",
"ISN2004",
"ISN2004",
"ISN2004",
"ISN2004 / Lambert 2004",
"Segara (Jakarta) / NEIEZ",
"Batavia (Jakarta) / NEIEZ",
"Makassar (Jakarta) / NEIEZ",
"ITRF2008",
"Black Sea depth",
"Aratu / UTM zone 25S",
"POSGAR 2007",
"POSGAR 2007",
"POSGAR 2007",
"POSGAR 2007 / Argentina 1",
"POSGAR 2007 / Argentina 2",
"POSGAR 2007 / Argentina 3",
"POSGAR 2007 / Argentina 4",
"POSGAR 2007 / Argentina 5",
"POSGAR 2007 / Argentina 6",
"POSGAR 2007 / Argentina 7",
"MARGEN",
"MARGEN",
"MARGEN",
"MARGEN / UTM zone 20S",
"MARGEN / UTM zone 19S",
"MARGEN / UTM zone 21S",
"SIRGAS-Chile",
"SIRGAS-Chile",
"SIRGAS-Chile",
"SIRGAS-Chile / UTM zone 19S",
"SIRGAS-Chile / UTM zone 18S",
"CR05",
"CR05",
"CR05",
"CR05 / CRTM05",
"MACARIO SOLIS",
"Peru96",
"MACARIO SOLIS",
"MACARIO SOLIS",
"Peru96",
"Peru96",
"SIRGAS-ROU98",
"SIRGAS-ROU98",
"SIRGAS-ROU98",
"SIRGAS-ROU98 / UTM zone 21S",
"SIRGAS-ROU98 / UTM zone 22S",
"Peru96 / UTM zone 18S",
"Peru96 / UTM zone 17S",
"Peru96 / UTM zone 19S",
"SIRGAS_ES2007.8",
"SIRGAS_ES2007.8",
"SIRGAS_ES2007.8",
"SIRGAS 2000 / UTM zone 26S",
"Ocotepeque 1935",
"Ocotepeque 1935 / Costa Rica Norte",
"Ocotepeque 1935 / Costa Rica Sur",
"Ocotepeque 1935 / Guatemala Norte",
"Ocotepeque 1935 / Guatemala Sur",
"Ocotepeque 1935 / El Salvador Lambert",
"Ocotepeque 1935 / Nicaragua Norte",
"Ocotepeque 1935 / Nicaragua Sur",
"SAD69 / UTM zone 17N",
"Sibun Gorge 1922",
"Sibun Gorge 1922 / Colony Grid",
"Panama-Colon 1911",
"Panama-Colon 1911 / Panama Lambert",
"Panama-Colon 1911 / Panama Polyconic",
"RSRGD2000 / MSLC2000",
"RSRGD2000 / BCLC2000",
"RSRGD2000 / PCLC2000",
"RSRGD2000 / RSPS2000",
"RGAF09",
"RGAF09",
"RGAF09",
"RGAF09 / UTM zone 20N",
"NAD83 + NAVD88 height",
"NAD83(HARN) + NAVD88 height",
"NAD83(NSRS2007) + NAVD88 height",
"S-JTSK / Krovak",
"S-JTSK / Krovak East North",
"S-JTSK/05 / Modified Krovak",
"S-JTSK/05 / Modified Krovak East North",
"CI1971 / Chatham Islands Map Grid",
"CI1979 / Chatham Islands Map Grid",
"DHDN / 3-degree Gauss-Kruger zone 1",
"WGS 84 / Gabon TM 2011",
"Corrego Alegre 1961",
"SAD69(96)",
"SAD69(96) / Brazil Polyconic",
"SAD69(96) / UTM zone 21S",
"SAD69(96) / UTM zone 22S",
"SAD69(96) / UTM zone 23S",
"SAD69(96) / UTM zone 24S",
"SAD69(96) / UTM zone 25S",
"Corrego Alegre 1961 / UTM zone 21S",
"Corrego Alegre 1961 / UTM zone 22S",
"Corrego Alegre 1961 / UTM zone 23S",
"Corrego Alegre 1961 / UTM zone 24S",
"PNG94",
"PNG94",
"PNG94",
"PNG94 / PNGMG94 zone 54",
"PNG94 / PNGMG94 zone 55",
"PNG94 / PNGMG94 zone 56",
"ETRS89 / UTM zone 31N + DHHN92 height",
"ETRS89 / UTM zone 32N + DHHN92 height",
"ETRS89 / UTM zone 33N + DHHN92 height",
"UCS-2000",
"Ocotepeque 1935 / Guatemala Norte",
"UCS-2000",
"UCS-2000",
"UCS-2000 / Gauss-Kruger zone 4",
"UCS-2000 / Gauss-Kruger zone 5",
"UCS-2000 / Gauss-Kruger zone 6",
"UCS-2000 / Gauss-Kruger zone 7",
"UCS-2000 / Gauss-Kruger CM 21E",
"UCS-2000 / Gauss-Kruger CM 27E",
"UCS-2000 / Gauss-Kruger CM 33E",
"UCS-2000 / Gauss-Kruger CM 39E",
"UCS-2000 / 3-degree Gauss-Kruger zone 7",
"UCS-2000 / 3-degree Gauss-Kruger zone 8",
"UCS-2000 / 3-degree Gauss-Kruger zone 9",
"UCS-2000 / 3-degree Gauss-Kruger zone 10",
"UCS-2000 / 3-degree Gauss-Kruger zone 11",
"UCS-2000 / 3-degree Gauss-Kruger zone 12",
"UCS-2000 / 3-degree Gauss-Kruger zone 13",
"UCS-2000 / 3-degree Gauss-Kruger CM 21E",
"UCS-2000 / 3-degree Gauss-Kruger CM 24E",
"UCS-2000 / 3-degree Gauss-Kruger CM 27E",
"UCS-2000 / 3-degree Gauss-Kruger CM 30E",
"UCS-2000 / 3-degree Gauss-Kruger CM 33E",
"UCS-2000 / 3-degree Gauss-Kruger CM 36E",
"UCS-2000 / 3-degree Gauss-Kruger CM 39E",
"NAD27 / New Brunswick Stereographic (NAD27)",
"Sibun Gorge 1922 / Colony Grid",
"FEH2010",
"FEH2010",
"FEH2010",
"FEH2010 / Fehmarnbelt TM",
"FCSVR10 height",
"FEH2010 / Fehmarnbelt TM + FCSVR10 height",
"NGPF height",
"IGN 1966 height",
"Moorea SAU 1981 height",
"Raiatea SAU 2001 height",
"Maupiti SAU 2001 height",
"Huahine SAU 2001 height",
"Tahaa SAU 2001 height",
"Bora Bora SAU 2001 height",
"IGLD 1955 height",
"IGLD 1985 height",
"HVRS71 height",
"Caspian height",
"Baltic depth",
"RH2000 height",
"KOC WD depth (ft)",
"RH00 height",
"IGN 1988 LS height",
"IGN 1988 MG height",
"IGN 1992 LD height",
"IGN 1988 SB height",
"IGN 1988 SM height",
"EVRF2007 height",
"NAD27 / Michigan East",
"NAD27 / Michigan Old Central",
"NAD27 / Michigan West",
"ED50 / TM 6 NE",
"SWEREF99 + RH2000 height",
"Moznet / UTM zone 38S",
"Pulkovo 1942(58) / Gauss-Kruger zone 2 (E-N)",
"PTRA08 / LCC Europe",
"PTRA08 / LAEA Europe",
"REGCAN95 / LCC Europe",
"REGCAN95 / LAEA Europe",
"TUREF / LAEA Europe",
"TUREF / LCC Europe",
"ISN2004 / LAEA Europe",
"ISN2004 / LCC Europe",
"SIRGAS 2000 / Brazil Mercator",
"ED50 / SPBA LCC",
"RGR92 / UTM zone 39S",
"NAD83 / Vermont (ftUS)",
"ETRS89 / UTM zone 31N (zE-N)",
"ETRS89 / UTM zone 33N (zE-N)",
"ETRS89 / UTM zone 31N (N-zE)",
"ETRS89 / UTM zone 32N (N-zE)",
"ETRS89 / UTM zone 33N (N-zE)",
"NAD83(HARN) / Vermont (ftUS)",
"NAD83(NSRS2007) / Vermont (ftUS)",
"Monte Mario / TM Emilia-Romagna",
"Pulkovo 1942(58) / Gauss-Kruger zone 3 (E-N)",
"Pulkovo 1942(83) / Gauss-Kruger zone 2 (E-N)",
"Pulkovo 1942(83) / Gauss-Kruger zone 3 (E-N)",
"PD/83 / 3-degree Gauss-Kruger zone 3 (E-N)",
"PD/83 / 3-degree Gauss-Kruger zone 4 (E-N)",
"RD/83 / 3-degree Gauss-Kruger zone 4 (E-N)",
"RD/83 / 3-degree Gauss-Kruger zone 5 (E-N)",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 3 (E-N)",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 4 (E-N)",
"Pulkovo 1942(58) / 3-degree Gauss-Kruger zone 5 (E-N)",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 3 (E-N)",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 4 (E-N)",
"Pulkovo 1942(83) / 3-degree Gauss-Kruger zone 5 (E-N)",
"DHDN / 3-degree Gauss-Kruger zone 2 (E-N)",
"DHDN / 3-degree Gauss-Kruger zone 3 (E-N)",
"DHDN / 3-degree Gauss-Kruger zone 4 (E-N)",
"DHDN / 3-degree Gauss-Kruger zone 5 (E-N)",
"DHDN / 3-degree Gauss-Kruger zone 1 (E-N)",
"DB_REF",
"DB_REF / 3-degree Gauss-Kruger zone 2 (E-N)",
"DB_REF / 3-degree Gauss-Kruger zone 3 (E-N)",
"DB_REF / 3-degree Gauss-Kruger zone 4 (E-N)",
"DB_REF / 3-degree Gauss-Kruger zone 5 (E-N)",
"RGF93 / Lambert-93 + NGF-IGN69 height",
"RGF93 / Lambert-93 + NGF-IGN78 height",
"NZGD2000 / UTM zone 1S",
"ODN height",
"NGVD29 height",
"NAVD88 height",
"Yellow Sea",
"Baltic height",
"Caspian depth",
"NTF (Paris) / Lambert zone I + NGF-IGN69 height",
"NTF (Paris) / Lambert zone IV + NGF-IGN78 height",
"NAP height",
"Ostend height",
"AHD height",
"AHD (Tasmania) height",
"CGVD28 height",
"MSL height",
"MSL depth",
"Piraeus height",
"N60 height",
"RH70 height",
"NGF Lallemand height",
"NGF-IGN69 height",
"NGF-IGN78 height",
"Maputo height",
"JSLD69 height",
"PHD93 height",
"Fahud HD height",
"Ha Tien 1960 height",
"Hon Dau 1992 height",
"LN02 height",
"LHN95 height",
"EVRF2000 height",
"Malin Head height",
"Belfast height",
"DNN height",
"AIOC95 depth",
"Black Sea height",
"Yellow Sea 1956 height",
"Yellow Sea 1985 height",
"HKPD height",
"HKCD depth",
"ODN Orkney height",
"Fair Isle height",
"Lerwick height",
"Foula height",
"Sule Skerry height",
"North Rona height",
"Stornoway height",
"St Kilda height",
"Flannan Isles height",
"St Marys height",
"Douglas height",
"Fao height",
"Bandar Abbas height",
"NGNC height",
"Poolbeg height",
"NGG1977 height",
"Martinique 1987 height",
"Guadeloupe 1988 height",
"Reunion 1989 height",
"Auckland 1946 height",
"Bluff 1955 height",
"Dunedin 1958 height",
"Gisborne 1926 height",
"Lyttelton 1937 height",
"Moturiki 1953 height",
"Napier 1962 height",
"Nelson 1955 height",
"One Tree Point 1964 height",
"Tararu 1952 height",
"Taranaki 1970 height",
"Wellington 1953 height",
"Chatham Island 1959 height",
"Stewart Island 1977 height",
"EGM96 geoid height",
"NG-L height",
"Antalya height",
"NN54 height",
"Durres height",
"GHA height",
"NVN99 height",
"Cascais height",
"Constanta height",
"Alicante height",
"DHHN92 height",
"DHHN85 height",
"SNN76 height",
"Baltic 1982 height",
"EOMA 1980 height",
"Kuwait PWD height",
"KOC WD depth",
"KOC CD height",
"NGC 1948 height",
"Danger 1950 height",
"Mayotte 1950 height",
"Martinique 1955 height",
"Guadeloupe 1951 height",
"Lagos 1955 height",
"AIOC95 height",
"EGM84 geoid height",
"DVR90 height",
"Astra Minas Grid",
"Barcelona Grid B1",
"Barcelona Grid B2",
"Maturin Grid",
"EPSG seismic bin grid example A",
"EPSG seismic bin grid example B",
"EPSG local engineering grid example A",
"EPSG local engineering grid example B",
"Maracaibo Cross Grid M4",
"Maracaibo Cross Grid M5",
"La Rosa Grid",
"Mene Grande",
"El Cubo",
"Dabajuro",
"Tucupita",
"Santa Maria de Ipire",
"Barinas west base",
"Tombak LNG plant",
"[enter here name of (I = J+90°) bin grid]",
"EPSG topocentric example A",
"EPSG topocentric example B",
"EPSG vertical perspective example",
"AGD66 / ACT Standard Grid",
"DB_REF",
"Instantaneous Water Level height",
"DB_REF",
"Instantaneous Water Level depth",
"DB_REF / 3-degree Gauss-Kruger zone 2 (E-N) + DHHN92 height",
"DB_REF / 3-degree Gauss-Kruger zone 3 (E-N) + DHHN92 height",
"DB_REF / 3-degree Gauss-Kruger zone 4 (E-N) + DHHN92 height",
"DB_REF / 3-degree Gauss-Kruger zone 5 (E-N) + DHHN92 height",
"Yemen NGN96 / UTM zone 37N",
"Yemen NGN96 / UTM zone 40N",
"Peru96 / UTM zone 17S",
"WGS 84 / TM 12 SE",
"Ras Ghumays height",
"RGRDC 2005 / Congo TM zone 30",
"SWEREF99 TM + RH2000 height",
"SWEREF99 12 00 + RH2000 height",
"SWEREF99 13 30 + RH2000 height",
"SWEREF99 15 00 + RH2000 height",
"SWEREF99 16 30 + RH2000 height",
"SWEREF99 18 00 + RH2000 height",
"SWEREF99 14 15 + RH2000 height",
"SWEREF99 15 45 + RH2000 height",
"SWEREF99 17 15 + RH2000 height",
"SWEREF99 18 45 + RH2000 height",
"SWEREF99 20 15 + RH2000 height",
"SWEREF99 21 45 + RH2000 height",
"SWEREF99 23 15 + RH2000 height",
"SAD69(96) / UTM zone 22S",
"[enter here name of (I = J-90°) bin grid]",
"LAT depth",
"LLWLT depth",
"ISLW depth",
"MLLWS depth",
"MLWS depth",
"MLLW depth",
"MLW depth",
"MHW height",
"MHHW height",
"MHWS height",
"HHWLT height",
"HAT height",
"Low Water depth",
"High Water height",
"SAD69(96) / UTM zone 18S",
"SAD69(96) / UTM zone 19S",
"SAD69(96) / UTM zone 20S",
"Cadastre 1997 / UTM zone 38S",
"SIRGAS 2000 / Brazil Polyconic",
"TGD2005",
"TGD2005",
"TGD2005",
"TGD2005 / Tonga Map Grid",
"JAXA Snow Depth Polar Stereographic North",
"WGS 84 / EPSG Arctic Regional zone A1",
"WGS 84 / EPSG Arctic Regional zone A2",
"WGS 84 / EPSG Arctic Regional zone A3",
"WGS 84 / EPSG Arctic Regional zone A4",
"WGS 84 / EPSG Arctic Regional zone A5",
"WGS 84 / EPSG Arctic Regional zone B1",
"WGS 84 / EPSG Arctic Regional zone B2",
"WGS 84 / EPSG Arctic Regional zone B3",
"WGS 84 / EPSG Arctic Regional zone B4",
"WGS 84 / EPSG Arctic Regional zone B5",
"WGS 84 / EPSG Arctic Regional zone C1",
"WGS 84 / EPSG Arctic Regional zone C2",
"WGS 84 / EPSG Arctic Regional zone C3",
"WGS 84 / EPSG Arctic Regional zone C4",
"WGS 84 / EPSG Arctic Regional zone C5",
"WGS 84 / EPSG Alaska Polar Stereographic",
"WGS 84 / EPSG Canada Polar Stereographic",
"WGS 84 / EPSG Greenland Polar Stereographic",
"WGS 84 / EPSG Norway Polar Stereographic",
"WGS 84 / EPSG Russia Polar Stereographic",
"NN2000 height",
"ETRS89 + NN2000 height",
"ETRS89 / NTM zone 5 + NN2000 height",
"ETRS89 / NTM zone 6 + NN2000 height",
"ETRS89 / NTM zone 7 + NN2000 height",
"ETRS89 / NTM zone 8 + NN2000 height",
"ETRS89 / NTM zone 9 + NN2000 height",
"ETRS89 / NTM zone 10 + NN2000 height",
"ETRS89 / NTM zone 11 + NN2000 height",
"ETRS89 / NTM zone 12 + NN2000 height",
"ETRS89 / NTM zone 13 + NN2000 height",
"ETRS89 / NTM zone 14 + NN2000 height",
"ETRS89 / NTM zone 15 + NN2000 height",
"ETRS89 / NTM zone 16 + NN2000 height",
"ETRS89 / NTM zone 17 + NN2000 height",
"ETRS89 / NTM zone 18 + NN2000 height",
"ETRS89 / NTM zone 19 + NN2000 height",
"ETRS89 / NTM zone 20 + NN2000 height",
"ETRS89 / NTM zone 21 + NN2000 height",
"ETRS89 / NTM zone 22 + NN2000 height",
"ETRS89 / NTM zone 23 + NN2000 height",
"ETRS89 / NTM zone 24 + NN2000 height",
"ETRS89 / NTM zone 25 + NN2000 height",
"ETRS89 / NTM zone 26 + NN2000 height",
"ETRS89 / NTM zone 27 + NN2000 height",
"ETRS89 / NTM zone 28 + NN2000 height",
"ETRS89 / NTM zone 29 + NN2000 height",
"ETRS89 / NTM zone 30 + NN2000 height",
"ETRS89 / UTM zone 31 + NN2000 height",
"ETRS89 / UTM zone 32 + NN2000 height",
"ETRS89 / UTM zone 33 + NN2000 height",
"ETRS89 / UTM zone 34 + NN2000 height",
"ETRS89 / UTM zone 35 + NN2000 height",
"ETRS89 / UTM zone 36 + NN2000 height",
"GR96 / EPSG Arctic zone 1-25",
"GR96 / EPSG Arctic zone 2-18",
"GR96 / EPSG Arctic zone 2-20",
"GR96 / EPSG Arctic zone 3-29",
"GR96 / EPSG Arctic zone 3-31",
"GR96 / EPSG Arctic zone 3-33",
"GR96 / EPSG Arctic zone 4-20",
"GR96 / EPSG Arctic zone 4-22",
"GR96 / EPSG Arctic zone 4-24",
"GR96 / EPSG Arctic zone 5-41",
"GR96 / EPSG Arctic zone 5-43",
"GR96 / EPSG Arctic zone 5-45",
"GR96 / EPSG Arctic zone 6-26",
"GR96 / EPSG Arctic zone 6-28",
"GR96 / EPSG Arctic zone 6-30",
"GR96 / EPSG Arctic zone 7-11",
"GR96 / EPSG Arctic zone 7-13",
"GR96 / EPSG Arctic zone 8-20",
"GR96 / EPSG Arctic zone 8-22",
"ETRS89 / EPSG Arctic zone 2-22",
"ETRS89 / EPSG Arctic zone 3-11",
"ETRS89 / EPSG Arctic zone 4-26",
"ETRS89 / EPSG Arctic zone 4-28",
"ETRS89 / EPSG Arctic zone 5-11",
"ETRS89 / EPSG Arctic zone 5-13",
"WGS 84 / EPSG Arctic zone 2-24",
"WGS 84 / EPSG Arctic zone 2-26",
"WGS 84 / EPSG Arctic zone 3-13",
"WGS 84 / EPSG Arctic zone 3-15",
"WGS 84 / EPSG Arctic zone 3-17",
"WGS 84 / EPSG Arctic zone 3-19",
"WGS 84 / EPSG Arctic zone 4-30",
"WGS 84 / EPSG Arctic zone 4-32",
"WGS 84 / EPSG Arctic zone 4-34",
"WGS 84 / EPSG Arctic zone 4-36",
"WGS 84 / EPSG Arctic zone 4-38",
"WGS 84 / EPSG Arctic zone 4-40",
"WGS 84 / EPSG Arctic zone 5-15",
"WGS 84 / EPSG Arctic zone 5-17",
"WGS 84 / EPSG Arctic zone 5-19",
"WGS 84 / EPSG Arctic zone 5-21",
"WGS 84 / EPSG Arctic zone 5-23",
"WGS 84 / EPSG Arctic zone 5-25",
"WGS 84 / EPSG Arctic zone 5-27",
"NAD83(NSRS2007) / EPSG Arctic zone 5-29",
"NAD83(NSRS2007) / EPSG Arctic zone 5-31",
"NAD83(NSRS2007) / EPSG Arctic zone 6-14",
"NAD83(NSRS2007) / EPSG Arctic zone 6-16",
"NAD83(CSRS) / EPSG Arctic zone 1-23",
"NAD83(CSRS) / EPSG Arctic zone 2-14",
"NAD83(CSRS) / EPSG Arctic zone 2-16",
"NAD83(CSRS) / EPSG Arctic zone 3-25",
"NAD83(CSRS) / EPSG Arctic zone 3-27",
"NAD83(CSRS) / EPSG Arctic zone 3-29",
"NAD83(CSRS) / EPSG Arctic zone 4-14",
"NAD83(CSRS) / EPSG Arctic zone 4-16",
"NAD83(CSRS) / EPSG Arctic zone 4-18",
"NAD83(CSRS) / EPSG Arctic zone 5-33",
"NAD83(CSRS) / EPSG Arctic zone 5-35",
"NAD83(CSRS) / EPSG Arctic zone 5-37",
"NAD83(CSRS) / EPSG Arctic zone 5-39",
"NAD83(CSRS) / EPSG Arctic zone 6-18",
"NAD83(CSRS) / EPSG Arctic zone 6-20",
"NAD83(CSRS) / EPSG Arctic zone 6-22",
"NAD83(CSRS) / EPSG Arctic zone 6-24",
"WGS 84 / EPSG Arctic zone 1-27",
"WGS 84 / EPSG Arctic zone 1-29",
"WGS 84 / EPSG Arctic zone 1-31",
"WGS 84 / EPSG Arctic zone 1-21",
"WGS 84 / EPSG Arctic zone 2-28",
"WGS 84 / EPSG Arctic zone 2-10",
"WGS 84 / EPSG Arctic zone 2-12",
"WGS 84 / EPSG Arctic zone 3-21",
"WGS 84 / EPSG Arctic zone 3-23",
"WGS 84 / EPSG Arctic zone 4-12",
"ETRS89 / EPSG Arctic zone 5-47",
"Grand Cayman National Grid 1959",
"Sister Islands National Grid 1961",
"GCVD54 height",
"LCVD61 height",
"CBVD61 height",
"CIGD11",
"CIGD11",
"CIGD11",
"Cayman Islands National Grid 2011",
"ETRS89 + NN54 height",
"ETRS89 / NTM zone 5 + NN54 height",
"ETRS89 / NTM zone 6 + NN54 height",
"ETRS89 / NTM zone 7 + NN54 height",
"ETRS89 / NTM zone 8 + NN54 height",
"ETRS89 / NTM zone 9 + NN54 height",
"ETRS89 / NTM zone 10 + NN54 height",
"ETRS89 / NTM zone 11 + NN54 height",
"ETRS89 / NTM zone 12 + NN54 height",
"ETRS89 / NTM zone 13 + NN54 height",
"ETRS89 / NTM zone 14 + NN54 height",
"ETRS89 / NTM zone 15 + NN54 height",
"ETRS89 / NTM zone 16 + NN54 height",
"ETRS89 / NTM zone 17 + NN54 height",
"ETRS89 / NTM zone 18 + NN54 height",
"ETRS89 / NTM zone 19 + NN54 height",
"ETRS89 / NTM zone 20 + NN54 height",
"ETRS89 / NTM zone 21 + NN54 height",
"ETRS89 / NTM zone 22 + NN54 height",
"ETRS89 / NTM zone 23 + NN54 height",
"ETRS89 / NTM zone 24 + NN54 height",
"ETRS89 / NTM zone 25 + NN54 height",
"ETRS89 / NTM zone 26 + NN54 height",
"ETRS89 / NTM zone 27 + NN54 height",
"ETRS89 / NTM zone 28 + NN54 height",
"ETRS89 / NTM zone 29 + NN54 height",
"ETRS89 / NTM zone 30 + NN54 height",
"ETRS89 / UTM zone 31 + NN54 height",
"ETRS89 / UTM zone 32 + NN54 height",
"ETRS89 / UTM zone 33 + NN54 height",
"ETRS89 / UTM zone 34 + NN54 height",
"ETRS89 / UTM zone 35 + NN54 height",
"ETRS89 / UTM zone 36 + NN54 height",
"Cais da Pontinha - Funchal height",
"Cais da Vila - Porto Santo height",
"Cais das Velas height",
"Horta height",
"Cais da Madalena height",
"Santa Cruz da Graciosa height",
"Cais da Figueirinha - Angra do Heroismo height",
"Santa Cruz das Flores height",
"Cais da Vila do Porto height",
"Ponta Delgada height",
"Belge 1972 / Belgian Lambert 72 + Ostend height",
"NAD27 / Michigan North",
"NAD27 / Michigan Central",
"NAD27 / Michigan South",
"Macedonian State Coordinate System",
"Nepal 1981",
"SIRGAS 2000 / UTM zone 23N",
"SIRGAS 2000 / UTM zone 24N",
"MAGNA-SIRGAS / Arauca urban grid",
"MAGNA-SIRGAS / Armenia urban grid",
"MAGNA-SIRGAS / Barranquilla urban grid",
"MAGNA-SIRGAS / Bogota urban grid",
"MAGNA-SIRGAS / Bucaramanga urban grid",
"MAGNA-SIRGAS / Cali urban grid",
"MAGNA-SIRGAS / Cartagena urban grid",
"MAGNA-SIRGAS / Cucuta urban grid",
"MAGNA-SIRGAS / Florencia urban grid",
"MAGNA-SIRGAS / Ibague urban grid",
"MAGNA-SIRGAS / Inirida urban grid",
"MAGNA-SIRGAS / Leticia urban grid",
"MAGNA-SIRGAS / Manizales urban grid",
"MAGNA-SIRGAS / Medellin urban grid",
"MAGNA-SIRGAS / Mitu urban grid",
"MAGNA-SIRGAS / Mocoa urban grid",
"MAGNA-SIRGAS / Monteria urban grid",
"MAGNA-SIRGAS / Neiva urban grid",
"MAGNA-SIRGAS / Pasto urban grid",
"MAGNA-SIRGAS / Pereira urban grid",
"MAGNA-SIRGAS / Popayan urban grid",
"MAGNA-SIRGAS / Puerto Carreno urban grid",
"MAGNA-SIRGAS / Quibdo urban grid",
"MAGNA-SIRGAS / Riohacha urban grid",
"MAGNA-SIRGAS / San Andres urban grid",
"MAGNA-SIRGAS / San Jose del Guaviare urban grid",
"MAGNA-SIRGAS / Santa Marta urban grid",
"MAGNA-SIRGAS / Sucre urban grid",
"MAGNA-SIRGAS / Tunja urban grid",
"MAGNA-SIRGAS / Valledupar urban grid",
"MAGNA-SIRGAS / Villavicencio urban grid",
"MAGNA-SIRGAS / Yopal urban grid",
"NAD83(CORS96) / Puerto Rico and Virgin Is.",
"Macedonia State Coordinate System zone 7",
"NAD83(2011)",
"NAD83(2011)",
"NAD83(2011)",
"NAD83(PA11)",
"NAD83(PA11)",
"NAD83(PA11)",
"NAD83(MA11)",
"NAD83(MA11)",
"NAD83(MA11)",
"NAD83(2011) / UTM zone 59N",
"NAD83(2011) / UTM zone 60N",
"NAD83(2011) / UTM zone 1N",
"NAD83(2011) / UTM zone 2N",
"NAD83(2011) / UTM zone 3N",
"NAD83(2011) / UTM zone 4N",
"NAD83(2011) / UTM zone 5N",
"NAD83(2011) / UTM zone 6N",
"NAD83(2011) / UTM zone 7N",
"NAD83(2011) / UTM zone 8N",
"NAD83(2011) / UTM zone 9N",
"NAD83(2011) / UTM zone 10N",
"NAD83(2011) / UTM zone 11N",
"NAD83(2011) / UTM zone 12N",
"NAD83(2011) / UTM zone 13N",
"NAD83(2011) / UTM zone 14N",
"NAD83(2011) / UTM zone 15N",
"NAD83(2011) / UTM zone 16N",
"NAD83(2011) / UTM zone 17N",
"NAD83(2011) / UTM zone 18N",
"NAD83(2011) / UTM zone 19N",
"NAD83(2011) + NAVD88 height",
"NAD83(2011) / Conus Albers",
"NAD83(2011) / EPSG Arctic zone 5-29",
"NAD83(2011) / EPSG Arctic zone 5-31",
"NAD83(2011) / EPSG Arctic zone 6-14",
"NAD83(2011) / EPSG Arctic zone 6-16",
"NAD83(2011) / Alabama East",
"NAD83(2011) / Alabama West",
"NAVD88 depth",
"NAVD88 depth (ftUS)",
"NGVD29 depth",
"NAVD88 height (ftUS)",
"Mexico ITRF92 / LCC",
"Mexico ITRF2008",
"Mexico ITRF2008",
"Mexico ITRF2008",
"Mexico ITRF2008 / UTM zone 11N",
"Mexico ITRF2008 / UTM zone 12N",
"Mexico ITRF2008 / UTM zone 13N",
"Mexico ITRF2008 / UTM zone 14N",
"Mexico ITRF2008 / UTM zone 15N",
"Mexico ITRF2008 / UTM zone 16N",
"Mexico ITRF2008 / LCC",
"UCS-2000 / Ukraine TM zone 7",
"UCS-2000 / Ukraine TM zone 8",
"UCS-2000 / Ukraine TM zone 9",
"UCS-2000 / Ukraine TM zone 10",
"UCS-2000 / Ukraine TM zone 11",
"UCS-2000 / Ukraine TM zone 12",
"UCS-2000 / Ukraine TM zone 13",
"Cayman Islands National Grid 2011",
"NAD83(2011) / Alaska Albers",
"NAD83(2011) / Alaska zone 1",
"NAD83(2011) / Alaska zone 2",
"NAD83(2011) / Alaska zone 3",
"NAD83(2011) / Alaska zone 4",
"NAD83(2011) / Alaska zone 5",
"NAD83(2011) / Alaska zone 6",
"NAD83(2011) / Alaska zone 7",
"NAD83(2011) / Alaska zone 8",
"NAD83(2011) / Alaska zone 9",
"NAD83(2011) / Alaska zone 10",
"NAD83(2011) / Arizona Central",
"NAD83(2011) / Arizona Central (ft)",
"NAD83(2011) / Arizona East",
"NAD83(2011) / Arizona East (ft)",
"NAD83(2011) / Arizona West",
"NAD83(2011) / Arizona West (ft)",
"NAD83(2011) / Arkansas North",
"NAD83(2011) / Arkansas North (ftUS)",
"NAD83(2011) / Arkansas South",
"NAD83(2011) / Arkansas South (ftUS)",
"NAD83(2011) / California Albers",
"NAD83(2011) / California zone 1",
"NAD83(2011) / California zone 1 (ftUS)",
"NAD83(2011) / California zone 2",
"NAD83(2011) / California zone 2 (ftUS)",
"NAD83(2011) / California zone 3",
"NAD83(2011) / California zone 3 (ftUS)",
"NAD83(2011) / California zone 4",
"NAD83(2011) / California zone 4 (ftUS)",
"NAD83(2011) / California zone 5",
"NAD83(2011) / California zone 5 (ftUS)",
"NAD83(2011) / California zone 6",
"NAD83(2011) / California zone 6 (ftUS)",
"NAD83(2011) / Colorado Central",
"NAD83(2011) / Colorado Central (ftUS)",
"NAD83(2011) / Colorado North",
"NAD83(2011) / Colorado North (ftUS)",
"NAD83(2011) / Colorado South",
"NAD83(2011) / Colorado South (ftUS)",
"NAD83(2011) / Connecticut",
"NAD83(2011) / Connecticut (ftUS)",
"NAD83(2011) / Delaware",
"NAD83(2011) / Delaware (ftUS)",
"NAD83(2011) / Florida East",
"NAD83(2011) / Florida East (ftUS)",
"NAD83(2011) / Florida GDL Albers",
"NAD83(2011) / Florida North",
"NAD83(2011) / Florida North (ftUS)",
"NAD83(2011) / Florida West",
"NAD83(2011) / Florida West (ftUS)",
"NAD83(2011) / Georgia East",
"NAD83(2011) / Georgia East (ftUS)",
"NAD83(2011) / Georgia West",
"NAD83(2011) / Georgia West (ftUS)",
"NAD83(2011) / Idaho Central",
"NAD83(2011) / Idaho Central (ftUS)",
"NAD83(2011) / Idaho East",
"NAD83(2011) / Idaho East (ftUS)",
"NAD83(2011) / Idaho West",
"NAD83(2011) / Idaho West (ftUS)",
"NAD83(2011) / Illinois East",
"NAD83(2011) / Illinois East (ftUS)",
"NAD83(2011) / Illinois West",
"NAD83(2011) / Illinois West (ftUS)",
"NAD83(2011) / Indiana East",
"NAD83(2011) / Indiana East (ftUS)",
"NAD83(2011) / Indiana West",
"NAD83(2011) / Indiana West (ftUS)",
"NAD83(2011) / Iowa North",
"NAD83(2011) / Iowa North (ftUS)",
"NAD83(2011) / Iowa South",
"NAD83(2011) / Iowa South (ftUS)",
"NAD83(2011) / Kansas North",
"NAD83(2011) / Kansas North (ftUS)",
"NAD83(2011) / Kansas South",
"NAD83(2011) / Kansas South (ftUS)",
"NAD83(2011) / Kentucky North",
"NAD83(2011) / Kentucky North (ftUS)",
"NAD83(2011) / Kentucky Single Zone",
"NAD83(2011) / Kentucky Single Zone (ftUS)",
"NAD83(2011) / Kentucky South",
"NAD83(2011) / Kentucky South (ftUS)",
"NAD83(2011) / Louisiana North",
"NAD83(2011) / Louisiana North (ftUS)",
"NAD83(2011) / Louisiana South",
"NAD83(2011) / Louisiana South (ftUS)",
"NAD83(2011) / Maine CS2000 Central",
"NAD83(2011) / Maine CS2000 East",
"NAD83(2011) / Maine CS2000 West",
"NAD83(2011) / Maine East",
"NAD83(2011) / Maine East (ftUS)",
"NAD83(2011) / Maine West",
"NAD83(2011) / Maine West (ftUS)",
"NAD83(2011) / Maryland",
"NAD83(2011) / Maryland (ftUS)",
"NAD83(2011) / Massachusetts Island",
"NAD83(2011) / Massachusetts Island (ftUS)",
"NAD83(2011) / Massachusetts Mainland",
"NAD83(2011) / Massachusetts Mainland (ftUS)",
"NAD83(2011) / Michigan Central",
"NAD83(2011) / Michigan Central (ft)",
"NAD83(2011) / Michigan North",
"NAD83(2011) / Michigan North (ft)",
"NAD83(2011) / Michigan Oblique Mercator",
"NAD83(2011) / Michigan South",
"NAD83(2011) / Michigan South (ft)",
"NAD83(2011) / Minnesota Central",
"NAD83(2011) / Minnesota Central (ftUS)",
"NAD83(2011) / Minnesota North",
"NAD83(2011) / Minnesota North (ftUS)",
"NAD83(2011) / Minnesota South",
"NAD83(2011) / Minnesota South (ftUS)",
"NAD83(2011) / Mississippi East",
"NAD83(2011) / Mississippi East (ftUS)",
"NAD83(2011) / Mississippi TM",
"NAD83(2011) / Mississippi West",
"NAD83(2011) / Mississippi West (ftUS)",
"NAD83(2011) / Missouri Central",
"NAD83(2011) / Missouri East",
"NAD83(2011) / Missouri West",
"NAD83(2011) / Montana",
"NAD83(2011) / Montana (ft)",
"NAD83(2011) / Nebraska",
"NAD83(2011) / Nebraska (ftUS)",
"NAD83(2011) / Nevada Central",
"NAD83(2011) / Nevada Central (ftUS)",
"NAD83(2011) / Nevada East",
"NAD83(2011) / Nevada East (ftUS)",
"NAD83(2011) / Nevada West",
"NAD83(2011) / Nevada West (ftUS)",
"NAD83(2011) / New Hampshire",
"NAD83(2011) / New Hampshire (ftUS)",
"NAD83(2011) / New Jersey",
"NAD83(2011) / New Jersey (ftUS)",
"NAD83(2011) / New Mexico Central",
"NAD83(2011) / New Mexico Central (ftUS)",
"NAD83(2011) / New Mexico East",
"NAD83(2011) / New Mexico East (ftUS)",
"NAD83(2011) / New Mexico West",
"NAD83(2011) / New Mexico West (ftUS)",
"NAD83(2011) / New York Central",
"NAD83(2011) / New York Central (ftUS)",
"NAD83(2011) / New York East",
"NAD83(2011) / New York East (ftUS)",
"NAD83(2011) / New York Long Island",
"NAD83(2011) / New York Long Island (ftUS)",
"NAD83(2011) / New York West",
"NAD83(2011) / New York West (ftUS)",
"NAD83(2011) / North Carolina",
"NAD83(2011) / North Carolina (ftUS)",
"NAD83(2011) / North Dakota North",
"NAD83(2011) / North Dakota North (ft)",
"NAD83(2011) / North Dakota South",
"NAD83(2011) / North Dakota South (ft)",
"NAD83(2011) / Ohio North",
"NAD83(2011) / Ohio North (ftUS)",
"NAD83(2011) / Ohio South",
"NAD83(2011) / Ohio South (ftUS)",
"NAD83(2011) / Oklahoma North",
"NAD83(2011) / Oklahoma North (ftUS)",
"NAD83(2011) / Oklahoma South",
"NAD83(2011) / Oklahoma South (ftUS)",
"NAD83(2011) / Oregon LCC (m)",
"NAD83(2011) / Oregon GIC Lambert (ft)",
"NAD83(2011) / Oregon North",
"NAD83(2011) / Oregon North (ft)",
"NAD83(2011) / Oregon South",
"NAD83(2011) / Oregon South (ft)",
"NAD83(2011) / Pennsylvania North",
"NAD83(2011) / Pennsylvania North (ftUS)",
"NAD83(2011) / Pennsylvania South",
"NAD83(2011) / Pennsylvania South (ftUS)",
"NAD83(2011) / Puerto Rico and Virgin Is.",
"NAD83(2011) / Rhode Island",
"NAD83(2011) / Rhode Island (ftUS)",
"NAD83(2011) / South Carolina",
"NAD83(2011) / South Carolina (ft)",
"NAD83(2011) / South Dakota North",
"NAD83(2011) / South Dakota North (ftUS)",
"NAD83(2011) / South Dakota South",
"NAD83(2011) / South Dakota South (ftUS)",
"NAD83(2011) / Tennessee",
"NAD83(2011) / Tennessee (ftUS)",
"NAD83(2011) / Texas Central",
"NAD83(2011) / Texas Central (ftUS)",
"NAD83(2011) / Texas Centric Albers Equal Area",
"NAD83(2011) / Texas Centric Lambert Conformal",
"NAD83(2011) / Texas North",
"NAD83(2011) / Texas North (ftUS)",
"NAD83(2011) / Texas North Central",
"NAD83(2011) / Texas North Central (ftUS)",
"NAD83(2011) / Texas South",
"NAD83(2011) / Texas South (ftUS)",
"NAD83(2011) / Texas South Central",
"NAD83(2011) / Texas South Central (ftUS)",
"NAD83(2011) / Vermont",
"NAD83(2011) / Vermont (ftUS)",
"NAD83(2011) / Virginia Lambert",
"NAD83(2011) / Virginia North",
"NAD83(2011) / Virginia North (ftUS)",
"NAD83(2011) / Virginia South",
"NAD83(2011) / Virginia South (ftUS)",
"NAD83(2011) / Washington North",
"NAD83(2011) / Washington North (ftUS)",
"NAD83(2011) / Washington South",
"NAD83(2011) / Washington South (ftUS)",
"NAD83(2011) / West Virginia North",
"NAD83(2011) / West Virginia North (ftUS)",
"NAD83(2011) / West Virginia South",
"NAD83(2011) / West Virginia South (ftUS)",
"NAD83(2011) / Wisconsin Central",
"NAD83(2011) / Wisconsin Central (ftUS)",
"NAD83(2011) / Wisconsin North",
"NAD83(2011) / Wisconsin North (ftUS)",
"NAD83(2011) / Wisconsin South",
"NAD83(2011) / Wisconsin South (ftUS)",
"NAD83(2011) / Wisconsin Transverse Mercator",
"NAD83(2011) / Wyoming East",
"NAD83(2011) / Wyoming East (ftUS)",
"NAD83(2011) / Wyoming East Central",
"NAD83(2011) / Wyoming East Central (ftUS)",
"NAD83(2011) / Wyoming West",
"NAD83(2011) / Wyoming West (ftUS)",
"NAD83(2011) / Wyoming West Central",
"NAD83(2011) / Wyoming West Central (ftUS)",
"NAD83(2011) / Utah Central",
"NAD83(2011) / Utah North",
"NAD83(2011) / Utah South",
"NAD83(CSRS) / Quebec Lambert",
"NAD83 / Quebec Albers",
"NAD83(CSRS) / Quebec Albers",
"NAD83(2011) / Utah Central (ftUS)",
"NAD83(2011) / Utah North (ftUS)",
"NAD83(2011) / Utah South (ftUS)",
"NAD83(PA11) / Hawaii zone 1",
"NAD83(PA11) / Hawaii zone 2",
"NAD83(PA11) / Hawaii zone 3",
"NAD83(PA11) / Hawaii zone 4",
"NAD83(PA11) / Hawaii zone 5",
"NAD83(PA11) / Hawaii zone 3 (ftUS)",
"NAD83(PA11) / UTM zone 4N",
"NAD83(PA11) / UTM zone 5N",
"NAD83(PA11) / UTM zone 2S",
"NAD83(MA11) / Guam Map Grid",
"Tutuila 1962 height",
"Guam 1963 height",
"NMVD03 height",
"PRVD02 height",
"VIVD09 height",
"ASVD02 height",
"GUVD04 height",
"Karbala 1979 / Iraq National Grid",
"CGVD2013 height",
"NAD83(CSRS) + CGVD2013 height",
"NAD83(CSRS) / UTM zone 7N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 8N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 9N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 10N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 11N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 12N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 13N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 14N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 15N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 16N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 17N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 18N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 19N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 20N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 21N + CGVD2013 height",
"NAD83(CSRS) / UTM zone 22N + CGVD2013 height",
"JGD2011",
"JGD2011",
"JGD2011",
"JGD2011 / Japan Plane Rectangular CS I",
"JGD2011 / Japan Plane Rectangular CS II",
"JGD2011 / Japan Plane Rectangular CS III",
"JGD2011 / Japan Plane Rectangular CS IV",
"JGD2011 / Japan Plane Rectangular CS V",
"JGD2011 / Japan Plane Rectangular CS VI",
"JGD2011 / Japan Plane Rectangular CS VII",
"JGD2011 / Japan Plane Rectangular CS VIII",
"JGD2011 / Japan Plane Rectangular CS IX",
"JGD2011 / Japan Plane Rectangular CS X",
"JGD2011 / Japan Plane Rectangular CS XI",
"JGD2011 / Japan Plane Rectangular CS XII",
"JGD2011 / Japan Plane Rectangular CS XIII",
"JGD2011 / Japan Plane Rectangular CS XIV",
"JGD2011 / Japan Plane Rectangular CS XV",
"JGD2011 / Japan Plane Rectangular CS XVI",
"JGD2011 / Japan Plane Rectangular CS XVII",
"JGD2011 / Japan Plane Rectangular CS XVIII",
"JGD2011 / Japan Plane Rectangular CS XIX",
"JGD2011 / UTM zone 51N",
"JGD2011 / UTM zone 52N",
"JGD2011 / UTM zone 53N",
"JGD2011 / UTM zone 54N",
"JGD2011 / UTM zone 55N",
"JSLD72 height",
"JGD2000 (vertical) height",
"JGD2011 (vertical) height",
"JGD2000 + JGD2000 (vertical) height",
"JGD2011 + JGD2011 (vertical) height",
"Tokyo + JSLD72 height",
"WGS 84 / TM 60 SW",
"RDN2008",
"RDN2008",
"RDN2008",
"RDN2008 / TM32",
"RDN2008 / TM33",
"RDN2008 / TM34",
"Christmas Island Grid 1985",
"WGS 84 / CIG92",
"GDA94 / CIG94",
"WGS 84 / CKIG92",
"GDA94 / CKIG94",
"GDA94 / MGA zone 41",
"GDA94 / MGA zone 42",
"GDA94 / MGA zone 43",
"GDA94 / MGA zone 44",
"GDA94 / MGA zone 46",
"GDA94 / MGA zone 47",
"GDA94 / MGA zone 59",
"NAD83(CORS96)",
"NAD83(CORS96)",
"NAD83(CORS96)",
"NAD83(CORS96) / Oregon Baker zone (m)",
"NAD83(CORS96) / Oregon Baker zone (ft)",
"NAD83(2011) / Oregon Baker zone (m)",
"NAD83(2011) / Oregon Baker zone (ft)",
"NAD83(CORS96) / Oregon Bend-Klamath Falls zone (m)",
"NAD83(CORS96) / Oregon Bend-Klamath Falls zone (ft)",
"NAD83(2011) / Oregon Bend-Klamath Falls zone (m)",
"NAD83(2011) / Oregon Bend-Klamath Falls zone (ft)",
"NAD83(CORS96) / Oregon Bend-Redmond-Prineville zone (m)",
"NAD83(CORS96) / Oregon Bend-Redmond-Prineville zone (ft)",
"NAD83(2011) / Oregon Bend-Redmond-Prineville zone (m)",
"NAD83(2011) / Oregon Bend-Redmond-Prineville zone (ft)",
"NAD83(CORS96) / Oregon Bend-Burns zone (m)",
"NAD83(CORS96) / Oregon Bend-Burns zone (ft)",
"NAD83(2011) / Oregon Bend-Burns zone (m)",
"NAD83(2011) / Oregon Bend-Burns zone (ft)",
"NAD83(CORS96) / Oregon Canyonville-Grants Pass zone (m)",
"NAD83(CORS96) / Oregon Canyonville-Grants Pass zone (ft)",
"NAD83(2011) / Oregon Canyonville-Grants Pass zone (m)",
"NAD83(2011) / Oregon Canyonville-Grants Pass zone (ft)",
"NAD83(CORS96) / Oregon Columbia River East zone (m)",
"NAD83(CORS96) / Oregon Columbia River East zone (ft)",
"NAD83(2011) / Oregon Columbia River East zone (m)",
"NAD83(2011) / Oregon Columbia River East zone (ft)",
"NAD83(CORS96) / Oregon Columbia River West zone (m)",
"NAD83(CORS96) / Oregon Columbia River West zone (ft)",
"NAD83(2011) / Oregon Columbia River West zone (m)",
"NAD83(2011) / Oregon Columbia River West zone (ft)",
"NAD83(CORS96) / Oregon Cottage Grove-Canyonville zone (m)",
"NAD83(CORS96) / Oregon Cottage Grove-Canyonville zone (ft)",
"NAD83(2011) / Oregon Cottage Grove-Canyonville zone (m)",
"NAD83(2011) / Oregon Cottage Grove-Canyonville zone (ft)",
"NAD83(CORS96) / Oregon Dufur-Madras zone (m)",
"NAD83(CORS96) / Oregon Dufur-Madras zone (ft)",
"NAD83(2011) / Oregon Dufur-Madras zone (m)",
"NAD83(2011) / Oregon Dufur-Madras zone (ft)",
"NAD83(CORS96) / Oregon Eugene zone (m)",
"NAD83(CORS96) / Oregon Eugene zone (ft)",
"NAD83(2011) / Oregon Eugene zone (m)",
"NAD83(2011) / Oregon Eugene zone (ft)",
"NAD83(CORS96) / Oregon Grants Pass-Ashland zone (m)",
"NAD83(CORS96) / Oregon Grants Pass-Ashland zone (ft)",
"NAD83(2011) / Oregon Grants Pass-Ashland zone (m)",
"NAD83(2011) / Oregon Grants Pass-Ashland zone (ft)",
"NAD83(CORS96) / Oregon Gresham-Warm Springs zone (m)",
"NAD83(CORS96) / Oregon Gresham-Warm Springs zone (ft)",
"NAD83(2011) / Oregon Gresham-Warm Springs zone (m)",
"NAD83(2011) / Oregon Gresham-Warm Springs zone (ft)",
"NAD83(CORS96) / Oregon La Grande zone (m)",
"NAD83(CORS96) / Oregon La Grande zone (ft)",
"NAD83(2011) / Oregon La Grande zone (m)",
"NAD83(2011) / Oregon La Grande zone (ft)",
"NAD83(CORS96) / Oregon Ontario zone (m)",
"NAD83(CORS96) / Oregon Ontario zone (ft)",
"NAD83(2011) / Oregon Ontario zone (m)",
"NAD83(2011) / Oregon Ontario zone (ft)",
"NAD83(CORS96) / Oregon Coast zone (m)",
"NAD83(CORS96) / Oregon Coast zone (ft)",
"NAD83(2011) / Oregon Coast zone (m)",
"NAD83(2011) / Oregon Coast zone (ft)",
"NAD83(CORS96) / Oregon Pendleton zone (m)",
"NAD83(CORS96) / Oregon Pendleton zone (ft)",
"NAD83(2011) / Oregon Pendleton zone (m)",
"NAD83(2011) / Oregon Pendleton zone (ft)",
"NAD83(CORS96) / Oregon Pendleton-La Grande zone (m)",
"NAD83(CORS96) / Oregon Pendleton-La Grande zone (ft)",
"NAD83(2011) / Oregon Pendleton-La Grande zone (m)",
"NAD83(2011) / Oregon Pendleton-La Grande zone (ft)",
"NAD83(CORS96) / Oregon Portland zone (m)",
"NAD83(CORS96) / Oregon Portland zone (ft)",
"NAD83(2011) / Oregon Portland zone (m)",
"NAD83(2011) / Oregon Portland zone (ft)",
"NAD83(CORS96) / Oregon Salem zone (m)",
"NAD83(CORS96) / Oregon Salem zone (ft)",
"NAD83(2011) / Oregon Salem zone (m)",
"NAD83(2011) / Oregon Salem zone (ft)",
"NAD83(CORS96) / Oregon Santiam Pass zone (m)",
"NAD83(CORS96) / Oregon Santiam Pass zone (ft)",
"NAD83(2011) / Oregon Santiam Pass zone (m)",
"NAD83(2011) / Oregon Santiam Pass (ft)",
"NAD83(CORS96) / Oregon LCC (m)",
"NAD83(CORS96) / Oregon GIC Lambert (ft)",
"ETRS89 / Albania TM 2010",
"WGS 84 / Pseudo-Mercator +  EGM2008 geoid height",
"RDN2008 / Italy zone",
"RDN2008 / Zone 12",
"NAD83(2011) / Wisconsin Central",
"NAD83(2011) / Nebraska (ftUS)",
"Aden 1925",
"Bekaa Valley 1920",
"Bioko",
"NAD83(CORS96) / Oregon North",
"NAD83(CORS96) / Oregon North (ft)",
"NAD83(CORS96) / Oregon South",
"NAD83(CORS96) / Oregon South (ft)",
"South East Island 1943",
"WGS 84 / World Mercator +  EGM2008 geoid height",
"Gambia",
"South East Island 1943 / UTM zone 40N",
"SHD height",
"SVY21 + SHD height",
"NAD83 / Kansas LCC",
"NAD83 / Kansas LCC (ftUS)",
"NAD83(2011) / Kansas LCC",
"NAD83(2011) / Kansas LCC (ftUS)",
"SVY21 / Singapore TM + SHD height",
"WGS 84 / NSIDC EASE-Grid 2.0 North",
"WGS 84 / NSIDC EASE-Grid 2.0 South",
"WGS 84 / NSIDC EASE-Grid 2.0 Global",
"IGS08",
"VN-2000 / TM-3 zone 481",
"VN-2000 / TM-3 zone 482",
"VN-2000 / TM-3 zone 491",
"VN-2000 / TM-3 Da Nang zone",
"ETRS89 / Albania LCC 2010",
"NAD27 / Michigan North",
"IGD05",
"IGD05",
"IGD05",
"IG05 Intermediate CRS",
"IG05 Intermediate CRS",
"IG05 Intermediate CRS",
"Israeli Grid 05",
"IGD05/12",
"IGD05/12",
"IGD05/12",
"IG05/12 Intermediate CRS",
"IG05/12 Intermediate CRS",
"IG05/12 Intermediate CRS",
"Israeli Grid 05/12",
"NAD83(2011) / San Francisco CS13",
"NAD83(2011) / San Francisco CS13 (ftUS)",
"Nahrwan 1934 / UTM zone 37N",
"Nahrwan 1934 / UTM zone 38N",
"Nahrwan 1934 / UTM zone 39N",
"NTF (Paris) + NGF IGN69 height",
"NTF (Paris) / France II + NGF Lallemand",
"NTF (Paris) / France II + NGF IGN69",
"NTF (Paris) / France III + NGF IGN69",
"RT90 + RH70 height",
"OSGB 1936 / British National Grid + ODN height",
"NAD27 + NGVD29 height",
"NAD27 / Texas North + NGVD29 height",
"RD/NAP",
"ETRS89 + EVRF2000 height",
"PSHD93",
"NTF (Paris) / Lambert zone II + NGF Lallemand height",
"NTF (Paris) / Lambert zone II + NGF IGN69",
"NTF (Paris) / Lambert zone III + NGF IGN69",
"Tokyo + JSLD69 height",
"Amersfoort / RD New + NAP height",
"ETRS89 / UTM zone 32N + DVR90 height",
"ETRS89 / UTM zone 33N + DVR90 height",
"ETRS89 / Kp2000 Jutland + DVR90 height",
"ETRS89 / Kp2000 Zealand + DVR90 height",
"ETRS89 / Kp2000 Bornholm + DVR90 height",
"NTF (Paris) / Lambert zone II + NGF-IGN69 height",
"NTF (Paris) / Lambert zone III + NGF-IGN69 height",
"ETRS89 + EVRF2007 height",
"Pulkovo 1995 / Gauss-Kruger zone 4",
"Pulkovo 1995 / Gauss-Kruger zone 5",
"Pulkovo 1995 / Gauss-Kruger zone 6",
"Pulkovo 1995 / Gauss-Kruger zone 7",
"Pulkovo 1995 / Gauss-Kruger zone 8",
"Pulkovo 1995 / Gauss-Kruger zone 9",
"Pulkovo 1995 / Gauss-Kruger zone 10",
"Pulkovo 1995 / Gauss-Kruger zone 11",
"Pulkovo 1995 / Gauss-Kruger zone 12",
"Pulkovo 1995 / Gauss-Kruger zone 13",
"Pulkovo 1995 / Gauss-Kruger zone 14",
"Pulkovo 1995 / Gauss-Kruger zone 15",
"Pulkovo 1995 / Gauss-Kruger zone 16",
"Pulkovo 1995 / Gauss-Kruger zone 17",
"Pulkovo 1995 / Gauss-Kruger zone 18",
"Pulkovo 1995 / Gauss-Kruger zone 19",
"Pulkovo 1995 / Gauss-Kruger zone 20",
"Pulkovo 1995 / Gauss-Kruger zone 21",
"Pulkovo 1995 / Gauss-Kruger zone 22",
"Pulkovo 1995 / Gauss-Kruger zone 23",
"Pulkovo 1995 / Gauss-Kruger zone 24",
"Pulkovo 1995 / Gauss-Kruger zone 25",
"Pulkovo 1995 / Gauss-Kruger zone 26",
"Pulkovo 1995 / Gauss-Kruger zone 27",
"Pulkovo 1995 / Gauss-Kruger zone 28",
"Pulkovo 1995 / Gauss-Kruger zone 29",
"Pulkovo 1995 / Gauss-Kruger zone 30",
"Pulkovo 1995 / Gauss-Kruger zone 31",
"Pulkovo 1995 / Gauss-Kruger zone 32",
"Pulkovo 1995 / Gauss-Kruger 4N",
"Pulkovo 1995 / Gauss-Kruger 5N",
"Pulkovo 1995 / Gauss-Kruger 6N",
"Pulkovo 1995 / Gauss-Kruger 7N",
"Pulkovo 1995 / Gauss-Kruger 8N",
"Pulkovo 1995 / Gauss-Kruger 9N",
"Pulkovo 1995 / Gauss-Kruger 10N",
"Pulkovo 1995 / Gauss-Kruger 11N",
"Pulkovo 1995 / Gauss-Kruger 12N",
"Pulkovo 1995 / Gauss-Kruger 13N",
"Pulkovo 1995 / Gauss-Kruger 14N",
"Pulkovo 1995 / Gauss-Kruger 15N",
"Pulkovo 1995 / Gauss-Kruger 16N",
"Pulkovo 1995 / Gauss-Kruger 17N",
"Pulkovo 1995 / Gauss-Kruger 18N",
"Pulkovo 1995 / Gauss-Kruger 19N",
"Pulkovo 1995 / Gauss-Kruger 20N",
"Pulkovo 1995 / Gauss-Kruger 21N",
"Pulkovo 1995 / Gauss-Kruger 22N",
"Pulkovo 1995 / Gauss-Kruger 23N",
"Pulkovo 1995 / Gauss-Kruger 24N",
"Pulkovo 1995 / Gauss-Kruger 25N",
"Pulkovo 1995 / Gauss-Kruger 26N",
"Pulkovo 1995 / Gauss-Kruger 27N",
"Pulkovo 1995 / Gauss-Kruger 28N",
"Pulkovo 1995 / Gauss-Kruger 29N",
"Pulkovo 1995 / Gauss-Kruger 30N",
"Pulkovo 1995 / Gauss-Kruger 31N",
"Pulkovo 1995 / Gauss-Kruger 32N",
"Adindan / UTM zone 35N",
"Adindan / UTM zone 36N",
"Adindan / UTM zone 37N",
"Adindan / UTM zone 38N",
"AGD66 / AMG zone 48",
"AGD66 / AMG zone 49",
"AGD66 / AMG zone 50",
"AGD66 / AMG zone 51",
"AGD66 / AMG zone 52",
"AGD66 / AMG zone 53",
"AGD66 / AMG zone 54",
"AGD66 / AMG zone 55",
"AGD66 / AMG zone 56",
"AGD66 / AMG zone 57",
"AGD66 / AMG zone 58",
"AGD84 / AMG zone 48",
"AGD84 / AMG zone 49",
"AGD84 / AMG zone 50",
"AGD84 / AMG zone 51",
"AGD84 / AMG zone 52",
"AGD84 / AMG zone 53",
"AGD84 / AMG zone 54",
"AGD84 / AMG zone 55",
"AGD84 / AMG zone 56",
"AGD84 / AMG zone 57",
"AGD84 / AMG zone 58",
"Ain el Abd / UTM zone 36N",
"Ain el Abd / UTM zone 37N",
"Ain el Abd / UTM zone 38N",
"Ain el Abd / UTM zone 39N",
"Ain el Abd / UTM zone 40N",
"Ain el Abd / Bahrain Grid",
"Afgooye / UTM zone 38N",
"Afgooye / UTM zone 39N",
"Lisbon (Lisbon) / Portuguese National Grid",
"Lisbon (Lisbon) / Portuguese Grid",
"Aratu / UTM zone 22S",
"Aratu / UTM zone 23S",
"Aratu / UTM zone 24S",
"Arc 1950 / UTM zone 34S",
"Arc 1950 / UTM zone 35S",
"Arc 1950 / UTM zone 36S",
"Arc 1960 / UTM zone 35S",
"Arc 1960 / UTM zone 36S",
"Arc 1960 / UTM zone 37S",
"Arc 1960 / UTM zone 35N",
"Arc 1960 / UTM zone 36N",
"Arc 1960 / UTM zone 37N",
"Batavia (Jakarta) / NEIEZ",
"Batavia / UTM zone 48S",
"Batavia / UTM zone 49S",
"Batavia / UTM zone 50S",
"Barbados 1938 / British West Indies Grid",
"Barbados 1938 / Barbados National Grid",
"Beijing 1954 / Gauss-Kruger zone 13",
"Beijing 1954 / Gauss-Kruger zone 14",
"Beijing 1954 / Gauss-Kruger zone 15",
"Beijing 1954 / Gauss-Kruger zone 16",
"Beijing 1954 / Gauss-Kruger zone 17",
"Beijing 1954 / Gauss-Kruger zone 18",
"Beijing 1954 / Gauss-Kruger zone 19",
"Beijing 1954 / Gauss-Kruger zone 20",
"Beijing 1954 / Gauss-Kruger zone 21",
"Beijing 1954 / Gauss-Kruger zone 22",
"Beijing 1954 / Gauss-Kruger zone 23",
"Beijing 1954 / Gauss-Kruger CM 75E",
"Beijing 1954 / Gauss-Kruger CM 81E",
"Beijing 1954 / Gauss-Kruger CM 87E",
"Beijing 1954 / Gauss-Kruger CM 93E",
"Beijing 1954 / Gauss-Kruger CM 99E",
"Beijing 1954 / Gauss-Kruger CM 105E",
"Beijing 1954 / Gauss-Kruger CM 111E",
"Beijing 1954 / Gauss-Kruger CM 117E",
"Beijing 1954 / Gauss-Kruger CM 123E",
"Beijing 1954 / Gauss-Kruger CM 129E",
"Beijing 1954 / Gauss-Kruger CM 135E",
"Beijing 1954 / Gauss-Kruger 13N",
"Beijing 1954 / Gauss-Kruger 14N",
"Beijing 1954 / Gauss-Kruger 15N",
"Beijing 1954 / Gauss-Kruger 16N",
"Beijing 1954 / Gauss-Kruger 17N",
"Beijing 1954 / Gauss-Kruger 18N",
"Beijing 1954 / Gauss-Kruger 19N",
"Beijing 1954 / Gauss-Kruger 20N",
"Beijing 1954 / Gauss-Kruger 21N",
"Beijing 1954 / Gauss-Kruger 22N",
"Beijing 1954 / Gauss-Kruger 23N",
"Belge 1950 (Brussels) / Belge Lambert 50",
"Bern 1898 (Bern) / LV03C",
"CH1903 / LV03",
"CH1903 / LV03C-G",
"Bogota 1975 / UTM zone 17N",
"Bogota 1975 / UTM zone 18N",
"Bogota 1975 / Colombia West zone",
"Bogota 1975 / Colombia Bogota zone",
"Bogota 1975 / Colombia East Central zone",
"Bogota 1975 / Colombia East",
"Bogota 1975 / Colombia West zone",
"Bogota 1975 / Colombia Bogota zone",
"Bogota 1975 / Colombia East Central zone",
"Bogota 1975 / Colombia East",
"Camacupa / UTM zone 32S",
"Camacupa / UTM zone 33S",
"Camacupa / TM 11.30 SE",
"Camacupa / TM 12 SE",
"POSGAR 98 / Argentina 1",
"POSGAR 98 / Argentina 2",
"POSGAR 98 / Argentina 3",
"POSGAR 98 / Argentina 4",
"POSGAR 98 / Argentina 5",
"POSGAR 98 / Argentina 6",
"POSGAR 98 / Argentina 7",
"POSGAR 94 / Argentina 1",
"POSGAR 94 / Argentina 2",
"POSGAR 94 / Argentina 3",
"POSGAR 94 / Argentina 4",
"POSGAR 94 / Argentina 5",
"POSGAR 94 / Argentina 6",
"POSGAR 94 / Argentina 7",
"Campo Inchauspe / Argentina 1",
"Campo Inchauspe / Argentina 2",
"Campo Inchauspe / Argentina 3",
"Campo Inchauspe / Argentina 4",
"Campo Inchauspe / Argentina 5",
"Campo Inchauspe / Argentina 6",
"Campo Inchauspe / Argentina 7",
"Cape / UTM zone 34S",
"Cape / UTM zone 35S",
"Cape / UTM zone 36S",
"Cape / Lo15",
"Cape / Lo17",
"Cape / Lo19",
"Cape / Lo21",
"Cape / Lo23",
"Cape / Lo25",
"Cape / Lo27",
"Cape / Lo29",
"Cape / Lo31",
"Cape / Lo33",
"Carthage (Paris) / Tunisia Mining Grid",
"Carthage / UTM zone 32N",
"Carthage / Nord Tunisie",
"Carthage / Sud Tunisie",
"Corrego Alegre 1970-72 / UTM zone 21S",
"Corrego Alegre 1970-72 / UTM zone 22S",
"Corrego Alegre 1970-72 / UTM zone 23S",
"Corrego Alegre 1970-72 / UTM zone 24S",
"Corrego Alegre 1970-72 / UTM zone 25S",
"Deir ez Zor / Levant Zone",
"Deir ez Zor / Syria Lambert",
"Deir ez Zor / Levant Stereographic",
"Douala / UTM zone 32N",
"Egypt 1907 / Blue Belt",
"Egypt 1907 / Red Belt",
"Egypt 1907 / Purple Belt",
"Egypt 1907 / Extended Purple Belt",
"ED50 / UTM zone 28N",
"ED50 / UTM zone 29N",
"ED50 / UTM zone 30N",
"ED50 / UTM zone 31N",
"ED50 / UTM zone 32N",
"ED50 / UTM zone 33N",
"ED50 / UTM zone 34N",
"ED50 / UTM zone 35N",
"ED50 / UTM zone 36N",
"ED50 / UTM zone 37N",
"ED50 / UTM zone 38N",
"ED50 / TM 0 N",
"ED50 / TM 5 NE",
"Fahud / UTM zone 39N",
"Fahud / UTM zone 40N",
"Garoua / UTM zone 33N",
"HD72 / EOV",
"DGN95 / Indonesia TM-3 zone 46.2",
"DGN95 / Indonesia TM-3 zone 47.1",
"DGN95 / Indonesia TM-3 zone 47.2",
"DGN95 / Indonesia TM-3 zone 48.1",
"DGN95 / Indonesia TM-3 zone 48.2",
"DGN95 / Indonesia TM-3 zone 49.1",
"DGN95 / Indonesia TM-3 zone 49.2",
"DGN95 / Indonesia TM-3 zone 50.1",
"DGN95 / Indonesia TM-3 zone 50.2",
"DGN95 / Indonesia TM-3 zone 51.1",
"DGN95 / Indonesia TM-3 zone 51.2",
"DGN95 / Indonesia TM-3 zone 52.1",
"DGN95 / Indonesia TM-3 zone 52.2",
"DGN95 / Indonesia TM-3 zone 53.1",
"DGN95 / Indonesia TM-3 zone 53.2",
"DGN95 / Indonesia TM-3 zone 54.1",
"ID74 / UTM zone 46N",
"ID74 / UTM zone 47N",
"ID74 / UTM zone 48N",
"ID74 / UTM zone 49N",
"ID74 / UTM zone 50N",
"ID74 / UTM zone 51N",
"ID74 / UTM zone 52N",
"ID74 / UTM zone 53N",
"DGN95 / UTM zone 46N",
"DGN95 / UTM zone 47N",
"DGN95 / UTM zone 48N",
"DGN95 / UTM zone 49N",
"DGN95 / UTM zone 50N",
"DGN95 / UTM zone 51N",
"DGN95 / UTM zone 52N",
"DGN95 / UTM zone 47S",
"DGN95 / UTM zone 48S",
"DGN95 / UTM zone 49S",
"DGN95 / UTM zone 50S",
"DGN95 / UTM zone 51S",
"DGN95 / UTM zone 52S",
"DGN95 / UTM zone 53S",
"DGN95 / UTM zone 54S",
"ID74 / UTM zone 46S",
"ID74 / UTM zone 47S",
"ID74 / UTM zone 48S",
"ID74 / UTM zone 49S",
"ID74 / UTM zone 50S",
"ID74 / UTM zone 51S",
"ID74 / UTM zone 52S",
"ID74 / UTM zone 53S",
"ID74 / UTM zone 54S",
"Indian 1954 / UTM zone 46N",
"Indian 1954 / UTM zone 47N",
"Indian 1954 / UTM zone 48N",
"Indian 1975 / UTM zone 47N",
"Indian 1975 / UTM zone 48N",
"Jamaica 1875 / Jamaica (Old Grid)",
"JAD69 / Jamaica National Grid",
"Kalianpur 1937 / UTM zone 45N",
"Kalianpur 1937 / UTM zone 46N",
"Kalianpur 1962 / UTM zone 41N",
"Kalianpur 1962 / UTM zone 42N",
"Kalianpur 1962 / UTM zone 43N",
"Kalianpur 1975 / UTM zone 42N",
"Kalianpur 1975 / UTM zone 43N",
"Kalianpur 1975 / UTM zone 44N",
"Kalianpur 1975 / UTM zone 45N",
"Kalianpur 1975 / UTM zone 46N",
"Kalianpur 1975 / UTM zone 47N",
"Kalianpur 1880 / India zone 0",
"Kalianpur 1880 / India zone I",
"Kalianpur 1880 / India zone IIa",
"Kalianpur 1880 / India zone IIIa",
"Kalianpur 1880 / India zone IVa",
"Kalianpur 1937 / India zone IIb",
"Kalianpur 1962 / India zone I",
"Kalianpur 1962 / India zone IIa",
"Kalianpur 1975 / India zone I",
"Kalianpur 1975 / India zone IIa",
"Kalianpur 1975 / India zone IIb",
"Kalianpur 1975 / India zone IIIa",
"Kalianpur 1880 / India zone IIb",
"Kalianpur 1975 / India zone IVa",
"Kertau 1968 / Singapore Grid",
"Kertau 1968 / UTM zone 47N",
"Kertau 1968 / UTM zone 48N",
"Kertau / R.S.O. Malaya (ch)",
"KOC Lambert",
"La Canoa / UTM zone 18N",
"La Canoa / UTM zone 19N",
"La Canoa / UTM zone 20N",
"PSAD56 / UTM zone 17N",
"PSAD56 / UTM zone 18N",
"PSAD56 / UTM zone 19N",
"PSAD56 / UTM zone 20N",
"PSAD56 / UTM zone 21N",
"PSAD56 / UTM zone 17S",
"PSAD56 / UTM zone 18S",
"PSAD56 / UTM zone 19S",
"PSAD56 / UTM zone 20S",
"PSAD56 / UTM zone 21S",
"PSAD56 / UTM zone 22S",
"PSAD56 / Peru west zone",
"PSAD56 / Peru central zone",
"PSAD56 / Peru east zone",
"Leigon / Ghana Metre Grid",
"Lome / UTM zone 31N",
"Luzon 1911 / Philippines zone I",
"Luzon 1911 / Philippines zone II",
"Luzon 1911 / Philippines zone III",
"Luzon 1911 / Philippines zone IV",
"Luzon 1911 / Philippines zone V",
"Makassar (Jakarta) / NEIEZ",
"ETRS89 / UTM zone 28N",
"ETRS89 / UTM zone 29N",
"ETRS89 / UTM zone 30N",
"ETRS89 / UTM zone 31N",
"ETRS89 / UTM zone 32N",
"ETRS89 / UTM zone 33N",
"ETRS89 / UTM zone 34N",
"ETRS89 / UTM zone 35N",
"ETRS89 / UTM zone 36N",
"ETRS89 / UTM zone 37N",
"ETRS89 / UTM zone 38N",
"ETRS89 / TM Baltic93",
"Malongo 1987 / UTM zone 32S",
"Merchich / Nord Maroc",
"Merchich / Sud Maroc",
"Merchich / Sahara",
"Merchich / Sahara Nord",
"Merchich / Sahara Sud",
"Massawa / UTM zone 37N",
"Minna / UTM zone 31N",
"Minna / UTM zone 32N",
"Minna / Nigeria West Belt",
"Minna / Nigeria Mid Belt",
"Minna / Nigeria East Belt",
"Mhast / UTM zone 32S",
"Monte Mario (Rome) / Italy zone 1",
"Monte Mario (Rome) / Italy zone 2",
"M'poraloko / UTM zone 32N",
"M'poraloko / UTM zone 32S",
"NAD27 / UTM zone 1N",
"NAD27 / UTM zone 2N",
"NAD27 / UTM zone 3N",
"NAD27 / UTM zone 4N",
"NAD27 / UTM zone 5N",
"NAD27 / UTM zone 6N",
"NAD27 / UTM zone 7N",
"NAD27 / UTM zone 8N",
"NAD27 / UTM zone 9N",
"NAD27 / UTM zone 10N",
"NAD27 / UTM zone 11N",
"NAD27 / UTM zone 12N",
"NAD27 / UTM zone 13N",
"NAD27 / UTM zone 14N",
"NAD27 / UTM zone 15N",
"NAD27 / UTM zone 16N",
"NAD27 / UTM zone 17N",
"NAD27 / UTM zone 18N",
"NAD27 / UTM zone 19N",
"NAD27 / UTM zone 20N",
"NAD27 / UTM zone 21N",
"NAD27 / UTM zone 22N",
"NAD27 / Alabama East",
"NAD27 / Alabama West",
"NAD27 / Alaska zone 1",
"NAD27 / Alaska zone 2",
"NAD27 / Alaska zone 3",
"NAD27 / Alaska zone 4",
"NAD27 / Alaska zone 5",
"NAD27 / Alaska zone 6",
"NAD27 / Alaska zone 7",
"NAD27 / Alaska zone 8",
"NAD27 / Alaska zone 9",
"NAD27 / Alaska zone 10",
"NAD27 / California zone I",
"NAD27 / California zone II",
"NAD27 / California zone III",
"NAD27 / California zone IV",
"NAD27 / California zone V",
"NAD27 / California zone VI",
"NAD27 / California zone VII",
"NAD27 / Arizona East",
"NAD27 / Arizona Central",
"NAD27 / Arizona West",
"NAD27 / Arkansas North",
"NAD27 / Arkansas South",
"NAD27 / Colorado North",
"NAD27 / Colorado Central",
"NAD27 / Colorado South",
"NAD27 / Connecticut",
"NAD27 / Delaware",
"NAD27 / Florida East",
"NAD27 / Florida West",
"NAD27 / Florida North",
"NAD27 / Georgia East",
"NAD27 / Georgia West",
"NAD27 / Idaho East",
"NAD27 / Idaho Central",
"NAD27 / Idaho West",
"NAD27 / Illinois East",
"NAD27 / Illinois West",
"NAD27 / Indiana East",
"NAD27 / Indiana West",
"NAD27 / Iowa North",
"NAD27 / Iowa South",
"NAD27 / Kansas North",
"NAD27 / Kansas South",
"NAD27 / Kentucky North",
"NAD27 / Kentucky South",
"NAD27 / Louisiana North",
"NAD27 / Louisiana South",
"NAD27 / Maine East",
"NAD27 / Maine West",
"NAD27 / Maryland",
"NAD27 / Massachusetts Mainland",
"NAD27 / Massachusetts Island",
"NAD27 / Minnesota North",
"NAD27 / Minnesota Central",
"NAD27 / Minnesota South",
"NAD27 / Mississippi East",
"NAD27 / Mississippi West",
"NAD27 / Missouri East",
"NAD27 / Missouri Central",
"NAD27 / Missouri West",
"NAD27 / California zone VII",
"NAD Michigan / Michigan East",
"NAD Michigan / Michigan Old Central",
"NAD Michigan / Michigan West",
"NAD Michigan / Michigan North",
"NAD Michigan / Michigan Central",
"NAD Michigan / Michigan South",
"NAD83 / Maine East (ftUS)",
"NAD83 / Maine West (ftUS)",
"NAD83 / Minnesota North (ftUS)",
"NAD83 / Minnesota Central (ftUS)",
"NAD83 / Minnesota South (ftUS)",
"NAD83 / Nebraska (ftUS)",
"NAD83 / West Virginia North (ftUS)",
"NAD83 / West Virginia South (ftUS)",
"NAD83(HARN) / Maine East (ftUS)",
"NAD83(HARN) / Maine West (ftUS)",
"NAD83(HARN) / Minnesota North (ftUS)",
"NAD83(HARN) / Minnesota Central (ftUS)",
"NAD83(HARN) / Minnesota South (ftUS)",
"NAD83(HARN) / Nebraska (ftUS)",
"NAD83(HARN) / West Virginia North (ftUS)",
"NAD83(HARN) / West Virginia South (ftUS)",
"NAD83(NSRS2007) / Maine East (ftUS)",
"NAD83(NSRS2007) / Maine West (ftUS)",
"NAD83(NSRS2007) / Minnesota North (ftUS)",
"NAD83(NSRS2007) / Minnesota Central (ftUS)",
"NAD83(NSRS2007) / Minnesota South (ftUS)",
"NAD83(NSRS2007) / Nebraska (ftUS)",
"NAD83(NSRS2007) / West Virginia North (ftUS)",
"NAD83(NSRS2007) / West Virginia South (ftUS)",
"NAD83 / Maine East (ftUS)",
"NAD83 / Maine West (ftUS)",
"NAD83 / Minnesota North (ftUS)",
"NAD83 / Minnesota Central (ftUS)",
"NAD83 / Minnesota South (ftUS)",
"NAD83 / Nebraska (ftUS)",
"NAD83 / West Virginia North (ftUS)",
"NAD83 / West Virginia South (ftUS)",
"NAD83(HARN) / Maine East (ftUS)",
"NAD83(HARN) / Maine West (ftUS)",
"NAD83(HARN) / Minnesota North (ftUS)",
"NAD83(HARN) / Minnesota Central (ftUS)",
"NAD83(HARN) / Minnesota South (ftUS)",
"NAD83(HARN) / Nebraska (ftUS)",
"NAD83(HARN) / West Virginia North (ftUS)",
"NAD83(HARN) / West Virginia South (ftUS)",
"NAD83(NSRS2007) / Maine East (ftUS)",
"NAD83(NSRS2007) / Maine West (ftUS)",
"NAD83(NSRS2007) / Minnesota North (ftUS)",
"NAD83(NSRS2007) / Minnesota Central (ftUS)",
"NAD83(NSRS2007) / Minnesota South (ftUS)",
"NAD83(NSRS2007) / Nebraska (ftUS)",
"NAD83(NSRS2007) / West Virginia North (ftUS)",
"NAD83(NSRS2007) / West Virginia South (ftUS)",
"NAD83(CSRS) / MTM zone 11",
"NAD83(CSRS) / MTM zone 12",
"NAD83(CSRS) / MTM zone 13",
"NAD83(CSRS) / MTM zone 14",
"NAD83(CSRS) / MTM zone 15",
"NAD83(CSRS) / MTM zone 16",
"NAD83(CSRS) / MTM zone 17",
"NAD83(CSRS) / MTM zone 1",
"NAD83(CSRS) / MTM zone 2",
"NAD83 / UTM zone 1N",
"NAD83 / UTM zone 2N",
"NAD83 / UTM zone 3N",
"NAD83 / UTM zone 4N",
"NAD83 / UTM zone 5N",
"NAD83 / UTM zone 6N",
"NAD83 / UTM zone 7N",
"NAD83 / UTM zone 8N",
"NAD83 / UTM zone 9N",
"NAD83 / UTM zone 10N",
"NAD83 / UTM zone 11N",
"NAD83 / UTM zone 12N",
"NAD83 / UTM zone 13N",
"NAD83 / UTM zone 14N",
"NAD83 / UTM zone 15N",
"NAD83 / UTM zone 16N",
"NAD83 / UTM zone 17N",
"NAD83 / UTM zone 18N",
"NAD83 / UTM zone 19N",
"NAD83 / UTM zone 20N",
"NAD83 / UTM zone 21N",
"NAD83 / UTM zone 22N",
"NAD83 / UTM zone 23N",
"NAD83 / Alabama East",
"NAD83 / Alabama West",
"NAD83 / Alaska zone 1",
"NAD83 / Alaska zone 2",
"NAD83 / Alaska zone 3",
"NAD83 / Alaska zone 4",
"NAD83 / Alaska zone 5",
"NAD83 / Alaska zone 6",
"NAD83 / Alaska zone 7",
"NAD83 / Alaska zone 8",
"NAD83 / Alaska zone 9",
"NAD83 / Alaska zone 10",
"NAD83 / California zone 1",
"NAD83 / California zone 2",
"NAD83 / California zone 3",
"NAD83 / California zone 4",
"NAD83 / California zone 5",
"NAD83 / California zone 6",
"NAD83 / Arizona East",
"NAD83 / Arizona Central",
"NAD83 / Arizona West",
"NAD83 / Arkansas North",
"NAD83 / Arkansas South",
"NAD83 / Colorado North",
"NAD83 / Colorado Central",
"NAD83 / Colorado South",
"NAD83 / Connecticut",
"NAD83 / Delaware",
"NAD83 / Florida East",
"NAD83 / Florida West",
"NAD83 / Florida North",
"NAD83 / Hawaii zone 1",
"NAD83 / Hawaii zone 2",
"NAD83 / Hawaii zone 3",
"NAD83 / Hawaii zone 4",
"NAD83 / Hawaii zone 5",
"NAD83 / Georgia East",
"NAD83 / Georgia West",
"NAD83 / Idaho East",
"NAD83 / Idaho Central",
"NAD83 / Idaho West",
"NAD83 / Illinois East",
"NAD83 / Illinois West",
"NAD83 / Indiana East",
"NAD83 / Indiana West",
"NAD83 / Iowa North",
"NAD83 / Iowa South",
"NAD83 / Kansas North",
"NAD83 / Kansas South",
"NAD83 / Kentucky North",
"NAD83 / Kentucky South",
"NAD83 / Louisiana North",
"NAD83 / Louisiana South",
"NAD83 / Maine East",
"NAD83 / Maine West",
"NAD83 / Maryland",
"NAD83 / Massachusetts Mainland",
"NAD83 / Massachusetts Island",
"NAD83 / Michigan North",
"NAD83 / Michigan Central",
"NAD83 / Michigan South",
"NAD83 / Minnesota North",
"NAD83 / Minnesota Central",
"NAD83 / Minnesota South",
"NAD83 / Mississippi East",
"NAD83 / Mississippi West",
"NAD83 / Missouri East",
"NAD83 / Missouri Central",
"NAD83 / Missouri West",
"Nahrwan 1967 / UTM zone 37N",
"Nahrwan 1967 / UTM zone 38N",
"Nahrwan 1967 / UTM zone 39N",
"Nahrwan 1967 / UTM zone 40N",
"Naparima 1972 / UTM zone 20N",
"NZGD49 / New Zealand Map Grid",
"NZGD49 / Mount Eden Circuit",
"NZGD49 / Bay of Plenty Circuit",
"NZGD49 / Poverty Bay Circuit",
"NZGD49 / Hawkes Bay Circuit",
"NZGD49 / Taranaki Circuit",
"NZGD49 / Tuhirangi Circuit",
"NZGD49 / Wanganui Circuit",
"NZGD49 / Wairarapa Circuit",
"NZGD49 / Wellington Circuit",
"NZGD49 / Collingwood Circuit",
"NZGD49 / Nelson Circuit",
"NZGD49 / Karamea Circuit",
"NZGD49 / Buller Circuit",
"NZGD49 / Grey Circuit",
"NZGD49 / Amuri Circuit",
"NZGD49 / Marlborough Circuit",
"NZGD49 / Hokitika Circuit",
"NZGD49 / Okarito Circuit",
"NZGD49 / Jacksons Bay Circuit",
"NZGD49 / Mount Pleasant Circuit",
"NZGD49 / Gawler Circuit",
"NZGD49 / Timaru Circuit",
"NZGD49 / Lindis Peak Circuit",
"NZGD49 / Mount Nicholas Circuit",
"NZGD49 / Mount York Circuit",
"NZGD49 / Observation Point Circuit",
"NZGD49 / North Taieri Circuit",
"NZGD49 / Bluff Circuit",
"NZGD49 / UTM zone 58S",
"NZGD49 / UTM zone 59S",
"NZGD49 / UTM zone 60S",
"NZGD49 / North Island Grid",
"NZGD49 / South Island Grid",
"NGO 1948 (Oslo) / NGO zone I",
"NGO 1948 (Oslo) / NGO zone II",
"NGO 1948 (Oslo) / NGO zone III",
"NGO 1948 (Oslo) / NGO zone IV",
"NGO 1948 (Oslo) / NGO zone V",
"NGO 1948 (Oslo) / NGO zone VI",
"NGO 1948 (Oslo) / NGO zone VII",
"NGO 1948 (Oslo) / NGO zone VIII",
"Datum 73 / UTM zone 29N",
"Datum 73 / Modified Portuguese Grid",
"Datum 73 / Modified Portuguese Grid",
"ATF (Paris) / Nord de Guerre",
"NTF (Paris) / Lambert Nord France",
"NTF (Paris) / Lambert Centre France",
"NTF (Paris) / Lambert Sud France",
"NTF (Paris) / Lambert Corse",
"NTF (Paris) / Lambert zone I",
"NTF (Paris) / Lambert zone II",
"NTF (Paris) / Lambert zone III",
"NTF (Paris) / Lambert zone IV",
"NTF (Paris) / France I",
"NTF (Paris) / France II",
"NTF (Paris) / France III",
"NTF (Paris) / France IV",
"NTF (Paris) / Nord France",
"NTF (Paris) / Centre France",
"NTF (Paris) / Sud France",
"NTF (Paris) / Corse",
"OSGB 1936 / British National Grid",
"Palestine 1923 / Palestine Grid",
"Palestine 1923 / Palestine Belt",
"Palestine 1923 / Israeli CS Grid",
"Pointe Noire / UTM zone 32S",
"GDA94 / MGA zone 48",
"GDA94 / MGA zone 49",
"GDA94 / MGA zone 50",
"GDA94 / MGA zone 51",
"GDA94 / MGA zone 52",
"GDA94 / MGA zone 53",
"GDA94 / MGA zone 54",
"GDA94 / MGA zone 55",
"GDA94 / MGA zone 56",
"GDA94 / MGA zone 57",
"GDA94 / MGA zone 58",
"Pulkovo 1942 / Gauss-Kruger zone 2",
"Pulkovo 1942 / Gauss-Kruger zone 3",
"Pulkovo 1942 / Gauss-Kruger zone 4",
"Pulkovo 1942 / Gauss-Kruger zone 5",
"Pulkovo 1942 / Gauss-Kruger zone 6",
"Pulkovo 1942 / Gauss-Kruger zone 7",
"Pulkovo 1942 / Gauss-Kruger zone 8",
"Pulkovo 1942 / Gauss-Kruger zone 9",
"Pulkovo 1942 / Gauss-Kruger zone 10",
"Pulkovo 1942 / Gauss-Kruger zone 11",
"Pulkovo 1942 / Gauss-Kruger zone 12",
"Pulkovo 1942 / Gauss-Kruger zone 13",
"Pulkovo 1942 / Gauss-Kruger zone 14",
"Pulkovo 1942 / Gauss-Kruger zone 15",
"Pulkovo 1942 / Gauss-Kruger zone 16",
"Pulkovo 1942 / Gauss-Kruger zone 17",
"Pulkovo 1942 / Gauss-Kruger zone 18",
"Pulkovo 1942 / Gauss-Kruger zone 19",
"Pulkovo 1942 / Gauss-Kruger zone 20",
"Pulkovo 1942 / Gauss-Kruger zone 21",
"Pulkovo 1942 / Gauss-Kruger zone 22",
"Pulkovo 1942 / Gauss-Kruger zone 23",
"Pulkovo 1942 / Gauss-Kruger zone 24",
"Pulkovo 1942 / Gauss-Kruger zone 25",
"Pulkovo 1942 / Gauss-Kruger zone 26",
"Pulkovo 1942 / Gauss-Kruger zone 27",
"Pulkovo 1942 / Gauss-Kruger zone 28",
"Pulkovo 1942 / Gauss-Kruger zone 29",
"Pulkovo 1942 / Gauss-Kruger zone 30",
"Pulkovo 1942 / Gauss-Kruger zone 31",
"Pulkovo 1942 / Gauss-Kruger zone 32",
"Pulkovo 1942 / Gauss-Kruger 2N",
"Pulkovo 1942 / Gauss-Kruger 3N",
"Pulkovo 1942 / Gauss-Kruger 4N",
"Pulkovo 1942 / Gauss-Kruger 5N",
"Pulkovo 1942 / Gauss-Kruger 6N",
"Pulkovo 1942 / Gauss-Kruger 7N",
"Pulkovo 1942 / Gauss-Kruger 8N",
"Pulkovo 1942 / Gauss-Kruger 9N",
"Pulkovo 1942 / Gauss-Kruger 10N",
"Pulkovo 1942 / Gauss-Kruger 11N",
"Pulkovo 1942 / Gauss-Kruger 12N",
"Pulkovo 1942 / Gauss-Kruger 13N",
"Pulkovo 1942 / Gauss-Kruger 14N",
"Pulkovo 1942 / Gauss-Kruger 15N",
"Pulkovo 1942 / Gauss-Kruger 16N",
"Pulkovo 1942 / Gauss-Kruger 17N",
"Pulkovo 1942 / Gauss-Kruger 18N",
"Pulkovo 1942 / Gauss-Kruger 19N",
"Pulkovo 1942 / Gauss-Kruger 20N",
"Pulkovo 1942 / Gauss-Kruger 21N",
"Pulkovo 1942 / Gauss-Kruger 22N",
"Pulkovo 1942 / Gauss-Kruger 23N",
"Pulkovo 1942 / Gauss-Kruger 24N",
"Pulkovo 1942 / Gauss-Kruger 25N",
"Pulkovo 1942 / Gauss-Kruger 26N",
"Pulkovo 1942 / Gauss-Kruger 27N",
"Pulkovo 1942 / Gauss-Kruger 28N",
"Pulkovo 1942 / Gauss-Kruger 29N",
"Pulkovo 1942 / Gauss-Kruger 30N",
"Pulkovo 1942 / Gauss-Kruger 31N",
"Pulkovo 1942 / Gauss-Kruger 32N",
"Qatar 1974 / Qatar National Grid",
"Amersfoort / RD Old",
"Amersfoort / RD New",
"SAD69 / Brazil Polyconic",
"SAD69 / Brazil Polyconic",
"SAD69 / UTM zone 18N",
"SAD69 / UTM zone 19N",
"SAD69 / UTM zone 20N",
"SAD69 / UTM zone 21N",
"SAD69 / UTM zone 22N",
"SAD69 / UTM zone 18N",
"SAD69 / UTM zone 19N",
"SAD69 / UTM zone 20N",
"SAD69 / UTM zone 21N",
"SAD69 / UTM zone 22N",
"SAD69 / UTM zone 17S",
"SAD69 / UTM zone 18S",
"SAD69 / UTM zone 19S",
"SAD69 / UTM zone 20S",
"SAD69 / UTM zone 21S",
"SAD69 / UTM zone 22S",
"SAD69 / UTM zone 23S",
"SAD69 / UTM zone 24S",
"SAD69 / UTM zone 25S",
"SAD69 / UTM zone 17S",
"SAD69 / UTM zone 18S",
"SAD69 / UTM zone 19S",
"SAD69 / UTM zone 20S",
"SAD69 / UTM zone 21S",
"SAD69 / UTM zone 22S",
"SAD69 / UTM zone 23S",
"SAD69 / UTM zone 24S",
"SAD69 / UTM zone 25S",
"Sapper Hill 1943 / UTM zone 20S",
"Sapper Hill 1943 / UTM zone 21S",
"Schwarzeck / UTM zone 33S",
"Schwarzeck / Lo22/11",
"Schwarzeck / Lo22/13",
"Schwarzeck / Lo22/15",
"Schwarzeck / Lo22/17",
"Schwarzeck / Lo22/19",
"Schwarzeck / Lo22/21",
"Schwarzeck / Lo22/23",
"Schwarzeck / Lo22/25",
"Sudan / UTM zone 35N",
"Sudan / UTM zone 36N",
"Tananarive (Paris) / Laborde Grid",
"Tananarive (Paris) / Laborde Grid",
"Tananarive (Paris) / Laborde Grid approximation",
"Tananarive / UTM zone 38S",
"Tananarive / UTM zone 39S",
"Timbalai 1948 / UTM zone 49N",
"Timbalai 1948 / UTM zone 50N",
"Timbalai 1948 / RSO Borneo (ch)",
"Timbalai 1948 / RSO Borneo (ft)",
"Timbalai 1948 / RSO Borneo (m)",
"TM65 / Irish National Grid",
"OSNI 1952 / Irish National Grid",
"TM65 / Irish Grid",
"TM75 / Irish Grid",
"Tokyo / Japan Plane Rectangular CS I",
"Tokyo / Japan Plane Rectangular CS II",
"Tokyo / Japan Plane Rectangular CS III",
"Tokyo / Japan Plane Rectangular CS IV",
"Tokyo / Japan Plane Rectangular CS V",
"Tokyo / Japan Plane Rectangular CS VI",
"Tokyo / Japan Plane Rectangular CS VII",
"Tokyo / Japan Plane Rectangular CS VIII",
"Tokyo / Japan Plane Rectangular CS IX",
"Tokyo / Japan Plane Rectangular CS X",
"Tokyo / Japan Plane Rectangular CS XI",
"Tokyo / Japan Plane Rectangular CS XII",
"Tokyo / Japan Plane Rectangular CS XIII",
"Tokyo / Japan Plane Rectangular CS XIV",
"Tokyo / Japan Plane Rectangular CS XV",
"Tokyo / Japan Plane Rectangular CS XVI",
"Tokyo / Japan Plane Rectangular CS XVII",
"Tokyo / Japan Plane Rectangular CS XVIII",
"Tokyo / Japan Plane Rectangular CS XIX",
"Trinidad 1903 / Trinidad Grid",
"TC(1948) / UTM zone 39N",
"TC(1948) / UTM zone 40N",
"Voirol 1875 / Nord Algerie (ancienne)",
"Voirol 1875 / Sud Algerie (ancienne)",
"Voirol 1879 / Nord Algerie (ancienne)",
"Voirol 1879 / Sud Algerie (ancienne)",
"Nord Sahara 1959 / UTM zone 29N",
"Nord Sahara 1959 / UTM zone 30N",
"Nord Sahara 1959 / UTM zone 31N",
"Nord Sahara 1959 / UTM zone 32N",
"Nord Sahara 1959 / Nord Algerie",
"Nord Sahara 1959 / Sud Algerie",
"RT38 2.5 gon W",
"Yoff / UTM zone 28N",
"Zanderij / UTM zone 21N",
"Zanderij / TM 54 NW",
"Zanderij / Suriname Old TM",
"Zanderij / Suriname TM",
"MGI (Ferro) / Austria GK West Zone",
"MGI (Ferro) / Austria GK Central Zone",
"MGI (Ferro) / Austria GK East Zone",
"MGI / Austria GK West",
"MGI / Austria GK Central",
"MGI / Austria GK East",
"MGI / Austria GK M28",
"MGI / Austria GK M31",
"MGI / Austria GK M34",
"MGI / 3-degree Gauss zone 5",
"MGI / 3-degree Gauss zone 6",
"MGI / 3-degree Gauss zone 7",
"MGI / 3-degree Gauss zone 8",
"MGI / Balkans zone 5",
"MGI / Balkans zone 6",
"MGI / Balkans zone 7",
"MGI / Balkans zone 8",
"MGI / Balkans zone 8",
"MGI (Ferro) / Austria West Zone",
"MGI (Ferro) / Austria Central Zone",
"MGI (Ferro) / Austria East Zone",
"MGI / Austria M28",
"MGI / Austria M31",
"MGI / Austria M34",
"MGI / Austria Lambert",
"MGI (Ferro) / M28",
"MGI (Ferro) / M31",
"MGI (Ferro) / M34",
"MGI (Ferro) / Austria West Zone",
"MGI (Ferro) / Austria Central Zone",
"MGI (Ferro) / Austria East Zone",
"MGI / M28",
"MGI / M31",
"MGI / M34",
"MGI / Austria Lambert",
"Belge 1972 / Belge Lambert 72",
"Belge 1972 / Belgian Lambert 72",
"DHDN / 3-degree Gauss zone 1",
"DHDN / 3-degree Gauss zone 2",
"DHDN / 3-degree Gauss zone 3",
"DHDN / 3-degree Gauss zone 4",
"DHDN / 3-degree Gauss zone 5",
"DHDN / 3-degree Gauss-Kruger zone 2",
"DHDN / 3-degree Gauss-Kruger zone 3",
"DHDN / 3-degree Gauss-Kruger zone 4",
"DHDN / 3-degree Gauss-Kruger zone 5",
"Conakry 1905 / UTM zone 28N",
"Conakry 1905 / UTM zone 29N",
"Dealul Piscului 1930 / Stereo 33",
"Dealul Piscului 1970/ Stereo 70",
"NGN / UTM zone 38N",
"NGN / UTM zone 39N",
"KUDAMS / KTM",
"KUDAMS / KTM",
"SIRGAS 2000 / UTM zone 11N",
"SIRGAS 2000 / UTM zone 12N",
"SIRGAS 2000 / UTM zone 13N",
"SIRGAS 2000 / UTM zone 14N",
"SIRGAS 2000 / UTM zone 15N",
"SIRGAS 2000 / UTM zone 16N",
"SIRGAS 2000 / UTM zone 17N",
"SIRGAS 2000 / UTM zone 18N",
"SIRGAS 2000 / UTM zone 19N",
"SIRGAS 2000 / UTM zone 20N",
"SIRGAS 2000 / UTM zone 21N",
"SIRGAS 2000 / UTM zone 22N",
"SIRGAS 2000 / UTM zone 17S",
"SIRGAS 2000 / UTM zone 18S",
"SIRGAS 2000 / UTM zone 19S",
"SIRGAS 2000 / UTM zone 20S",
"SIRGAS 2000 / UTM zone 21S",
"SIRGAS 2000 / UTM zone 22S",
"SIRGAS 2000 / UTM zone 23S",
"SIRGAS 2000 / UTM zone 24S",
"SIRGAS 2000 / UTM zone 25S",
"SIRGAS 1995 / UTM zone 17N",
"SIRGAS 1995 / UTM zone 18N",
"SIRGAS 1995 / UTM zone 19N",
"SIRGAS 1995 / UTM zone 20N",
"SIRGAS 1995 / UTM zone 21N",
"SIRGAS 1995 / UTM zone 22N",
"SIRGAS 1995 / UTM zone 17S",
"SIRGAS 1995 / UTM zone 18S",
"SIRGAS 1995 / UTM zone 19S",
"SIRGAS 1995 / UTM zone 20S",
"SIRGAS 1995 / UTM zone 21S",
"SIRGAS 1995 / UTM zone 22S",
"SIRGAS 1995 / UTM zone 23S",
"SIRGAS 1995 / UTM zone 24S",
"SIRGAS 1995 / UTM zone 25S",
"NAD27 / Montana North",
"NAD27 / Montana Central",
"NAD27 / Montana South",
"NAD27 / Nebraska North",
"NAD27 / Nebraska South",
"NAD27 / Nevada East",
"NAD27 / Nevada Central",
"NAD27 / Nevada West",
"NAD27 / New Hampshire",
"NAD27 / New Jersey",
"NAD27 / New Mexico East",
"NAD27 / New Mexico Central",
"NAD27 / New Mexico West",
"NAD27 / New York East",
"NAD27 / New York Central",
"NAD27 / New York West",
"NAD27 / New York Long Island",
"NAD27 / North Carolina",
"NAD27 / North Dakota North",
"NAD27 / North Dakota South",
"NAD27 / Ohio North",
"NAD27 / Ohio South",
"NAD27 / Oklahoma North",
"NAD27 / Oklahoma South",
"NAD27 / Oregon North",
"NAD27 / Oregon South",
"NAD27 / Pennsylvania North",
"NAD27 / Pennsylvania South",
"NAD27 / Rhode Island",
"NAD27 / South Carolina North",
"NAD27 / South Carolina South",
"NAD27 / South Dakota North",
"NAD27 / South Dakota South",
"NAD27 / Tennessee",
"NAD27 / Texas North",
"NAD27 / Texas North Central",
"NAD27 / Texas Central",
"NAD27 / Texas South Central",
"NAD27 / Texas South",
"NAD27 / Utah North",
"NAD27 / Utah Central",
"NAD27 / Utah South",
"NAD27 / Vermont",
"NAD27 / Virginia North",
"NAD27 / Virginia South",
"NAD27 / Washington North",
"NAD27 / Washington South",
"NAD27 / West Virginia North",
"NAD27 / West Virginia South",
"NAD27 / Wisconsin North",
"NAD27 / Wisconsin Central",
"NAD27 / Wisconsin South",
"NAD27 / Wyoming East",
"NAD27 / Wyoming East Central",
"NAD27 / Wyoming West Central",
"NAD27 / Wyoming West",
"NAD27 / Guatemala Norte",
"NAD27 / Guatemala Sur",
"NAD27 / BLM 14N (ftUS)",
"NAD27 / BLM 15N (ftUS)",
"NAD27 / BLM 16N (ftUS)",
"NAD27 / BLM 17N (ftUS)",
"NAD27 / BLM 14N (feet)",
"NAD27 / BLM 15N (feet)",
"NAD27 / BLM 16N (feet)",
"NAD27 / BLM 17N (feet)",
"NAD27 / MTM zone 1",
"NAD27 / MTM zone 2",
"NAD27 / MTM zone 3",
"NAD27 / MTM zone 4",
"NAD27 / MTM zone 5",
"NAD27 / MTM zone 6",
"NAD27 / Quebec Lambert",
"NAD27 / Louisiana Offshore",
"NAD83 / Montana",
"NAD83 / Nebraska",
"NAD83 / Nevada East",
"NAD83 / Nevada Central",
"NAD83 / Nevada West",
"NAD83 / New Hampshire",
"NAD83 / New Jersey",
"NAD83 / New Mexico East",
"NAD83 / New Mexico Central",
"NAD83 / New Mexico West",
"NAD83 / New York East",
"NAD83 / New York Central",
"NAD83 / New York West",
"NAD83 / New York Long Island",
"NAD83 / North Carolina",
"NAD83 / North Dakota North",
"NAD83 / North Dakota South",
"NAD83 / Ohio North",
"NAD83 / Ohio South",
"NAD83 / Oklahoma North",
"NAD83 / Oklahoma South",
"NAD83 / Oregon North",
"NAD83 / Oregon South",
"NAD83 / Pennsylvania North",
"NAD83 / Pennsylvania South",
"NAD83 / Rhode Island",
"NAD83 / South Carolina",
"NAD83 / South Dakota North",
"NAD83 / South Dakota South",
"NAD83 / Tennessee",
"NAD83 / Texas North",
"NAD83 / Texas North Central",
"NAD83 / Texas Central",
"NAD83 / Texas South Central",
"NAD83 / Texas South",
"NAD83 / Utah North",
"NAD83 / Utah Central",
"NAD83 / Utah South",
"NAD83 / Vermont",
"NAD83 / Virginia North",
"NAD83 / Virginia South",
"NAD83 / Washington North",
"NAD83 / Washington South",
"NAD83 / West Virginia North",
"NAD83 / West Virginia South",
"NAD83 / Wisconsin North",
"NAD83 / Wisconsin Central",
"NAD83 / Wisconsin South",
"NAD83 / Wyoming East",
"NAD83 / Wyoming East Central",
"NAD83 / Wyoming West Central",
"NAD83 / Wyoming West",
"NAD83 / Puerto Rico & Virgin Is.",
"NAD83 / BLM 14N (ftUS)",
"NAD83 / BLM 15N (ftUS)",
"NAD83 / BLM 16N (ftUS)",
"NAD83 / BLM 17N (ftUS)",
"NAD83 / SCoPQ zone 2",
"NAD83 / MTM zone 1",
"NAD83 / MTM zone 2",
"NAD83 / MTM zone 3",
"NAD83 / MTM zone 4",
"NAD83 / MTM zone 5",
"NAD83 / MTM zone 6",
"NAD83 / MTM zone 7",
"NAD83 / MTM zone 8",
"NAD83 / MTM zone 9",
"NAD83 / MTM zone 10",
"NAD83 / MTM zone 11",
"NAD83 / MTM zone 12",
"NAD83 / MTM zone 13",
"NAD83 / MTM zone 14",
"NAD83 / MTM zone 15",
"NAD83 / MTM zone 16",
"NAD83 / MTM zone 17",
"NAD83 / Quebec Lambert",
"NAD83 / Louisiana Offshore",
"WGS 72 / UTM zone 1N",
"WGS 72 / UTM zone 2N",
"WGS 72 / UTM zone 3N",
"WGS 72 / UTM zone 4N",
"WGS 72 / UTM zone 5N",
"WGS 72 / UTM zone 6N",
"WGS 72 / UTM zone 7N",
"WGS 72 / UTM zone 8N",
"WGS 72 / UTM zone 9N",
"WGS 72 / UTM zone 10N",
"WGS 72 / UTM zone 11N",
"WGS 72 / UTM zone 12N",
"WGS 72 / UTM zone 13N",
"WGS 72 / UTM zone 14N",
"WGS 72 / UTM zone 15N",
"WGS 72 / UTM zone 16N",
"WGS 72 / UTM zone 17N",
"WGS 72 / UTM zone 18N",
"WGS 72 / UTM zone 19N",
"WGS 72 / UTM zone 20N",
"WGS 72 / UTM zone 21N",
"WGS 72 / UTM zone 22N",
"WGS 72 / UTM zone 23N",
"WGS 72 / UTM zone 24N",
"WGS 72 / UTM zone 25N",
"WGS 72 / UTM zone 26N",
"WGS 72 / UTM zone 27N",
"WGS 72 / UTM zone 28N",
"WGS 72 / UTM zone 29N",
"WGS 72 / UTM zone 30N",
"WGS 72 / UTM zone 31N",
"WGS 72 / UTM zone 32N",
"WGS 72 / UTM zone 33N",
"WGS 72 / UTM zone 34N",
"WGS 72 / UTM zone 35N",
"WGS 72 / UTM zone 36N",
"WGS 72 / UTM zone 37N",
"WGS 72 / UTM zone 38N",
"WGS 72 / UTM zone 39N",
"WGS 72 / UTM zone 40N",
"WGS 72 / UTM zone 41N",
"WGS 72 / UTM zone 42N",
"WGS 72 / UTM zone 43N",
"WGS 72 / UTM zone 44N",
"WGS 72 / UTM zone 45N",
"WGS 72 / UTM zone 46N",
"WGS 72 / UTM zone 47N",
"WGS 72 / UTM zone 48N",
"WGS 72 / UTM zone 49N",
"WGS 72 / UTM zone 50N",
"WGS 72 / UTM zone 51N",
"WGS 72 / UTM zone 52N",
"WGS 72 / UTM zone 53N",
"WGS 72 / UTM zone 54N",
"WGS 72 / UTM zone 55N",
"WGS 72 / UTM zone 56N",
"WGS 72 / UTM zone 57N",
"WGS 72 / UTM zone 58N",
"WGS 72 / UTM zone 59N",
"WGS 72 / UTM zone 60N",
"WGS 72 / UTM zone 1S",
"WGS 72 / UTM zone 2S",
"WGS 72 / UTM zone 3S",
"WGS 72 / UTM zone 4S",
"WGS 72 / UTM zone 5S",
"WGS 72 / UTM zone 6S",
"WGS 72 / UTM zone 7S",
"WGS 72 / UTM zone 8S",
"WGS 72 / UTM zone 9S",
"WGS 72 / UTM zone 10S",
"WGS 72 / UTM zone 11S",
"WGS 72 / UTM zone 12S",
"WGS 72 / UTM zone 13S",
"WGS 72 / UTM zone 14S",
"WGS 72 / UTM zone 15S",
"WGS 72 / UTM zone 16S",
"WGS 72 / UTM zone 17S",
"WGS 72 / UTM zone 18S",
"WGS 72 / UTM zone 19S",
"WGS 72 / UTM zone 20S",
"WGS 72 / UTM zone 21S",
"WGS 72 / UTM zone 22S",
"WGS 72 / UTM zone 23S",
"WGS 72 / UTM zone 24S",
"WGS 72 / UTM zone 25S",
"WGS 72 / UTM zone 26S",
"WGS 72 / UTM zone 27S",
"WGS 72 / UTM zone 28S",
"WGS 72 / UTM zone 29S",
"WGS 72 / UTM zone 30S",
"WGS 72 / UTM zone 31S",
"WGS 72 / UTM zone 32S",
"WGS 72 / UTM zone 33S",
"WGS 72 / UTM zone 34S",
"WGS 72 / UTM zone 35S",
"WGS 72 / UTM zone 36S",
"WGS 72 / UTM zone 37S",
"WGS 72 / UTM zone 38S",
"WGS 72 / UTM zone 39S",
"WGS 72 / UTM zone 40S",
"WGS 72 / UTM zone 41S",
"WGS 72 / UTM zone 42S",
"WGS 72 / UTM zone 43S",
"WGS 72 / UTM zone 44S",
"WGS 72 / UTM zone 45S",
"WGS 72 / UTM zone 46S",
"WGS 72 / UTM zone 47S",
"WGS 72 / UTM zone 48S",
"WGS 72 / UTM zone 49S",
"WGS 72 / UTM zone 50S",
"WGS 72 / UTM zone 51S",
"WGS 72 / UTM zone 52S",
"WGS 72 / UTM zone 53S",
"WGS 72 / UTM zone 54S",
"WGS 72 / UTM zone 55S",
"WGS 72 / UTM zone 56S",
"WGS 72 / UTM zone 57S",
"WGS 72 / UTM zone 58S",
"WGS 72 / UTM zone 59S",
"WGS 72 / UTM zone 60S",
"WGS 72BE / UTM zone 1N",
"WGS 72BE / UTM zone 2N",
"WGS 72BE / UTM zone 3N",
"WGS 72BE / UTM zone 4N",
"WGS 72BE / UTM zone 5N",
"WGS 72BE / UTM zone 6N",
"WGS 72BE / UTM zone 7N",
"WGS 72BE / UTM zone 8N",
"WGS 72BE / UTM zone 9N",
"WGS 72BE / UTM zone 10N",
"WGS 72BE / UTM zone 11N",
"WGS 72BE / UTM zone 12N",
"WGS 72BE / UTM zone 13N",
"WGS 72BE / UTM zone 14N",
"WGS 72BE / UTM zone 15N",
"WGS 72BE / UTM zone 16N",
"WGS 72BE / UTM zone 17N",
"WGS 72BE / UTM zone 18N",
"WGS 72BE / UTM zone 19N",
"WGS 72BE / UTM zone 20N",
"WGS 72BE / UTM zone 21N",
"WGS 72BE / UTM zone 22N",
"WGS 72BE / UTM zone 23N",
"WGS 72BE / UTM zone 24N",
"WGS 72BE / UTM zone 25N",
"WGS 72BE / UTM zone 26N",
"WGS 72BE / UTM zone 27N",
"WGS 72BE / UTM zone 28N",
"WGS 72BE / UTM zone 29N",
"WGS 72BE / UTM zone 30N",
"WGS 72BE / UTM zone 31N",
"WGS 72BE / UTM zone 32N",
"WGS 72BE / UTM zone 33N",
"WGS 72BE / UTM zone 34N",
"WGS 72BE / UTM zone 35N",
"WGS 72BE / UTM zone 36N",
"WGS 72BE / UTM zone 37N",
"WGS 72BE / UTM zone 38N",
"WGS 72BE / UTM zone 39N",
"WGS 72BE / UTM zone 40N",
"WGS 72BE / UTM zone 41N",
"WGS 72BE / UTM zone 42N",
"WGS 72BE / UTM zone 43N",
"WGS 72BE / UTM zone 44N",
"WGS 72BE / UTM zone 45N",
"WGS 72BE / UTM zone 46N",
"WGS 72BE / UTM zone 47N",
"WGS 72BE / UTM zone 48N",
"WGS 72BE / UTM zone 49N",
"WGS 72BE / UTM zone 50N",
"WGS 72BE / UTM zone 51N",
"WGS 72BE / UTM zone 52N",
"WGS 72BE / UTM zone 53N",
"WGS 72BE / UTM zone 54N",
"WGS 72BE / UTM zone 55N",
"WGS 72BE / UTM zone 56N",
"WGS 72BE / UTM zone 57N",
"WGS 72BE / UTM zone 58N",
"WGS 72BE / UTM zone 59N",
"WGS 72BE / UTM zone 60N",
"WGS 72BE / UTM zone 1S",
"WGS 72BE / UTM zone 2S",
"WGS 72BE / UTM zone 3S",
"WGS 72BE / UTM zone 4S",
"WGS 72BE / UTM zone 5S",
"WGS 72BE / UTM zone 6S",
"WGS 72BE / UTM zone 7S",
"WGS 72BE / UTM zone 8S",
"WGS 72BE / UTM zone 9S",
"WGS 72BE / UTM zone 10S",
"WGS 72BE / UTM zone 11S",
"WGS 72BE / UTM zone 12S",
"WGS 72BE / UTM zone 13S",
"WGS 72BE / UTM zone 14S",
"WGS 72BE / UTM zone 15S",
"WGS 72BE / UTM zone 16S",
"WGS 72BE / UTM zone 17S",
"WGS 72BE / UTM zone 18S",
"WGS 72BE / UTM zone 19S",
"WGS 72BE / UTM zone 20S",
"WGS 72BE / UTM zone 21S",
"WGS 72BE / UTM zone 22S",
"WGS 72BE / UTM zone 23S",
"WGS 72BE / UTM zone 24S",
"WGS 72BE / UTM zone 25S",
"WGS 72BE / UTM zone 26S",
"WGS 72BE / UTM zone 27S",
"WGS 72BE / UTM zone 28S",
"WGS 72BE / UTM zone 29S",
"WGS 72BE / UTM zone 30S",
"WGS 72BE / UTM zone 31S",
"WGS 72BE / UTM zone 32S",
"WGS 72BE / UTM zone 33S",
"WGS 72BE / UTM zone 34S",
"WGS 72BE / UTM zone 35S",
"WGS 72BE / UTM zone 36S",
"WGS 72BE / UTM zone 37S",
"WGS 72BE / UTM zone 38S",
"WGS 72BE / UTM zone 39S",
"WGS 72BE / UTM zone 40S",
"WGS 72BE / UTM zone 41S",
"WGS 72BE / UTM zone 42S",
"WGS 72BE / UTM zone 43S",
"WGS 72BE / UTM zone 44S",
"WGS 72BE / UTM zone 45S",
"WGS 72BE / UTM zone 46S",
"WGS 72BE / UTM zone 47S",
"WGS 72BE / UTM zone 48S",
"WGS 72BE / UTM zone 49S",
"WGS 72BE / UTM zone 50S",
"WGS 72BE / UTM zone 51S",
"WGS 72BE / UTM zone 52S",
"WGS 72BE / UTM zone 53S",
"WGS 72BE / UTM zone 54S",
"WGS 72BE / UTM zone 55S",
"WGS 72BE / UTM zone 56S",
"WGS 72BE / UTM zone 57S",
"WGS 72BE / UTM zone 58S",
"WGS 72BE / UTM zone 59S",
"WGS 72BE / UTM zone 60S",
"WGS 84 / UTM grid system (northern hemisphere)",
"WGS 84 / UTM zone 1N",
"WGS 84 / UTM zone 2N",
"WGS 84 / UTM zone 3N",
"WGS 84 / UTM zone 4N",
"WGS 84 / UTM zone 5N",
"WGS 84 / UTM zone 6N",
"WGS 84 / UTM zone 7N",
"WGS 84 / UTM zone 8N",
"WGS 84 / UTM zone 9N",
"WGS 84 / UTM zone 10N",
"WGS 84 / UTM zone 11N",
"WGS 84 / UTM zone 12N",
"WGS 84 / UTM zone 13N",
"WGS 84 / UTM zone 14N",
"WGS 84 / UTM zone 15N",
"WGS 84 / UTM zone 16N",
"WGS 84 / UTM zone 17N",
"WGS 84 / UTM zone 18N",
"WGS 84 / UTM zone 19N",
"WGS 84 / UTM zone 20N",
"WGS 84 / UTM zone 21N",
"WGS 84 / UTM zone 22N",
"WGS 84 / UTM zone 23N",
"WGS 84 / UTM zone 24N",
"WGS 84 / UTM zone 25N",
"WGS 84 / UTM zone 26N",
"WGS 84 / UTM zone 27N",
"WGS 84 / UTM zone 28N",
"WGS 84 / UTM zone 29N",
"WGS 84 / UTM zone 30N",
"WGS 84 / UTM zone 31N",
"WGS 84 / UTM zone 32N",
"WGS 84 / UTM zone 33N",
"WGS 84 / UTM zone 34N",
"WGS 84 / UTM zone 35N",
"WGS 84 / UTM zone 36N",
"WGS 84 / UTM zone 37N",
"WGS 84 / UTM zone 38N",
"WGS 84 / UTM zone 39N",
"WGS 84 / UTM zone 40N",
"WGS 84 / UTM zone 41N",
"WGS 84 / UTM zone 42N",
"WGS 84 / UTM zone 43N",
"WGS 84 / UTM zone 44N",
"WGS 84 / UTM zone 45N",
"WGS 84 / UTM zone 46N",
"WGS 84 / UTM zone 47N",
"WGS 84 / UTM zone 48N",
"WGS 84 / UTM zone 49N",
"WGS 84 / UTM zone 50N",
"WGS 84 / UTM zone 51N",
"WGS 84 / UTM zone 52N",
"WGS 84 / UTM zone 53N",
"WGS 84 / UTM zone 54N",
"WGS 84 / UTM zone 55N",
"WGS 84 / UTM zone 56N",
"WGS 84 / UTM zone 57N",
"WGS 84 / UTM zone 58N",
"WGS 84 / UTM zone 59N",
"WGS 84 / UTM zone 60N",
"WGS 84 / UPS North (N,E)",
"WGS 84 / Plate Carree",
"WGS 84 / World Equidistant Cylindrical",
"WGS 84 / BLM 14N (ftUS)",
"WGS 84 / BLM 15N (ftUS)",
"WGS 84 / BLM 16N (ftUS)",
"WGS 84 / BLM 17N (ftUS)",
"WGS 84 / UTM grid system (southern hemisphere)",
"WGS 84 / UTM zone 1S",
"WGS 84 / UTM zone 2S",
"WGS 84 / UTM zone 3S",
"WGS 84 / UTM zone 4S",
"WGS 84 / UTM zone 5S",
"WGS 84 / UTM zone 6S",
"WGS 84 / UTM zone 7S",
"WGS 84 / UTM zone 8S",
"WGS 84 / UTM zone 9S",
"WGS 84 / UTM zone 10S",
"WGS 84 / UTM zone 11S",
"WGS 84 / UTM zone 12S",
"WGS 84 / UTM zone 13S",
"WGS 84 / UTM zone 14S",
"WGS 84 / UTM zone 15S",
"WGS 84 / UTM zone 16S",
"WGS 84 / UTM zone 17S",
"WGS 84 / UTM zone 18S",
"WGS 84 / UTM zone 19S",
"WGS 84 / UTM zone 20S",
"WGS 84 / UTM zone 21S",
"WGS 84 / UTM zone 22S",
"WGS 84 / UTM zone 23S",
"WGS 84 / UTM zone 24S",
"WGS 84 / UTM zone 25S",
"WGS 84 / UTM zone 26S",
"WGS 84 / UTM zone 27S",
"WGS 84 / UTM zone 28S",
"WGS 84 / UTM zone 29S",
"WGS 84 / UTM zone 30S",
"WGS 84 / UTM zone 31S",
"WGS 84 / UTM zone 32S",
"WGS 84 / UTM zone 33S",
"WGS 84 / UTM zone 34S",
"WGS 84 / UTM zone 35S",
"WGS 84 / UTM zone 36S",
"WGS 84 / UTM zone 37S",
"WGS 84 / UTM zone 38S",
"WGS 84 / UTM zone 39S",
"WGS 84 / UTM zone 40S",
"WGS 84 / UTM zone 41S",
"WGS 84 / UTM zone 42S",
"WGS 84 / UTM zone 43S",
"WGS 84 / UTM zone 44S",
"WGS 84 / UTM zone 45S",
"WGS 84 / UTM zone 46S",
"WGS 84 / UTM zone 47S",
"WGS 84 / UTM zone 48S",
"WGS 84 / UTM zone 49S",
"WGS 84 / UTM zone 50S",
"WGS 84 / UTM zone 51S",
"WGS 84 / UTM zone 52S",
"WGS 84 / UTM zone 53S",
"WGS 84 / UTM zone 54S",
"WGS 84 / UTM zone 55S",
"WGS 84 / UTM zone 56S",
"WGS 84 / UTM zone 57S",
"WGS 84 / UTM zone 58S",
"WGS 84 / UTM zone 59S",
"WGS 84 / UTM zone 60S",
"WGS 84 / UPS South (N,E)",
"WGS 84 / TM 36 SE",
NULL
};


BOOL CRScheck::set_projection_from_ProjectedCSTypeGeoKey(const U16 value, CHAR* description)
{
  I32 ellipsoid_id = -1;
  BOOL utm_northern = FALSE;
  I32 utm_zone = -1;
  BOOL is_mga = FALSE;
  CHAR* sp = 0;
  BOOL sp_nad27 = FALSE;

  switch (value)
  {
  case 3154: // NAD83(CSRS) / UTM zone 7N
  case 3155: // NAD83(CSRS) / UTM zone 8N
  case 3156: // NAD83(CSRS) / UTM zone 9N
  case 3157: // NAD83(CSRS) / UTM zone 10N
    utm_northern = TRUE; utm_zone = value - 3154 + 7;
    ellipsoid_id = CRS_ELLIPSOID_NAD83;
    break;
  case 3158: // NAD83(CSRS) / UTM zone 14N
  case 3159: // NAD83(CSRS) / UTM zone 15N
  case 3160: // NAD83(CSRS) / UTM zone 16N
    utm_northern = TRUE; utm_zone = value - 3158 + 14;
    ellipsoid_id = CRS_ELLIPSOID_NAD83;
    break;
  case 20137: // PCS_Adindan_UTM_zone_37N
  case 20138: // PCS_Adindan_UTM_zone_38N
    utm_northern = TRUE; utm_zone = value-20100;
    break;
  case 20437: // PCS_Ain_el_Abd_UTM_zone_37N
  case 20438: // PCS_Ain_el_Abd_UTM_zone_38N
  case 20439: // PCS_Ain_el_Abd_UTM_zone_39N
    utm_northern = TRUE; utm_zone = value-20400;
    break;                
  case 20538: // PCS_Afgooye_UTM_zone_38N
  case 20539: // PCS_Afgooye_UTM_zone_39N
    utm_northern = TRUE; utm_zone = value-20500;
    break;
  case 20822: // PCS_Aratu_UTM_zone_22S
  case 20823: // PCS_Aratu_UTM_zone_23S
  case 20824: // PCS_Aratu_UTM_zone_24S
    utm_northern = FALSE; utm_zone = value-20800;
    break;
  case 21148: // PCS_Batavia_UTM_zone_48S
  case 21149: // PCS_Batavia_UTM_zone_49S
  case 21150: // PCS_Batavia_UTM_zone_50S
    utm_northern = FALSE; utm_zone = value-21100;
    break;
  case 21817: // PCS_Bogota_UTM_zone_17N
  case 21818: // PCS_Bogota_UTM_zone_18N
    utm_northern = TRUE; utm_zone = value-21800;
    break;
  case 22032: // PCS_Camacupa_UTM_32S
  case 22033: // PCS_Camacupa_UTM_33S
    utm_northern = FALSE; utm_zone = value-22000;
    break; 
  case 22332: // PCS_Carthage_UTM_zone_32N
    utm_northern = TRUE; utm_zone = 32;
    break; 
  case 22523: // PCS_Corrego_Alegre_UTM_23S
  case 22524: // PCS_Corrego_Alegre_UTM_24S
    utm_northern = FALSE; utm_zone = value-22500;
    break;
  case 22832: // PCS_Douala_UTM_zone_32N
    utm_northern = TRUE; utm_zone = 32;
    break;
  case 23028: // PCS_ED50_UTM_zone_28N
  case 23029: // PCS_ED50_UTM_zone_29N
  case 23030: 
  case 23031: 
  case 23032: 
  case 23033: 
  case 23034: 
  case 23035: 
  case 23036: 
  case 23037: 
  case 23038: // PCS_ED50_UTM_zone_38N
    utm_northern = TRUE; utm_zone = value-23000;
    ellipsoid_id = CRS_ELLIPSOID_Inter;
    break;
  case 23239: // PCS_Fahud_UTM_zone_39N
  case 23240: // PCS_Fahud_UTM_zone_40N
    utm_northern = TRUE; utm_zone = value-23200;
    break;
  case 23433: // PCS_Garoua_UTM_zone_33N
    utm_northern = TRUE; utm_zone = 33;
    break;
  case 23846: // PCS_ID74_UTM_zone_46N
  case 23847: // PCS_ID74_UTM_zone_47N
  case 23848:
  case 23849:
  case 23850:
  case 23851:
  case 23852:
  case 23853: // PCS_ID74_UTM_zone_53N
    utm_northern = TRUE; utm_zone = value-23800;
    ellipsoid_id = CRS_ELLIPSOID_ID74;
    break;
  case 23886: // PCS_ID74_UTM_zone_46S
  case 23887: // PCS_ID74_UTM_zone_47S
  case 23888:
  case 23889:
  case 23890:
  case 23891:
  case 23892:
  case 23893:
  case 23894: // PCS_ID74_UTM_zone_54S
    utm_northern = FALSE; utm_zone = value-23840;
    ellipsoid_id = CRS_ELLIPSOID_ID74;
    break;
  case 23947: // PCS_Indian_1954_UTM_47N
  case 23948: // PCS_Indian_1954_UTM_48N
    utm_northern = TRUE; utm_zone = value-23900;
    break;
  case 24047: // PCS_Indian_1975_UTM_47N
  case 24048: // PCS_Indian_1975_UTM_48N
    utm_northern = TRUE; utm_zone = value-24000;
    break;
  case 24547: // PCS_Kertau_UTM_zone_47N
  case 24548: // PCS_Kertau_UTM_zone_48N
    utm_northern = TRUE; utm_zone = value-24500;
    break;
  case 24720: // PCS_La_Canoa_UTM_zone_20N
  case 24721: // PCS_La_Canoa_UTM_zone_21N
    utm_northern = TRUE; utm_zone = value-24700;
    break;
  case 24818: // PCS_PSAD56_UTM_zone_18N
  case 24819: // PCS_PSAD56_UTM_zone_19N
  case 24820: // PCS_PSAD56_UTM_zone_20N
  case 24821: // PCS_PSAD56_UTM_zone_21N
    utm_northern = TRUE; utm_zone = value-24800;
    break;
  case 24877: // PCS_PSAD56_UTM_zone_17S
  case 24878: // PCS_PSAD56_UTM_zone_18S
  case 24879: // PCS_PSAD56_UTM_zone_19S
  case 24880: // PCS_PSAD56_UTM_zone_20S
    utm_northern = FALSE; utm_zone = value-24860;
    break;
  case 25231: // PCS_Lome_UTM_zone_31N
    utm_northern = TRUE; utm_zone = 31;
    break;
  case 25828: // PCS_ETRS89_UTM_zone_28N
  case 25829: // PCS_ETRS89_UTM_zone_29N
  case 25830: // PCS_ETRS89_UTM_zone_30N
  case 25831: // PCS_ETRS89_UTM_zone_31N
  case 25832: // PCS_ETRS89_UTM_zone_32N
  case 25833: // PCS_ETRS89_UTM_zone_33N
  case 25834: // PCS_ETRS89_UTM_zone_34N
  case 25835: // PCS_ETRS89_UTM_zone_35N
  case 25836: // PCS_ETRS89_UTM_zone_36N
  case 25837: // PCS_ETRS89_UTM_zone_37N
  case 25838: // PCS_ETRS89_UTM_zone_38N
    utm_northern = TRUE; utm_zone = value-25800;
    ellipsoid_id = CRS_ELLIPSOID_NAD83;
    break;
  case 25932: // PCS_Malongo_1987_UTM_32S
    utm_northern = FALSE; utm_zone = 32;
    break;
  case 26237: // PCS_Massawa_UTM_zone_37N
    utm_northern = TRUE; utm_zone = 37;
    break;
  case 26331: // PCS_Minna_UTM_zone_31N
  case 26332: // PCS_Minna_UTM_zone_32N
    utm_northern = TRUE; utm_zone = value-26300;
    break;
  case 26432: // PCS_Mhast_UTM_zone_32S
    utm_northern = FALSE; utm_zone = 32;
    break;
  case 26632: // PCS_M_poraloko_UTM_32N
    utm_northern = TRUE; utm_zone = 32;
    break;
  case 26692: // PCS_Minna_UTM_zone_32S
    utm_northern = FALSE; utm_zone = 32;
    break;
  case 26703: // PCS_NAD27_UTM_zone_3N
  case 26704: // PCS_NAD27_UTM_zone_4N
  case 26705: // PCS_NAD27_UTM_zone_5N
  case 26706: // PCS_NAD27_UTM_zone_6N
  case 26707: // PCS_NAD27_UTM_zone_7N
  case 26708: // PCS_NAD27_UTM_zone_7N
  case 26709: // PCS_NAD27_UTM_zone_9N
  case 26710: // PCS_NAD27_UTM_zone_10N
  case 26711: // PCS_NAD27_UTM_zone_11N
  case 26712: // PCS_NAD27_UTM_zone_12N
  case 26713: // PCS_NAD27_UTM_zone_13N
  case 26714: // PCS_NAD27_UTM_zone_14N
  case 26715: // PCS_NAD27_UTM_zone_15N
  case 26716: // PCS_NAD27_UTM_zone_16N
  case 26717: // PCS_NAD27_UTM_zone_17N
  case 26718: // PCS_NAD27_UTM_zone_18N
  case 26719: // PCS_NAD27_UTM_zone_19N
  case 26720: // PCS_NAD27_UTM_zone_20N
  case 26721: // PCS_NAD27_UTM_zone_21N
  case 26722: // PCS_NAD27_UTM_zone_22N
    utm_northern = TRUE; utm_zone = value-26700;
    ellipsoid_id = CRS_ELLIPSOID_NAD27;
    break;
  case 26729: // PCS_NAD27_Alabama_East
    sp_nad27 = TRUE; sp = "AL_E";
    break;
  case 26730: // PCS_NAD27_Alabama_West
    sp_nad27 = TRUE; sp = "AL_W";
    break;
  case 26731: // PCS_NAD27_Alaska_zone_1
    sp_nad27 = TRUE; sp = "AK_1";
    break;
  case 26732: // PCS_NAD27_Alaska_zone_2
    sp_nad27 = TRUE; sp = "AK_2";
    break;
  case 26733: // PCS_NAD27_Alaska_zone_3
    sp_nad27 = TRUE; sp = "AK_3";
    break;
  case 26734: // PCS_NAD27_Alaska_zone_4
    sp_nad27 = TRUE; sp = "AK_4";
    break;
  case 26735: // PCS_NAD27_Alaska_zone_5
    sp_nad27 = TRUE; sp = "AK_5";
    break;
  case 26736: // PCS_NAD27_Alaska_zone_6
    sp_nad27 = TRUE; sp = "AK_6";
    break;
  case 26737: // PCS_NAD27_Alaska_zone_7
    sp_nad27 = TRUE; sp = "AK_7";
    break;
  case 26738: // PCS_NAD27_Alaska_zone_8
    sp_nad27 = TRUE; sp = "AK_8";
    break;
  case 26739: // PCS_NAD27_Alaska_zone_9
    sp_nad27 = TRUE; sp = "AK_9";
    break;
  case 26740: // PCS_NAD27_Alaska_zone_10
    sp_nad27 = TRUE; sp = "AK_10";
    break;
  case 26741: // PCS_NAD27_California_I
    sp_nad27 = TRUE; sp = "CA_I";
    break;
  case 26742: // PCS_NAD27_California_II
    sp_nad27 = TRUE; sp = "CA_II";
    break;
  case 26743: // PCS_NAD27_California_III
    sp_nad27 = TRUE; sp = "CA_III";
    break;
  case 26744: // PCS_NAD27_California_IV
    sp_nad27 = TRUE; sp = "CA_IV";
    break;
  case 26745: // PCS_NAD27_California_V
    sp_nad27 = TRUE; sp = "CA_V";
    break;
  case 26746: // PCS_NAD27_California_VI
    sp_nad27 = TRUE; sp = "CA_VI";
    break;
  case 26747: // PCS_NAD27_California_VII
    sp_nad27 = TRUE; sp = "CA_VII";
    break;
  case 26748: // PCS_NAD27_Arizona_East
    sp_nad27 = TRUE; sp = "AZ_E";
    break;
  case 26749: // PCS_NAD27_Arizona_Central
    sp_nad27 = TRUE; sp = "AZ_C";
    break;
  case 26750: // PCS_NAD27_Arizona_West
    sp_nad27 = TRUE; sp = "AZ_W";
    break;
  case 26751: // PCS_NAD27_Arkansas_North
    sp_nad27 = TRUE; sp = "AR_N";
    break;
  case 26752: // PCS_NAD27_Arkansas_South
    sp_nad27 = TRUE; sp = "AR_S";
    break;
  case 26753: // PCS_NAD27_Colorado_North
    sp_nad27 = TRUE; sp = "CO_N";
    break;
  case 26754: // PCS_NAD27_Colorado_Central
    sp_nad27 = TRUE; sp = "CO_C";
    break;
  case 26755: // PCS_NAD27_Colorado_South
    sp_nad27 = TRUE; sp = "CO_S";
    break;
  case 26756: // PCS_NAD27_Connecticut
    sp_nad27 = TRUE; sp = "CT";
    break;
  case 26757: // PCS_NAD27_Delaware
    sp_nad27 = TRUE; sp = "DE";
    break;
  case 26758: // PCS_NAD27_Florida_East
    sp_nad27 = TRUE; sp = "FL_E";
    break;
  case 26759: // PCS_NAD27_Florida_West
    sp_nad27 = TRUE; sp = "FL_W";
    break;
  case 26760: // PCS_NAD27_Florida_North
    sp_nad27 = TRUE; sp = "FL_N";
    break;
  case 26761: // PCS_NAD27_Hawaii_zone_1
    sp_nad27 = TRUE; sp = "HI_1";
    break;
  case 26762: // PCS_NAD27_Hawaii_zone_2
    sp_nad27 = TRUE; sp = "HI_2";
    break;
  case 26763: // PCS_NAD27_Hawaii_zone_3
    sp_nad27 = TRUE; sp = "HI_3";
    break;
  case 26764: // PCS_NAD27_Hawaii_zone_4
    sp_nad27 = TRUE; sp = "HI_4";
    break;
  case 26765: // PCS_NAD27_Hawaii_zone_5
    sp_nad27 = TRUE; sp = "HI_5";
    break;
  case 26766: // PCS_NAD27_Georgia_East
    sp_nad27 = TRUE; sp = "GA_E";
    break;
  case 26767: // PCS_NAD27_Georgia_West
    sp_nad27 = TRUE; sp = "GA_W";
    break;
  case 26768: // PCS_NAD27_Idaho_East
    sp_nad27 = TRUE; sp = "ID_E";
    break;
  case 26769: // PCS_NAD27_Idaho_Central
    sp_nad27 = TRUE; sp = "ID_C";
    break;
  case 26770: // PCS_NAD27_Idaho_West
    sp_nad27 = TRUE; sp = "ID_W";
    break;
  case 26771: // PCS_NAD27_Illinois_East
    sp_nad27 = TRUE; sp = "IL_E";
    break;
  case 26772: // PCS_NAD27_Illinois_West
    sp_nad27 = TRUE; sp = "IL_W";
    break;
  case 26773: // PCS_NAD27_Indiana_East
    sp_nad27 = TRUE; sp = "IN_E";
    break;
  case 26774: // PCS_NAD27_Indiana_West
    sp_nad27 = TRUE; sp = "IN_W";
    break;
  case 26775: // PCS_NAD27_Iowa_North
    sp_nad27 = TRUE; sp = "IA_N";
    break;
  case 26776: // PCS_NAD27_Iowa_South
    sp_nad27 = TRUE; sp = "IA_S";
    break;
  case 26777: // PCS_NAD27_Kansas_North
    sp_nad27 = TRUE; sp = "KS_N";
    break;
  case 26778: // PCS_NAD27_Kansas_South
    sp_nad27 = TRUE; sp = "KS_S";
    break;
  case 26779: // PCS_NAD27_Kentucky_North
    sp_nad27 = TRUE; sp = "KY_N";
    break;
  case 26780: // PCS_NAD27_Kentucky_South
    sp_nad27 = TRUE; sp = "KY_S";
    break;
  case 26781: // PCS_NAD27_Louisiana_North
    sp_nad27 = TRUE; sp = "LA_N";
    break;
  case 26782: // PCS_NAD27_Louisiana_South
    sp_nad27 = TRUE; sp = "LA_S";
    break;
  case 26783: // PCS_NAD27_Maine_East
    sp_nad27 = TRUE; sp = "ME_E";
    break;
  case 26784: // PCS_NAD27_Maine_West
    sp_nad27 = TRUE; sp = "ME_W";
    break;
  case 26785: // PCS_NAD27_Maryland
    sp_nad27 = TRUE; sp = "MD";
    break;
  case 26786: // PCS_NAD27_Massachusetts
    sp_nad27 = TRUE; sp = "M_M";
    break;
  case 26787: // PCS_NAD27_Massachusetts_Is
    sp_nad27 = TRUE; sp = "M_I";
    break;
  case 26788: // PCS_NAD27_Michigan_North
    sp_nad27 = TRUE; sp = "MI_N";
    break;
  case 26789: // PCS_NAD27_Michigan_Central
    sp_nad27 = TRUE; sp = "MI_C";
    break;
  case 26790: // PCS_NAD27_Michigan_South
    sp_nad27 = TRUE; sp = "MI_S";
    break;
  case 26791: // PCS_NAD27_Minnesota_North
    sp_nad27 = TRUE; sp = "MN_N";
    break;
  case 26792: // PCS_NAD27_Minnesota_Cent
    sp_nad27 = TRUE; sp = "MN_C";
    break;
  case 26793: // PCS_NAD27_Minnesota_South
    sp_nad27 = TRUE; sp = "MN_S";
    break;
  case 26794: // PCS_NAD27_Mississippi_East
    sp_nad27 = TRUE; sp = "MS_E";
    break;
  case 26795: // PCS_NAD27_Mississippi_West
    sp_nad27 = TRUE; sp = "MS_W";
    break;
  case 26796: // PCS_NAD27_Missouri_East
    sp_nad27 = TRUE; sp = "MO_E";
    break;
  case 26797: // PCS_NAD27_Missouri_Central
    sp_nad27 = TRUE; sp = "MO_C";
    break;
  case 26798: // PCS_NAD27_Missouri_West
    sp_nad27 = TRUE; sp = "MO_W";
    break;
  case 26903: // PCS_NAD83_UTM_zone_3N
  case 26904: // PCS_NAD83_UTM_zone_4N
  case 26905: // PCS_NAD83_UTM_zone_5N
  case 26906: // PCS_NAD83_UTM_zone_6N
  case 26907: // PCS_NAD83_UTM_zone_7N
  case 26908: // PCS_NAD83_UTM_zone_7N
  case 26909: // PCS_NAD83_UTM_zone_9N
  case 26910: // PCS_NAD83_UTM_zone_10N
  case 26911: // PCS_NAD83_UTM_zone_11N
  case 26912: // PCS_NAD83_UTM_zone_12N
  case 26913: // PCS_NAD83_UTM_zone_13N
  case 26914: // PCS_NAD83_UTM_zone_14N
  case 26915: // PCS_NAD83_UTM_zone_15N
  case 26916: // PCS_NAD83_UTM_zone_16N
  case 26917: // PCS_NAD83_UTM_zone_17N
  case 26918: // PCS_NAD83_UTM_zone_18N
  case 26919: // PCS_NAD83_UTM_zone_19N
  case 26920: // PCS_NAD83_UTM_zone_20N
  case 26921: // PCS_NAD83_UTM_zone_21N
  case 26922: // PCS_NAD83_UTM_zone_22N
  case 26923: // PCS_NAD83_UTM_zone_23N
    utm_northern = TRUE; utm_zone = value-26900;
    ellipsoid_id = CRS_ELLIPSOID_NAD83;
    break;
  case 26929: // PCS_NAD83_Alabama_East
    sp_nad27 = FALSE; sp = "AL_E";
    break;
  case 26930: // PCS_NAD83_Alabama_West
    sp_nad27 = FALSE; sp = "AL_W";
    break;
  case 26931: // PCS_NAD83_Alaska_zone_1
    sp_nad27 = FALSE; sp = "AK_1";
    break;
  case 26932: // PCS_NAD83_Alaska_zone_2
    sp_nad27 = FALSE; sp = "AK_2";
    break;
  case 26933: // PCS_NAD83_Alaska_zone_3
    sp_nad27 = FALSE; sp = "AK_3";
    break;
  case 26934: // PCS_NAD83_Alaska_zone_4
    sp_nad27 = FALSE; sp = "AK_4";
    break;
  case 26935: // PCS_NAD83_Alaska_zone_5
    sp_nad27 = FALSE; sp = "AK_5";
    break;
  case 26936: // PCS_NAD83_Alaska_zone_6
    sp_nad27 = FALSE; sp = "AK_6";
    break;
  case 26937: // PCS_NAD83_Alaska_zone_7
    sp_nad27 = FALSE; sp = "AK_7";
    break;
  case 26938: // PCS_NAD83_Alaska_zone_8
    sp_nad27 = FALSE; sp = "AK_8";
    break;
  case 26939: // PCS_NAD83_Alaska_zone_9
    sp_nad27 = FALSE; sp = "AK_9";
    break;
  case 26940: // PCS_NAD83_Alaska_zone_10
    sp_nad27 = FALSE; sp = "AK_10";
    break;
  case 26941: // PCS_NAD83_California_I
    sp_nad27 = FALSE; sp = "CA_I";
    break;
  case 26942: // PCS_NAD83_California_II
    sp_nad27 = FALSE; sp = "CA_II";
    break;
  case 26943: // PCS_NAD83_California_III
    sp_nad27 = FALSE; sp = "CA_III";
    break;
  case 26944: // PCS_NAD83_California_IV
    sp_nad27 = FALSE; sp = "CA_IV";
    break;
  case 26945: // PCS_NAD83_California_V
    sp_nad27 = FALSE; sp = "CA_V";
    break;
  case 26946: // PCS_NAD83_California_VI
    sp_nad27 = FALSE; sp = "CA_VI";
    break;
  case 26947: // PCS_NAD83_California_VII
    sp_nad27 = FALSE; sp = "CA_VII";
    break;
  case 26948: // PCS_NAD83_Arizona_East
    sp_nad27 = FALSE; sp = "AZ_E";
    break;
  case 26949: // PCS_NAD83_Arizona_Central
    sp_nad27 = FALSE; sp = "AZ_C";
    break;
  case 26950: // PCS_NAD83_Arizona_West
    sp_nad27 = FALSE; sp = "AZ_W";
    break;
  case 26951: // PCS_NAD83_Arkansas_North
    sp_nad27 = FALSE; sp = "AR_N";
    break;
  case 26952: // PCS_NAD83_Arkansas_South
    sp_nad27 = FALSE; sp = "AR_S";
    break;
  case 26953: // PCS_NAD83_Colorado_North
    sp_nad27 = FALSE; sp = "CO_N";
    break;
  case 26954: // PCS_NAD83_Colorado_Central
    sp_nad27 = FALSE; sp = "CO_C";
    break;
  case 26955: // PCS_NAD83_Colorado_South
    sp_nad27 = FALSE; sp = "CO_S";
    break;
  case 26956: // PCS_NAD83_Connecticut
    sp_nad27 = FALSE; sp = "CT";
    break;
  case 26957: // PCS_NAD83_Delaware
    sp_nad27 = FALSE; sp = "DE";
    break;
  case 26958: // PCS_NAD83_Florida_East
    sp_nad27 = FALSE; sp = "FL_E";
    break;
  case 26959: // PCS_NAD83_Florida_West
    sp_nad27 = FALSE; sp = "FL_W";
    break;
  case 26960: // PCS_NAD83_Florida_North
    sp_nad27 = FALSE; sp = "FL_N";
    break;
  case 26961: // PCS_NAD83_Hawaii_zone_1
    sp_nad27 = FALSE; sp = "HI_1";
    break;
  case 26962: // PCS_NAD83_Hawaii_zone_2
    sp_nad27 = FALSE; sp = "HI_2";
    break;
  case 26963: // PCS_NAD83_Hawaii_zone_3
    sp_nad27 = FALSE; sp = "HI_3";
    break;
  case 26964: // PCS_NAD83_Hawaii_zone_4
    sp_nad27 = FALSE; sp = "HI_4";
    break;
  case 26965: // PCS_NAD83_Hawaii_zone_5
    sp_nad27 = FALSE; sp = "HI_5";
    break;
  case 26966: // PCS_NAD83_Georgia_East
    sp_nad27 = FALSE; sp = "GA_E";
    break;
  case 26967: // PCS_NAD83_Georgia_West
    sp_nad27 = FALSE; sp = "GA_W";
    break;
  case 26968: // PCS_NAD83_Idaho_East
    sp_nad27 = FALSE; sp = "ID_E";
    break;
  case 26969: // PCS_NAD83_Idaho_Central
    sp_nad27 = FALSE; sp = "ID_C";
    break;
  case 26970: // PCS_NAD83_Idaho_West
    sp_nad27 = FALSE; sp = "ID_W";
    break;
  case 26971: // PCS_NAD83_Illinois_East
    sp_nad27 = FALSE; sp = "IL_E";
    break;
  case 26972: // PCS_NAD83_Illinois_West
    sp_nad27 = FALSE; sp = "IL_W";
    break;
  case 26973: // PCS_NAD83_Indiana_East
    sp_nad27 = FALSE; sp = "IN_E";
    break;
  case 26974: // PCS_NAD83_Indiana_West
    sp_nad27 = FALSE; sp = "IN_W";
    break;
  case 26975: // PCS_NAD83_Iowa_North
    sp_nad27 = FALSE; sp = "IA_N";
    break;
  case 26976: // PCS_NAD83_Iowa_South
    sp_nad27 = FALSE; sp = "IA_S";
    break;
  case 26977: // PCS_NAD83_Kansas_North
    sp_nad27 = FALSE; sp = "KS_N";
    break;
  case 26978: // PCS_NAD83_Kansas_South
    sp_nad27 = FALSE; sp = "KS_S";
    break;
  case 26979: // PCS_NAD83_Kentucky_North
    sp_nad27 = FALSE; sp = "KY_N";
    break;
  case 26980: // PCS_NAD83_Kentucky_South
    sp_nad27 = FALSE; sp = "KY_S";
    break;
  case 26981: // PCS_NAD83_Louisiana_North
    sp_nad27 = FALSE; sp = "LA_N";
    break;
  case 26982: // PCS_NAD83_Louisiana_South
    sp_nad27 = FALSE; sp = "LA_S";
    break;
  case 26983: // PCS_NAD83_Maine_East
    sp_nad27 = FALSE; sp = "ME_E";
    break;
  case 26984: // PCS_NAD83_Maine_West
    sp_nad27 = FALSE; sp = "ME_W";
    break;
  case 26985: // PCS_NAD83_Maryland
    sp_nad27 = FALSE; sp = "MD";
    break;
  case 26986: // PCS_NAD83_Massachusetts
    sp_nad27 = FALSE; sp = "M_M";
    break;
  case 26987: // PCS_NAD83_Massachusetts_Is
    sp_nad27 = FALSE; sp = "M_I";
    break;
  case 26988: // PCS_NAD83_Michigan_North
    sp_nad27 = FALSE; sp = "MI_N";
    break;
  case 26989: // PCS_NAD83_Michigan_Central
    sp_nad27 = FALSE; sp = "MI_C";
    break;
  case 26990: // PCS_NAD83_Michigan_South
    sp_nad27 = FALSE; sp = "MI_S";
    break;
  case 26991: // PCS_NAD83_Minnesota_North
    sp_nad27 = FALSE; sp = "MN_N";
    break;
  case 26992: // PCS_NAD83_Minnesota_Cent
    sp_nad27 = FALSE; sp = "MN_C";
    break;
  case 26993: // PCS_NAD83_Minnesota_South
    sp_nad27 = FALSE; sp = "MN_S";
    break;
  case 26994: // PCS_NAD83_Mississippi_East
    sp_nad27 = FALSE; sp = "MS_E";
    break;
  case 26995: // PCS_NAD83_Mississippi_West
    sp_nad27 = FALSE; sp = "MS_W";
    break;
  case 26996: // PCS_NAD83_Missouri_East
    sp_nad27 = FALSE; sp = "MO_E";
    break;
  case 26997: // PCS_NAD83_Missouri_Central
    sp_nad27 = FALSE; sp = "MO_C";
    break;
  case 26998: // PCS_NAD83_Missouri_West
    sp_nad27 = FALSE; sp = "MO_W";
    break;
  case 27700:

    break;
  case 28348: // PCS_GDA94_MGA_zone_48
  case 28349:
  case 28350:
  case 28351:
  case 28352:
  case 28353:
  case 28354: // PCS_GDA94_MGA_zone_54
  case 28355: // PCS_GDA94_MGA_zone_55
  case 28356: // PCS_GDA94_MGA_zone_56
  case 28357: // PCS_GDA94_MGA_zone_57
  case 28358: // PCS_GDA94_MGA_zone_58
    utm_northern = FALSE; utm_zone = value-28300; is_mga = TRUE;
    ellipsoid_id = CRS_ELLIPSOID_GDA94;
    break;
  case 29118: // PCS_SAD69_UTM_zone_18N
  case 29119: // PCS_SAD69_UTM_zone_19N
  case 29120: // PCS_SAD69_UTM_zone_20N
  case 29121: // PCS_SAD69_UTM_zone_21N
  case 29122: // PCS_SAD69_UTM_zone_22N
    utm_northern = TRUE; utm_zone = value-29100;
    ellipsoid_id = CRS_ELLIPSOID_SAD69;
    break;
  case 29177: // PCS_SAD69_UTM_zone_17S
  case 29178: // PCS_SAD69_UTM_zone_18S
  case 29179: // PCS_SAD69_UTM_zone_19S
  case 29180: // PCS_SAD69_UTM_zone_20S
  case 29181: // PCS_SAD69_UTM_zone_21S
  case 29182: // PCS_SAD69_UTM_zone_22S
  case 29183: // PCS_SAD69_UTM_zone_23S
  case 29184: // PCS_SAD69_UTM_zone_24S
  case 29185: // PCS_SAD69_UTM_zone_25S
    utm_northern = FALSE; utm_zone = value-29160;
    ellipsoid_id = CRS_ELLIPSOID_SAD69;
    break;
  case 29220: // PCS_Sapper_Hill_UTM_20S
  case 29221: // PCS_Sapper_Hill_UTM_21S
    utm_northern = FALSE; utm_zone = value-29200;
    break;
  case 29333: // PCS_Schwarzeck_UTM_33S
    utm_northern = FALSE; utm_zone = 33;
    break;
  case 29635: // PCS_Sudan_UTM_zone_35N
  case 29636: // PCS_Sudan_UTM_zone_35N
    utm_northern = TRUE; utm_zone = value-29600;
    break;
  case 29738: // PCS_Tananarive_UTM_38S
  case 29739: // PCS_Tananarive_UTM_39S
    utm_northern = FALSE; utm_zone = value-29700;
    break;
  case 29849: // PCS_Timbalai_1948_UTM_49N
  case 29850: // PCS_Timbalai_1948_UTM_50N
    utm_northern = TRUE; utm_zone = value-29800;
    break;
  case 30339: // PCS_TC_1948_UTM_zone_39N
  case 30340: // PCS_TC_1948_UTM_zone_40N
    utm_northern = TRUE; utm_zone = value-30300;
    break;
  case 30729: // PCS_Nord_Sahara_UTM_29N
  case 30730: // PCS_Nord_Sahara_UTM_30N
  case 30731: // PCS_Nord_Sahara_UTM_31N
  case 30732: // PCS_Nord_Sahara_UTM_32N
    utm_northern = TRUE; utm_zone = value-30700;
    break;
  case 31028: // PCS_Yoff_UTM_zone_28N
    utm_northern = TRUE; utm_zone = 28;
    break;
  case 31121: // PCS_Zanderij_UTM_zone_21N
    utm_northern = TRUE; utm_zone = 21;
    break;
  case 32001: // PCS_NAD27_Montana_North
    sp_nad27 = TRUE; sp = "MT_N";
    break;
  case 32002: // PCS_NAD27_Montana_Central
    sp_nad27 = TRUE; sp = "MT_C";
    break;
  case 32003: // PCS_NAD27_Montana_South
    sp_nad27 = TRUE; sp = "MT_S";
    break;
  case 32005: // PCS_NAD27_Nebraska_North
    sp_nad27 = TRUE; sp = "NE_N";
    break;
  case 32006: // PCS_NAD27_Nebraska_South
    sp_nad27 = TRUE; sp = "NE_S";
    break;
  case 32007: // PCS_NAD27_Nevada_East
    sp_nad27 = TRUE; sp = "NV_E";
    break;
  case 32008: // PCS_NAD27_Nevada_Central
    sp_nad27 = TRUE; sp = "NV_C";
    break;
  case 32009: // PCS_NAD27_Nevada_West
    sp_nad27 = TRUE; sp = "NV_W";
    break;
  case 32010: // PCS_NAD27_New_Hampshire
    sp_nad27 = TRUE; sp = "NH";
    break;
  case 32011: // PCS_NAD27_New_Jersey
    sp_nad27 = TRUE; sp = "NJ";
    break;
  case 32012: // PCS_NAD27_New_Mexico_East
    sp_nad27 = TRUE; sp = "NM_E";
    break;
  case 32013: // PCS_NAD27_New_Mexico_Cent
    sp_nad27 = TRUE; sp = "NM_C";
    break;
  case 32014: // PCS_NAD27_New_Mexico_West
    sp_nad27 = TRUE; sp = "NM_W";
    break;
  case 32015: // PCS_NAD27_New_York_East
    sp_nad27 = TRUE; sp = "NY_E";
    break;
  case 32016: // PCS_NAD27_New_York_Central
    sp_nad27 = TRUE; sp = "NY_C";
    break;
  case 32017: // PCS_NAD27_New_York_West
    sp_nad27 = TRUE; sp = "NY_W";
    break;
  case 32018: // PCS_NAD27_New_York_Long_Is
    sp_nad27 = TRUE; sp = "NT_LI";
    break;
  case 32019: // PCS_NAD27_North_Carolina
    sp_nad27 = TRUE; sp = "NC";
    break;
  case 32020: // PCS_NAD27_North_Dakota_N
    sp_nad27 = TRUE; sp = "ND_N";
    break;
  case 32021: // PCS_NAD27_North_Dakota_S
    sp_nad27 = TRUE; sp = "ND_S";
    break;
  case 32022: // PCS_NAD27_Ohio_North
    sp_nad27 = TRUE; sp = "OH_N";
    break;
  case 32023: // PCS_NAD27_Ohio_South
    sp_nad27 = TRUE; sp = "OH_S";
    break;
  case 32024: // PCS_NAD27_Oklahoma_North
    sp_nad27 = TRUE; sp = "OK_N";
    break;
  case 32025: // PCS_NAD27_Oklahoma_South
    sp_nad27 = TRUE; sp = "OK_S";
    break;
  case 32026: // PCS_NAD27_Oregon_North
    sp_nad27 = TRUE; sp = "OR_N";
    break;
  case 32027: // PCS_NAD27_Oregon_South
    sp_nad27 = TRUE; sp = "OR_S";
    break;
  case 32028: // PCS_NAD27_Pennsylvania_N
    sp_nad27 = TRUE; sp = "PA_N";
    break;
  case 32029: // PCS_NAD27_Pennsylvania_S
    sp_nad27 = TRUE; sp = "PA_S";
    break;
  case 32030: // PCS_NAD27_Rhode_Island
    sp_nad27 = TRUE; sp = "RI";
    break;
  case 32031: // PCS_NAD27_South_Carolina_N
    sp_nad27 = TRUE; sp = "SC_N";
    break;
  case 32033: // PCS_NAD27_South_Carolina_S
    sp_nad27 = TRUE; sp = "SC_S";
    break;
  case 32034: // PCS_NAD27_South_Dakota_N
    sp_nad27 = TRUE; sp = "SD_N";
    break;
  case 32035: // PCS_NAD27_South_Dakota_S
    sp_nad27 = TRUE; sp = "SD_S";
    break;
  case 32036: // PCS_NAD27_Tennessee
    sp_nad27 = TRUE; sp = "TN";
    break;
  case 32037: // PCS_NAD27_Texas_North
    sp_nad27 = TRUE; sp = "TX_N";
    break;
  case 32038: // PCS_NAD27_Texas_North_Cen
    sp_nad27 = TRUE; sp = "TX_NC";
    break;
  case 32039: // PCS_NAD27_Texas_Central
    sp_nad27 = TRUE; sp = "TX_C";
    break;
  case 32040: // PCS_NAD27_Texas_South_Cen
    sp_nad27 = TRUE; sp = "TX_SC";
    break;
  case 32041: // PCS_NAD27_Texas_South
    sp_nad27 = TRUE; sp = "TX_S";
    break;
  case 32042: // PCS_NAD27_Utah_North
    sp_nad27 = TRUE; sp = "UT_N";
    break;
  case 32043: // PCS_NAD27_Utah_Central
    sp_nad27 = TRUE; sp = "UT_C";
    break;
  case 32044: // PCS_NAD27_Utah_South
    sp_nad27 = TRUE; sp = "UT_S";
    break;
  case 32045: // PCS_NAD27_Vermont
    sp_nad27 = TRUE; sp = "VT";
    break;
  case 32046: // PCS_NAD27_Virginia_North
    sp_nad27 = TRUE; sp = "VA_N";
    break;
  case 32047: // PCS_NAD27_Virginia_South
    sp_nad27 = TRUE; sp = "VA_S";
    break;
  case 32048: // PCS_NAD27_Washington_North
    sp_nad27 = TRUE; sp = "WA_N";
    break;
  case 32049: // PCS_NAD27_Washington_South
    sp_nad27 = TRUE; sp = "WA_S";
    break;
  case 32050: // PCS_NAD27_West_Virginia_N
    sp_nad27 = TRUE; sp = "WV_N";
    break;
  case 32051: // PCS_NAD27_West_Virginia_S
    sp_nad27 = TRUE; sp = "WV_S";
    break;
  case 32052: // PCS_NAD27_Wisconsin_North
    sp_nad27 = TRUE; sp = "WI_N";
    break;
  case 32053: // PCS_NAD27_Wisconsin_Cen
    sp_nad27 = TRUE; sp = "WI_C";
    break;
  case 32054: // PCS_NAD27_Wisconsin_South
    sp_nad27 = TRUE; sp = "WI_S";
    break;
  case 32055: // PCS_NAD27_Wyoming_East
    sp_nad27 = TRUE; sp = "WY_E";
    break;
  case 32056: // PCS_NAD27_Wyoming_E_Cen
    sp_nad27 = TRUE; sp = "WY_EC";
    break;
  case 32057: // PCS_NAD27_Wyoming_W_Cen
    sp_nad27 = TRUE; sp = "WY_WC";
    break;
  case 32058: // PCS_NAD27_Wyoming_West
    sp_nad27 = TRUE; sp = "WY_W";
    break;
  case 32059: // PCS_NAD27_Puerto_Rico
    sp_nad27 = TRUE; sp = "PR";
    break;
  case 32060: // PCS_NAD27_St_Croix
    sp_nad27 = TRUE; sp = "St.Croix";
    break;
  case 32100: // PCS_NAD83_Montana
    sp_nad27 = FALSE; sp = "MT";
    break;
  case 32104: // PCS_NAD83_Nebraska
    sp_nad27 = FALSE; sp = "NE";
    break;
  case 32107: // PCS_NAD83_Nevada_East
    sp_nad27 = FALSE; sp = "NV_E";
    break;
  case 32108: // PCS_NAD83_Nevada_Central
    sp_nad27 = FALSE; sp = "NV_C";
    break;
  case 32109: // PCS_NAD83_Nevada_West
    sp_nad27 = FALSE; sp = "NV_W";
    break;
  case 32110: // PCS_NAD83_New_Hampshire
    sp_nad27 = FALSE; sp = "NH";
    break;
  case 32111: // PCS_NAD83_New_Jersey
    sp_nad27 = FALSE; sp = "NJ";
    break;
  case 32112: // PCS_NAD83_New_Mexico_East
    sp_nad27 = FALSE; sp = "NM_E";
    break;
  case 32113: // PCS_NAD83_New_Mexico_Cent
    sp_nad27 = FALSE; sp = "NM_C";
    break;
  case 32114: // PCS_NAD83_New_Mexico_West
    sp_nad27 = FALSE; sp = "NM_W";
    break;
  case 32115: // PCS_NAD83_New_York_East
    sp_nad27 = FALSE; sp = "NY_E";
    break;
  case 32116: // PCS_NAD83_New_York_Central
    sp_nad27 = FALSE; sp = "NY_C";
    break;
  case 32117: // PCS_NAD83_New_York_West
    sp_nad27 = FALSE; sp = "NY_W";
    break;
  case 32118: // PCS_NAD83_New_York_Long_Is
    sp_nad27 = FALSE; sp = "NT_LI";
    break;
  case 32119: // PCS_NAD83_North_Carolina
    sp_nad27 = FALSE; sp = "NC";
    break;
  case 32120: // PCS_NAD83_North_Dakota_N
    sp_nad27 = FALSE; sp = "ND_N";
    break;
  case 32121: // PCS_NAD83_North_Dakota_S
    sp_nad27 = FALSE; sp = "ND_S";
    break;
  case 32122: // PCS_NAD83_Ohio_North
    sp_nad27 = FALSE; sp = "OH_N";
    break;
  case 32123: // PCS_NAD83_Ohio_South
    sp_nad27 = FALSE; sp = "OH_S";
    break;
  case 32124: // PCS_NAD83_Oklahoma_North
    sp_nad27 = FALSE; sp = "OK_N";
    break;
  case 32125: // PCS_NAD83_Oklahoma_South
    sp_nad27 = FALSE; sp = "OK_S";
    break;
  case 32126: // PCS_NAD83_Oregon_North
    sp_nad27 = FALSE; sp = "OR_N";
    break;
  case 32127: // PCS_NAD83_Oregon_South
    sp_nad27 = FALSE; sp = "OR_S";
    break;
  case 32128: // PCS_NAD83_Pennsylvania_N
    sp_nad27 = FALSE; sp = "PA_N";
    break;
  case 32129: // PCS_NAD83_Pennsylvania_S
    sp_nad27 = FALSE; sp = "PA_S";
    break;
  case 32130: // PCS_NAD83_Rhode_Island
    sp_nad27 = FALSE; sp = "RI";
    break;
  case 32133: // PCS_NAD83_South_Carolina
    sp_nad27 = FALSE; sp = "SC";
    break;
  case 32134: // PCS_NAD83_South_Dakota_N
    sp_nad27 = FALSE; sp = "SD_N";
    break;
  case 32135: // PCS_NAD83_South_Dakota_S
    sp_nad27 = FALSE; sp = "SD_S";
    break;
  case 32136: // PCS_NAD83_Tennessee
    sp_nad27 = FALSE; sp = "TN";
    break;
  case 32137: // PCS_NAD83_Texas_North
    sp_nad27 = FALSE; sp = "TX_N";
    break;
  case 32138: // PCS_NAD83_Texas_North_Cen
    sp_nad27 = FALSE; sp = "TX_NC";
    break;
  case 32139: // PCS_NAD83_Texas_Central
    sp_nad27 = FALSE; sp = "TX_C";
    break;
  case 32140: // PCS_NAD83_Texas_South_Cen
    sp_nad27 = FALSE; sp = "TX_SC";
    break;
  case 32141: // PCS_NAD83_Texas_South
    sp_nad27 = FALSE; sp = "TX_S";
    break;
  case 32142: // PCS_NAD83_Utah_North
    sp_nad27 = FALSE; sp = "UT_N";
    break;
  case 32143: // PCS_NAD83_Utah_Central
    sp_nad27 = FALSE; sp = "UT_C";
    break;
  case 32144: // PCS_NAD83_Utah_South
    sp_nad27 = FALSE; sp = "UT_S";
    break;
  case 32145: // PCS_NAD83_Vermont
    sp_nad27 = FALSE; sp = "VT";
    break;
  case 32146: // PCS_NAD83_Virginia_North
    sp_nad27 = FALSE; sp = "VA_N";
    break;
  case 32147: // PCS_NAD83_Virginia_South
    sp_nad27 = FALSE; sp = "VA_S";
    break;
  case 32148: // PCS_NAD83_Washington_North
    sp_nad27 = FALSE; sp = "WA_N";
    break;
  case 32149: // PCS_NAD83_Washington_South
    sp_nad27 = FALSE; sp = "WA_S";
    break;
  case 32150: // PCS_NAD83_West_Virginia_N
    sp_nad27 = FALSE; sp = "WV_N";
    break;
  case 32151: // PCS_NAD83_West_Virginia_S
    sp_nad27 = FALSE; sp = "WV_S";
    break;
  case 32152: // PCS_NAD83_Wisconsin_North
    sp_nad27 = FALSE; sp = "WI_N";
    break;
  case 32153: // PCS_NAD83_Wisconsin_Cen
    sp_nad27 = FALSE; sp = "WI_C";
    break;
  case 32154: // PCS_NAD83_Wisconsin_South
    sp_nad27 = FALSE; sp = "WI_S";
    break;
  case 32155: // PCS_NAD83_Wyoming_East
    sp_nad27 = FALSE; sp = "WY_E";
    break;
  case 32156: // PCS_NAD83_Wyoming_E_Cen
    sp_nad27 = FALSE; sp = "WY_EC";
    break;
  case 32157: // PCS_NAD83_Wyoming_W_Cen
    sp_nad27 = FALSE; sp = "WY_WC";
    break;
  case 32158: // PCS_NAD83_Wyoming_West
    sp_nad27 = FALSE; sp = "WY_W";
    break;
  case 32161: // PCS_NAD83_Puerto_Rico_Virgin_Is
    sp_nad27 = FALSE; sp = "PR";
    break;
  case 32201: // PCS_WGS72_UTM_zone_1N 
  case 32202: // PCS_WGS72_UTM_zone_2N 
  case 32203: // PCS_WGS72_UTM_zone_3N 
  case 32204: // PCS_WGS72_UTM_zone_4N 
  case 32205: // PCS_WGS72_UTM_zone_5N 
  case 32206: // PCS_WGS72_UTM_zone_6N 
  case 32207: // PCS_WGS72_UTM_zone_7N 
  case 32208:
  case 32209:
  case 32210:
  case 32211:
  case 32212:
  case 32213:
  case 32214:
  case 32215:
  case 32216:
  case 32217:
  case 32218:
  case 32219:
  case 32220:
  case 32221:
  case 32222:
  case 32223:
  case 32224:
  case 32225:
  case 32226:
  case 32227:
  case 32228:
  case 32229:
  case 32230:
  case 32231:
  case 32232:
  case 32233:
  case 32234:
  case 32235:
  case 32236:
  case 32237:
  case 32238:
  case 32239:
  case 32240:
  case 32241:
  case 32242:
  case 32243:
  case 32244:
  case 32245:
  case 32246:
  case 32247:
  case 32248:
  case 32249:
  case 32250:
  case 32251:
  case 32252:
  case 32253:
  case 32254:
  case 32255:
  case 32256:
  case 32257:
  case 32258:
  case 32259: // PCS_WGS72_UTM_zone_59N 
  case 32260: // PCS_WGS72_UTM_zone_60N 
    utm_northern = TRUE; utm_zone = value-32200;
    ellipsoid_id = CRS_ELLIPSOID_WGS72;
    break;
  case 32301: // PCS_WGS72_UTM_zone_1S
  case 32302: // PCS_WGS72_UTM_zone_2S
  case 32303: // PCS_WGS72_UTM_zone_3S
  case 32304: // PCS_WGS72_UTM_zone_4S
  case 32305: // PCS_WGS72_UTM_zone_5S
  case 32306: // PCS_WGS72_UTM_zone_6S
  case 32307: // PCS_WGS72_UTM_zone_7S
  case 32308:
  case 32309:
  case 32310:
  case 32311:
  case 32312:
  case 32313:
  case 32314:
  case 32315:
  case 32316:
  case 32317:
  case 32318:
  case 32319:
  case 32320:
  case 32321:
  case 32322:
  case 32323:
  case 32324:
  case 32325:
  case 32326:
  case 32327:
  case 32328:
  case 32329:
  case 32330:
  case 32331:
  case 32332:
  case 32333:
  case 32334:
  case 32335:
  case 32336:
  case 32337:
  case 32338:
  case 32339:
  case 32340:
  case 32341:
  case 32342:
  case 32343:
  case 32344:
  case 32345:
  case 32346:
  case 32347:
  case 32348:
  case 32349:
  case 32350:
  case 32351:
  case 32352:
  case 32353:
  case 32354:
  case 32355:
  case 32356:
  case 32357:
  case 32358:
  case 32359: // PCS_WGS72_UTM_zone_59S
  case 32360: // PCS_WGS72_UTM_zone_60S
    utm_northern = FALSE; utm_zone = value-32300;
    ellipsoid_id = CRS_ELLIPSOID_WGS72;
    break;
  case 32401: // PCS_WGS72BE_UTM_zone_1N
  case 32402: // PCS_WGS72BE_UTM_zone_2N
  case 32403: // PCS_WGS72BE_UTM_zone_3N
  case 32404: // PCS_WGS72BE_UTM_zone_4N
  case 32405: // PCS_WGS72BE_UTM_zone_5N
  case 32406: // PCS_WGS72BE_UTM_zone_6N
  case 32407: // PCS_WGS72BE_UTM_zone_7N
  case 32408:
  case 32409:
  case 32410:
  case 32411:
  case 32412:
  case 32413:
  case 32414:
  case 32415:
  case 32416:
  case 32417:
  case 32418:
  case 32419:
  case 32420:
  case 32421:
  case 32422:
  case 32423:
  case 32424:
  case 32425:
  case 32426:
  case 32427:
  case 32428:
  case 32429:
  case 32430:
  case 32431:
  case 32432:
  case 32433:
  case 32434:
  case 32435:
  case 32436:
  case 32437:
  case 32438:
  case 32439:
  case 32440:
  case 32441:
  case 32442:
  case 32443:
  case 32444:
  case 32445:
  case 32446:
  case 32447:
  case 32448:
  case 32449:
  case 32450:
  case 32451:
  case 32452:
  case 32453:
  case 32454:
  case 32455:
  case 32456:
  case 32457:
  case 32458:
  case 32459: // PCS_WGS72BE_UTM_zone_59N
  case 32460: // PCS_WGS72BE_UTM_zone_60N
    utm_northern = TRUE; utm_zone = value-32400;
    ellipsoid_id = CRS_ELLIPSOID_WGS72;
    break;
  case 32501: // PCS_WGS72BE_UTM_zone_1S
  case 32502: // PCS_WGS72BE_UTM_zone_2S
  case 32503: // PCS_WGS72BE_UTM_zone_3S
  case 32504: // PCS_WGS72BE_UTM_zone_4S
  case 32505: // PCS_WGS72BE_UTM_zone_5S
  case 32506: // PCS_WGS72BE_UTM_zone_6S
  case 32507: // PCS_WGS72BE_UTM_zone_7S
  case 32508:
  case 32509:
  case 32510:
  case 32511:
  case 32512:
  case 32513:
  case 32514:
  case 32515:
  case 32516:
  case 32517:
  case 32518:
  case 32519:
  case 32520:
  case 32521:
  case 32522:
  case 32523:
  case 32524:
  case 32525:
  case 32526:
  case 32527:
  case 32528:
  case 32529:
  case 32530:
  case 32531:
  case 32532:
  case 32533:
  case 32534:
  case 32535:
  case 32536:
  case 32537:
  case 32538:
  case 32539:
  case 32540:
  case 32541:
  case 32542:
  case 32543:
  case 32544:
  case 32545:
  case 32546:
  case 32547:
  case 32548:
  case 32549:
  case 32550:
  case 32551:
  case 32552:
  case 32553:
  case 32554:
  case 32555:
  case 32556:
  case 32557:
  case 32558:
  case 32559: // PCS_WGS72BE_UTM_zone_59S
  case 32560: // PCS_WGS72BE_UTM_zone_60S
    utm_northern = FALSE; utm_zone = value-32500;
    ellipsoid_id = CRS_ELLIPSOID_WGS72;
    break;
  case 32601: // PCS_WGS84_UTM_zone_1N
  case 32602: // PCS_WGS84_UTM_zone_2N
  case 32603: // PCS_WGS84_UTM_zone_3N
  case 32604: // PCS_WGS84_UTM_zone_4N
  case 32605: // PCS_WGS84_UTM_zone_5N
  case 32606: // PCS_WGS84_UTM_zone_6N
  case 32607: // PCS_WGS84_UTM_zone_7N
  case 32608:
  case 32609:
  case 32610:
  case 32611:
  case 32612:
  case 32613:
  case 32614:
  case 32615:
  case 32616:
  case 32617:
  case 32618:
  case 32619:
  case 32620:
  case 32621:
  case 32622:
  case 32623:
  case 32624:
  case 32625:
  case 32626:
  case 32627:
  case 32628:
  case 32629:
  case 32630:
  case 32631:
  case 32632:
  case 32633:
  case 32634:
  case 32635:
  case 32636:
  case 32637:
  case 32638:
  case 32639:
  case 32640:
  case 32641:
  case 32642:
  case 32643:
  case 32644:
  case 32645:
  case 32646:
  case 32647:
  case 32648:
  case 32649:
  case 32650:
  case 32651:
  case 32652:
  case 32653:
  case 32654:
  case 32655:
  case 32656:
  case 32657:
  case 32658:
  case 32659: // PCS_WGS84_UTM_zone_59N
  case 32660: // PCS_WGS84_UTM_zone_60N
    utm_northern = TRUE; utm_zone = value-32600;
    ellipsoid_id = CRS_ELLIPSOID_WGS84;
    break;
  case 32701: // PCS_WGS84_UTM_zone_1S
  case 32702: // PCS_WGS84_UTM_zone_2S
  case 32703: // PCS_WGS84_UTM_zone_3S
  case 32704: // PCS_WGS84_UTM_zone_4S
  case 32705: // PCS_WGS84_UTM_zone_5S
  case 32706: // PCS_WGS84_UTM_zone_6S
  case 32707: // PCS_WGS84_UTM_zone_7S
  case 32708:
  case 32709:
  case 32710:
  case 32711:
  case 32712:
  case 32713:
  case 32714:
  case 32715:
  case 32716:
  case 32717:
  case 32718:
  case 32719:
  case 32720:
  case 32721:
  case 32722:
  case 32723:
  case 32724:
  case 32725:
  case 32726:
  case 32727:
  case 32728:
  case 32729:
  case 32730:
  case 32731:
  case 32732:
  case 32733:
  case 32734:
  case 32735:
  case 32736:
  case 32737:
  case 32738:
  case 32739:
  case 32740:
  case 32741:
  case 32742:
  case 32743:
  case 32744:
  case 32745:
  case 32746:
  case 32747:
  case 32748:
  case 32749:
  case 32750:
  case 32751:
  case 32752:
  case 32753:
  case 32754:
  case 32755:
  case 32756:
  case 32757:
  case 32758:
  case 32759: // PCS_WGS84_UTM_zone_59S
  case 32760: // PCS_WGS84_UTM_zone_60S
    utm_northern = FALSE; utm_zone = value-32700;
    ellipsoid_id = CRS_ELLIPSOID_WGS84;
    break;
  }

  if (ellipsoid_id == -1)
  {
    if (utm_zone != -1) ellipsoid_id = CRS_ELLIPSOID_WGS84;
    else if (sp != 0) ellipsoid_id = (sp_nad27 ? CRS_ELLIPSOID_NAD27 : CRS_ELLIPSOID_NAD83);
  }

  if (ellipsoid_id != -1)
  {
    set_ellipsoid(ellipsoid_id, TRUE);
  }

  if (utm_zone != -1)
  {
    if (is_mga)
    {
      if (set_mga_projection(utm_zone, utm_northern, TRUE, description))
      {
        return TRUE;
      }
    }
    else
    {
      if (set_utm_projection(utm_zone, utm_northern, TRUE, description))
      {
        return TRUE;
      }
    }
  }

  if (sp)
  {
    if (sp_nad27)
    {
      if (set_state_plane_nad27_lcc(sp, TRUE, description))
      {
        return TRUE;
      }
      if (set_state_plane_nad27_tm(sp, TRUE, description))
      {
        return TRUE;
      }
    }
    else
    {
      if (set_state_plane_nad83_lcc(sp, TRUE, description))
      {
        return TRUE;
      }
      if (set_state_plane_nad83_tm(sp, TRUE, description))
      {
        return TRUE;
      }
    }
  }

  if (value == EPSG_IRENET95_Irish_Transverse_Mercator)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(600000.0, 750000.0, 53.5, -8.0, 0.99982, TRUE, 0); // "IRENET95 / Irish Transverse Mercator"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "IRENET95 / Irish Transverse Mercator");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_Poland_CS92)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, -5300000.0, 0.0, 19.0, 0.9993, TRUE, 0); // "ETRS89 / Poland CS92"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / Poland CS92");
    return TRUE;
  }
  else if (value == EPSG_NZGD2000)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(1600000.0, 10000000.0, 0.0, 173.0, 0.9996, TRUE, 0); // "NZGD2000 / New Zealand Transverse Mercator 2000"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "NZGD2000");
    return TRUE;
  }
  else if (value == EPSG_NAD83_Maryland_ftUS)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(400000.0, 0.0, 37.66666666666666, -77, 37.66666666666666, 38.3, TRUE, 0); // "NAD83 / Maryland (ftUS)"
    set_coordinates_in_survey_feet(TRUE);
    if (description) sprintf(description, "NAD83 / Maryland (ftUS)");
    return TRUE;
  }
  else if (value == EPSG_NAD83_HARN_UTM2_South_American_Samoa)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 10000000.0, 0.0, -171.0, 0.9996, TRUE, 0); // "NAD83(HARN) / UTM zone 2S (American Samoa)"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "UTM zone 2S (American Samoa)");
    return TRUE;
  }
  else if (value == EPSG_NAD83_HARN_Virginia_North_ftUS)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(3500000.0, 2000000.0, 37.66666666666666, -78.5, 39.2, 38.03333333333333, TRUE, 0); // "NAD83(HARN) / Virginia North (ftUS)"
    set_coordinates_in_survey_feet(TRUE);
    if (description) sprintf(description, "NAD83(HARN) / Virginia North (ftUS)");
    return TRUE;
  }
  else if (value == EPSG_NAD83_HARN_Virginia_South_ftUS)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(3500000.0, 1000000.0, 36.33333333333334, -78.5, 37.96666666666667, 36.76666666666667, TRUE, 0); // "NAD83(HARN) / Virginia South (ftUS)"
    set_coordinates_in_survey_feet(TRUE);
    if (description) sprintf(description, "NAD83(HARN) / Virginia South (ftUS)");
    return TRUE;
  }
  else if (value == EPSG_Reseau_Geodesique_Francais_Guyane_1995)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, -51.0, 0.9996, TRUE, 0); // "Reseau Geodesique Francais Guyane 1995"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Reseau Geodesique Francais Guyane 1995");
    return TRUE;
  }
  else if (value == EPSG_NAD83_Oregon_Lambert)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(400000.0, 0.0, 41.75, -120.5, 43.0, 45.5, TRUE, 0); // "NAD83 / Oregon Lambert"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "NAD83 / Oregon Lambert");
    return TRUE;
  }
  else if (value == EPSG_NAD83_Oregon_Lambert_ft)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(400000.0, 0.0, 41.75, -120.5, 43.0, 45.5, TRUE, 0); // "NAD83 / Oregon Lambert (ft)"
    set_coordinates_in_feet(TRUE);
    if (description) sprintf(description, "NAD83 / Oregon Lambert (ft)");
    return TRUE;
  }
  else if (value == EPSG_SWEREF99_TM)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 15.0, 0.9996, TRUE, 0); // "SWEREF99 TM"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "SWEREF99 TM");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_LCC)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(4000000.0, 2800000.0, 52.0, 10.0, 35.0, 65.0, TRUE, 0); // "ETRS89 / ETRS-LCC"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-LCC");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_TM34)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 21.0, 0.9996, TRUE, 0); // "ETRS89 / ETRS-TM34"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-TM34");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_TM35)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 27.0, 0.9996, TRUE, 0); // "ETRS89 / ETRS-TM35"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-TM35");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_TM36)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 33.0, 0.9996, TRUE, 0); // "ETRS89 / ETRS-TM36"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-TM36");
    return TRUE;
  }
  else if (value == EPSG_ETRS89_ETRS_TM35FIN)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, 0.0, 0.0, 27.0, 0.9996, TRUE, 0); // "ETRS89 / ETRS-TM35FIN"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / ETRS-TM35FIN");
    return TRUE;
  }
  else if (value == EPSG_Fiji_1956_UTM60_South)
  {
    set_ellipsoid(CRS_ELLIPSOID_Inter, TRUE);
    set_utm_projection(60, FALSE, TRUE, 0); // "Fiji 1956 / UTM zone 60S"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Fiji 1956 / UTM zone 60S");
  }
  else if (value == EPSG_Fiji_1956_UTM1_South)
  {
    set_ellipsoid(CRS_ELLIPSOID_Inter, TRUE);
    set_utm_projection(1, FALSE, TRUE, 0); // "Fiji 1956 / UTM zone 1S"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Fiji 1956 / UTM zone 1S");
  }
  else if (value == EPSG_Fiji_Map_Grid_1986)
  {
    set_ellipsoid(CRS_ELLIPSOID_WGS72, TRUE);
    set_transverse_mercator_projection(2000000.0, 4000000.0, -17.0, 178.75, 0.99985, TRUE, 0); // "Fiji 1986 / Fiji Map Grid"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Fiji 1986 / Fiji Map Grid");
    return TRUE;
  }
  else if (value == EPSG_NAD83_NSRS2007_Maryland_ftUS)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(400000.0, 0.0, 37.66666666666666, -77, 39.45, 38.3, TRUE, 0); // "NAD83(NSRS2007) / Maryland (ftUS)"
    set_coordinates_in_survey_feet(TRUE);
    if (description) sprintf(description, "NAD83(NSRS2007) / Maryland (ftUS)");
    return TRUE;
  }
  else if (value == EPSG_Slovene_National_Grid_1996)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(500000.0, -5000000.0, 0.0, 15.0, 0.9999, TRUE, 0); // "Slovenia 1996 / Slovene National Grid"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Slovenia 1996 / Slovene National Grid");
    return TRUE;
  }
  else if (value == EPSG_MGI_1901_Slovene_National_Grid)
  {
    set_ellipsoid(3, TRUE); // Bessel 1841
    set_transverse_mercator_projection(500000.0, -5000000.0, 0.0, 15.0, 0.9999, TRUE, 0); // "MGI 1901 / Slovene National Grid"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "MGI 1901 / Slovene National Grid");
    return TRUE;    
  }
  else if ((EPSG_RGF93_CC42 <= value) && (value <= EPSG_RGF93_CC50))
  {
    int v = value - EPSG_RGF93_CC42;
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_lambert_conformal_conic_projection(1700000.0, 1200000.0+v*1000000, 42.0+v, 3.0, 41.25+v, 42.75+v, TRUE, 0);
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "RGF93 / CC%d Reseau_Geodesique_Francais_1993", value-3900);
    return TRUE;
  }
  else if ((EPSG_ETRS89_DKTM1 <= value) && (value <= EPSG_ETRS89_DKTM4))
  {
    int v = ((value - EPSG_ETRS89_DKTM1)%4) + 1;
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(200000.0*v, -5000000.0, 0.0, 9.0 + (v == 1 ? 0.0 : (v == 2 ? 1.0 : (v == 3 ? 2.75 : 6.0))), 0.99998, TRUE, 0);
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / DKTM%d", v);
    return TRUE;
  }
  else if (value == EPSG_ETRS89_UTM32_north_zE_N)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(32500000.0, 0.0, 0.0, 9.0, 0.9996, TRUE, 0); // "ETRS89 / UTM zone 32N (zE-N)"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / UTM zone 32N (zE-N)");
    return TRUE;
  }
  else if ((EPSG_ETRS89_NTM_zone_5 <= value) && (value <= EPSG_ETRS89_NTM_zone_30))
  {
    int v = value - 5100;
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(100000.0, 1000000.0, 58.0, 0.3+v, 1.0, TRUE, 0);
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / NTM zone %d", v);
    return TRUE;
  }
  else if (value == EPSG_ETRS89_UTM33_north_zE_N)
  {
    set_ellipsoid(CRS_ELLIPSOID_NAD83, TRUE); // GRS 1980
    set_transverse_mercator_projection(33500000.0, 0.0, 0.0, 15.0, 0.9996, TRUE, 0); // "ETRS89 / UTM zone 33N (zE-N)"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "ETRS89 / UTM zone 33N (zE-N)");
    return TRUE;
  }
  else if (value == EPSG_OSGB_1936)
  {
    set_ellipsoid(1, TRUE); // Airy 1830
    set_transverse_mercator_projection(400000.0, -100000.0, 49.0, -2.0, 0.9996012717, TRUE, 0); // "OSGB 1936 / British National Grid"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "OSGB 1936 / British National Grid");
    return TRUE;
  } 
  else if (value == EPSG_Belgian_Lambert_1972)
  {
    set_ellipsoid(CRS_ELLIPSOID_Inter, TRUE);
    set_lambert_conformal_conic_projection(150000.013, 5400088.438, 90, 4.367486666666666, 51.16666723333333, 49.8333339, TRUE, 0); // "Belge 1972 / Belgian Lambert 72"
    set_coordinates_in_meter(TRUE);
    if (description) sprintf(description, "Belge 1972 / Belgian Lambert 72");
    return TRUE;
  }
  else if (value)
  {
    // check against list
    short code = 0;
    while (epsg_codes[code])
    {
      if (epsg_codes[code] == value)
      {
        if (description) sprintf(description, epsg_descriptions[code]);
        return TRUE;
      }
      code++;
    }
    if (description) sprintf(description, "unknown EPSG code %d. please email this particular LAS/LAZ file (or at least a lasinfo report of it) to 'lasvalidator@rapidlasso.com' to have this projection added to the CRS check.", (I32)value);
    return FALSE;
  }
  fprintf(stderr, "CRScheck::set_projection_from_ProjectedCSTypeGeoKey: %d not implemented\n", value);
  return FALSE;
}

BOOL CRScheck::check_geokeys(LASheader* lasheader, CHAR* description)
{
  BOOL has_projection = FALSE;
  BOOL user_defined_ellipsoid = FALSE;
  I32 user_defined_projection = 0;
  I32 offsetProjStdParallel1GeoKey = -1;
  I32 offsetProjStdParallel2GeoKey = -1;
  I32 offsetProjNatOriginLatGeoKey = -1;
  I32 offsetProjFalseEastingGeoKey = -1;
  I32 offsetProjFalseNorthingGeoKey = -1;
  I32 offsetProjCenterLongGeoKey = -1;
  I32 offsetProjScaleAtNatOriginGeoKey = -1;
  I32 ellipsoid = -1;

  U32 num_geokey_entries = lasheader->geokeys->number_of_keys;
  LASgeokey_entry* geokey_entries = lasheader->geokey_entries;
  F64* geokey_double_params = lasheader->geokey_double_params;

  for (U32 i = 0; i < num_geokey_entries; i++)
  {
    switch (geokey_entries[i].key_id)
    {
    case 1024: // GTModelTypeGeoKey
      if (geokey_entries[i].value_offset == 2) // ModelTypeGeographic
      {
        has_projection = set_longlat_projection(TRUE, description);
      }
      else if (geokey_entries[i].value_offset == 3) // ModelTypeGeocentric
      {
        has_projection = set_ecef_projection(TRUE, description);
      }
      else if (geokey_entries[i].value_offset == 0) // ModelTypeUndefined
      {
        has_projection = set_no_projection(TRUE, description);
      }
      break;
    case 2048: // GeographicTypeGeoKey
      switch (geokey_entries[i].value_offset)
      {
      case 32767: // user-defined GCS
        user_defined_ellipsoid = TRUE;
        break;
      case 4001: // GCSE_Airy1830
        ellipsoid = 1;
        break;
      case 4002: // GCSE_AiryModified1849 
        ellipsoid = 16;
        break;
      case 4003: // GCSE_AustralianNationalSpheroid
        ellipsoid = 2;
        break;
      case 4004: // GCSE_Bessel1841
      case 4005: // GCSE_Bessel1841Modified
        ellipsoid = 3;
        break;
      case 4006: // GCSE_BesselNamibia
        ellipsoid = 4;
        break;
      case 4008: // GCSE_Clarke1866
      case 4009: // GCSE_Clarke1866Michigan
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 4010: // GCSE_Clarke1880_Benoit
      case 4011: // GCSE_Clarke1880_IGN
      case 4012: // GCSE_Clarke1880_RGS
      case 4013: // GCSE_Clarke1880_Arc
      case 4014: // GCSE_Clarke1880_SGA1922
      case 4034: // GCSE_Clarke1880
        ellipsoid = 6;
        break;
      case 4015: // GCSE_Everest1830_1937Adjustment
      case 4016: // GCSE_Everest1830_1967Definition
      case 4017: // GCSE_Everest1830_1975Definition
        ellipsoid = 7;
        break;
      case 4018: // GCSE_Everest1830Modified
        ellipsoid = 17;
        break;
      case 4019: // GCSE_GRS1980
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 4020: // GCSE_Helmert1906
        ellipsoid = 12;
        break;
      case 4022: // GCSE_International1924
      case 4023: // GCSE_International1967
        ellipsoid = 14;
        break;
      case 4024: // GCSE_Krassowsky1940
        ellipsoid = 15;
        break;
      case 4030: // GCSE_WGS84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      case 4267: // GCS_NAD27
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 4269: // GCS_NAD83
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 4283: // GCS_GDA94
        ellipsoid = CRS_ELLIPSOID_GDA94;
        break;
      case 4322: // GCS_WGS_72
        ellipsoid = CRS_ELLIPSOID_WGS72;
        break;
      case 4326: // GCS_WGS_84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      default:
        fprintf(stderr, "GeographicTypeGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 2050: // GeogGeodeticDatumGeoKey 
      switch (geokey_entries[i].value_offset)
      {
      case 32767: // user-defined GCS
        user_defined_ellipsoid = TRUE;
        break;
      case 6202: // Datum_Australian_Geodetic_Datum_1966
      case 6203: // Datum_Australian_Geodetic_Datum_1984
        ellipsoid = 2;
        break;
      case 6267: // Datum_North_American_Datum_1927
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 6269: // Datum_North_American_Datum_1983
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 6283: // Datum_Geocentric_Datum_of_Australia_1994
        ellipsoid = CRS_ELLIPSOID_GDA94;
        break;
      case 6322: // Datum_WGS72
        ellipsoid = CRS_ELLIPSOID_WGS72;
        break;
      case 6326: // Datum_WGS84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      case 6001: // DatumE_Airy1830
        ellipsoid = 1;
        break;
      case 6002: // DatumE_AiryModified1849
        ellipsoid = 16;
        break;
      case 6003: // DatumE_AustralianNationalSpheroid
        ellipsoid = 2;
        break;
      case 6004: // DatumE_Bessel1841
      case 6005: // DatumE_BesselModified
        ellipsoid = 3;
        break;
      case 6006: // DatumE_BesselNamibia
        ellipsoid = 4;
        break;
      case 6008: // DatumE_Clarke1866
      case 6009: // DatumE_Clarke1866Michigan
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 6010: // DatumE_Clarke1880_Benoit
      case 6011: // DatumE_Clarke1880_IGN
      case 6012: // DatumE_Clarke1880_RGS
      case 6013: // DatumE_Clarke1880_Arc
      case 6014: // DatumE_Clarke1880_SGA1922
      case 6034: // DatumE_Clarke1880
        ellipsoid = 6;
        break;
      case 6015: // DatumE_Everest1830_1937Adjustment
      case 6016: // DatumE_Everest1830_1967Definition
      case 6017: // DatumE_Everest1830_1975Definition
        ellipsoid = 7;
        break;
      case 6018: // DatumE_Everest1830Modified
        ellipsoid = 17;
        break;
      case 6019: // DatumE_GRS1980
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 6020: // DatumE_Helmert1906
        ellipsoid = 12;
        break;
      case 6022: // DatumE_International1924
      case 6023: // DatumE_International1967
        ellipsoid = 14;
        break;
      case 6024: // DatumE_Krassowsky1940
        ellipsoid = 15;
        break;
      case 6030: // DatumE_WGS84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      default:
        fprintf(stderr, "GeogGeodeticDatumGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 2052: // GeogLinearUnitsGeoKey 
      switch (geokey_entries[i].value_offset)
      {
      case 9001: // Linear_Meter
        break;
      case 9002: // Linear_Foot
        break;
      case 9003: // Linear_Foot_US_Survey
        break;
      default:
        fprintf(stderr, "GeogLinearUnitsGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 2056: // GeogEllipsoidGeoKey
      switch (geokey_entries[i].value_offset)
      {
      case 7001: // Ellipse_Airy_1830
        ellipsoid = 1;
        break;
      case 7002: // Ellipse_Airy_Modified_1849
        ellipsoid = 16;
        break;
      case 7003: // Ellipse_Australian_National_Spheroid
        ellipsoid = 2;
        break;
      case 7004: // Ellipse_Bessel_1841
      case 7005: // Ellipse_Bessel_Modified
        ellipsoid = 3;
        break;
      case 7006: // Ellipse_Bessel_Namibia
        ellipsoid = 4;
        break;
      case 7008: // Ellipse_Clarke_1866
      case 7009: // Ellipse_Clarke_1866_Michigan
        ellipsoid = CRS_ELLIPSOID_NAD27;
        break;
      case 7010: // Ellipse_Clarke1880_Benoit
      case 7011: // Ellipse_Clarke1880_IGN
      case 7012: // Ellipse_Clarke1880_RGS
      case 7013: // Ellipse_Clarke1880_Arc
      case 7014: // Ellipse_Clarke1880_SGA1922
      case 7034: // Ellipse_Clarke1880
        ellipsoid = 6;
        break;
      case 7015: // Ellipse_Everest1830_1937Adjustment
      case 7016: // Ellipse_Everest1830_1967Definition
      case 7017: // Ellipse_Everest1830_1975Definition
        ellipsoid = 7;
        break;
      case 7018: // Ellipse_Everest1830Modified
        ellipsoid = 17;
        break;
      case 7019: // Ellipse_GRS_1980
        ellipsoid = CRS_ELLIPSOID_NAD83;
        break;
      case 7020: // Ellipse_Helmert1906
        ellipsoid = 12;
        break;
      case 7022: // Ellipse_International1924
      case 7023: // Ellipse_International1967
        ellipsoid = 14;
        break;
      case 7024: // Ellipse_Krassowsky1940
        ellipsoid = 15;
        break;
      case 7030: // Ellipse_WGS_84
        ellipsoid = CRS_ELLIPSOID_WGS84;
        break;
      default:
        fprintf(stderr, "GeogEllipsoidGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 3072: // ProjectedCSTypeGeoKey
      if (geokey_entries[i].value_offset != 32767)
        has_projection = set_projection_from_ProjectedCSTypeGeoKey(geokey_entries[i].value_offset, description);
      break;
    case 3075: // ProjCoordTransGeoKey
      user_defined_projection = 0;
      switch (geokey_entries[i].value_offset)
      {
      case 1: // CT_TransverseMercator
        user_defined_projection = 1;
        break;
      case 8: // CT_LambertConfConic_2SP
        user_defined_projection = 8;
        break;
      case 2: // CT_TransvMercator_Modified_Alaska
        fprintf(stderr, "ProjCoordTransGeoKey: CT_TransvMercator_Modified_Alaska not implemented\n");
        break;
      case 3: // CT_ObliqueMercator
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueMercator not implemented\n");
        break;
      case 4: // CT_ObliqueMercator_Laborde
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueMercator_Laborde not implemented\n");
        break;
      case 5: // CT_ObliqueMercator_Rosenmund
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueMercator_Rosenmund not implemented\n");
        break;
      case 6: // CT_ObliqueMercator_Spherical
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueMercator_Spherical not implemented\n");
        break;
      case 7: // CT_Mercator
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Mercator not implemented\n");
        break;
      case 9: // CT_LambertConfConic_Helmert
        fprintf(stderr, "ProjCoordTransGeoKey: CT_LambertConfConic_Helmert not implemented\n");
        break;
      case 10: // CT_LambertAzimEqualArea
        fprintf(stderr, "ProjCoordTransGeoKey: CT_LambertAzimEqualArea not implemented\n");
        break;
      case 11: // CT_AlbersEqualArea
        fprintf(stderr, "ProjCoordTransGeoKey: CT_AlbersEqualArea not implemented\n");
        break;
      case 12: // CT_AzimuthalEquidistant
        fprintf(stderr, "ProjCoordTransGeoKey: CT_AzimuthalEquidistant not implemented\n");
        break;
      case 13: // CT_EquidistantConic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_EquidistantConic not implemented\n");
        break;
      case 14: // CT_Stereographic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Stereographic not implemented\n");
        break;
      case 15: // CT_PolarStereographic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_PolarStereographic not implemented\n");
        break;
      case 16: // CT_ObliqueStereographic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_ObliqueStereographic not implemented\n");
        break;
      case 17: // CT_Equirectangular
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Equirectangular not implemented\n");
        break;
      case 18: // CT_CassiniSoldner
        fprintf(stderr, "ProjCoordTransGeoKey: CT_CassiniSoldner not implemented\n");
        break;
      case 19: // CT_Gnomonic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Gnomonic not implemented\n");
        break;
      case 20: // CT_MillerCylindrical
        fprintf(stderr, "ProjCoordTransGeoKey: CT_MillerCylindrical not implemented\n");
        break;
      case 21: // CT_Orthographic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Orthographic not implemented\n");
        break;
      case 22: // CT_Polyconic
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Polyconic not implemented\n");
        break;
      case 23: // CT_Robinson
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Robinson not implemented\n");
        break;
      case 24: // CT_Sinusoidal
        fprintf(stderr, "ProjCoordTransGeoKey: CT_Sinusoidal not implemented\n");
        break;
      case 25: // CT_VanDerGrinten
        fprintf(stderr, "ProjCoordTransGeoKey: CT_VanDerGrinten not implemented\n");
        break;
      case 26: // CT_NewZealandMapGrid
        fprintf(stderr, "ProjCoordTransGeoKey: CT_NewZealandMapGrid not implemented\n");
        break;
      case 27: // CT_TransvMercator_SouthOriented
        fprintf(stderr, "ProjCoordTransGeoKey: CT_TransvMercator_SouthOriented not implemented\n");
        break;
      default:
        fprintf(stderr, "ProjCoordTransGeoKey: look-up for %d not implemented\n", geokey_entries[i].value_offset);
      }
      break;
    case 3076: // ProjLinearUnitsGeoKey
      set_coordinates_from_ProjLinearUnitsGeoKey(geokey_entries[i].value_offset);
      break;
    case 3078: // ProjStdParallel1GeoKey
      offsetProjStdParallel1GeoKey = geokey_entries[i].value_offset;
      break;
    case 3079: // ProjStdParallel2GeoKey
      offsetProjStdParallel2GeoKey = geokey_entries[i].value_offset;
      break;        
    case 3081: // ProjNatOriginLatGeoKey
      offsetProjNatOriginLatGeoKey = geokey_entries[i].value_offset;
      break;
    case 3082: // ProjFalseEastingGeoKey
      offsetProjFalseEastingGeoKey = geokey_entries[i].value_offset;
      break;
    case 3083: // ProjFalseNorthingGeoKey
      offsetProjFalseNorthingGeoKey = geokey_entries[i].value_offset;
      break;
    case 3088: // ProjCenterLongGeoKey
      offsetProjCenterLongGeoKey = geokey_entries[i].value_offset;
      break;
    case 3092: // ProjScaleAtNatOriginGeoKey
      offsetProjScaleAtNatOriginGeoKey = geokey_entries[i].value_offset;
      break;
    case 4096: // VerticalCSTypeGeoKey 
      set_vertical_from_VerticalCSTypeGeoKey(geokey_entries[i].value_offset);
      break;
    case 4099: // VerticalUnitsGeoKey
      set_elevation_from_VerticalUnitsGeoKey(geokey_entries[i].value_offset);
      break;
    }
  }

  if (ellipsoid != -1)
  {
    set_ellipsoid(ellipsoid, TRUE);
  }

  if (!has_projection)
  {
    if (user_defined_projection == 1)
    {
      if ((offsetProjFalseEastingGeoKey >= 0) &&
          (offsetProjFalseNorthingGeoKey >= 0) &&
          (offsetProjNatOriginLatGeoKey >= 0) &&
          (offsetProjCenterLongGeoKey >= 0) &&
          (offsetProjScaleAtNatOriginGeoKey >= 0))
      {
        F64 falseEasting = geokey_double_params[offsetProjFalseEastingGeoKey];
        F64 falseNorthing = geokey_double_params[offsetProjFalseNorthingGeoKey];
        F64 latOriginDeg = geokey_double_params[offsetProjNatOriginLatGeoKey];
        F64 longMeridianDeg = geokey_double_params[offsetProjCenterLongGeoKey];
        F64 scaleFactor = geokey_double_params[offsetProjScaleAtNatOriginGeoKey];
        set_transverse_mercator_projection(falseEasting, falseNorthing, latOriginDeg, longMeridianDeg, scaleFactor, TRUE, description);
        if (description)
        {
          sprintf(description, "generic transverse mercator");
        }
        has_projection = TRUE;
      }
    }
    else if (user_defined_projection == 8)
    {
      if ((offsetProjFalseEastingGeoKey >= 0) &&
          (offsetProjFalseNorthingGeoKey >= 0) &&
          (offsetProjNatOriginLatGeoKey >= 0) &&
          (offsetProjCenterLongGeoKey >= 0) &&
          (offsetProjStdParallel1GeoKey >= 0) &&
          (offsetProjStdParallel2GeoKey >= 0))
      {
        F64 falseEasting = geokey_double_params[offsetProjFalseEastingGeoKey];
        F64 falseNorthing = geokey_double_params[offsetProjFalseNorthingGeoKey];
        F64 latOriginDeg = geokey_double_params[offsetProjNatOriginLatGeoKey];
        F64 longOriginDeg = geokey_double_params[offsetProjCenterLongGeoKey];
        F64 firstStdParallelDeg = geokey_double_params[offsetProjStdParallel1GeoKey];
        F64 secondStdParallelDeg = geokey_double_params[offsetProjStdParallel2GeoKey];
        set_lambert_conformal_conic_projection(falseEasting, falseNorthing, latOriginDeg, longOriginDeg, firstStdParallelDeg, secondStdParallelDeg, TRUE, description);
        if (description)
        {
          sprintf(description, "generic lambert conformal conic");
        }
        has_projection = TRUE;
      }
    }
  }
  return has_projection;
}

void CRScheck::check(LASheader* lasheader, CHAR* description, BOOL no_CRS_fail)
{
  CHAR note[512];

  if (lasheader->geokeys || lasheader->ogc_wkt)
  {
    if (lasheader->geokeys)
    {
      if (!check_geokeys(lasheader, description))
      {
        sprintf(note, "the %u geokeys do not properly specify a Coordinate Reference System", lasheader->geokeys->number_of_keys);
        if (no_CRS_fail)
        {
          lasheader->add_warning("CRS", note);
        }
        else
        {
          lasheader->add_fail("CRS", note);
        }
      }
      else if ((projections[0]) && (projections[0]->type == CRS_PROJECTION_NONE))
      {
        sprintf(note, "Coordinate Reference System was intentionally not specified (according to the %u geokey%s)", lasheader->geokeys->number_of_keys, (lasheader->geokeys->number_of_keys > 1 ? "s" : ""));
        lasheader->add_warning("CRS", note);
      }
    }
    if (lasheader->ogc_wkt)
    {
      sprintf(note, "there is a OGC WKT string but its check is not yet implemented");
      lasheader->add_warning("CRS", note);
    }
  }
  else
  {
    sprintf(note, "neither GEOTIFF tags nor OGC WKT specify Coordinate Reference System");
    if (no_CRS_fail)
    {
      lasheader->add_warning("CRS", note);
    }
    else
    {
      lasheader->add_fail("CRS", note);
    }
  }
}

CRScheck::CRScheck()
{
  coordinate_units[0] = coordinate_units[1] = 0;
  elevation_units[0] = elevation_units[1] = 0;
  vertical_epsg[0] = vertical_epsg[1] = 0;
  ellipsoids[0] = ellipsoids[1] = 0;
  projections[0] = projections[1] = 0;
};

CRScheck::~CRScheck()
{
  if (ellipsoids[0]) delete ellipsoids[0];
  if (ellipsoids[1]) delete ellipsoids[1];
  if (projections[0]) delete projections[0];
  if (projections[1]) delete projections[1];
};
