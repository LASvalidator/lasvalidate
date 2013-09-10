/*
===============================================================================

  FILE:  lascheck.hpp
  
  CONTENTS:
  
    A set of functions to check LAS headers and points for being specification
    conform. Supported are LAS formats from 1.0 to 1.4 ...
  
  PROGRAMMERS:

    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
  
  COPYRIGHT:
  
    (c) 2007-2013, martin isenburg, rapidlasso - tools to catch reality

    This is free software; you can redistribute and/or modify it under the
    terms of the GNU Lesser General Licence as published by the Free Software
    Foundation. See the COPYING.txt file for more information.

    This software is distributed WITHOUT ANY WARRANTY and without even the
    implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
  
  CHANGE HISTORY:
  
    1 April 2013 -- on Easter Monday all-nighting in Perth airport for PER->SYD
  
===============================================================================
*/
#ifndef LAS_CHECK_HPP
#define LAS_CHECK_HPP

#include "lasheader.hpp"
#include "laspoint.hpp"
#include "lasutility.hpp"

#define LASCHECK_VERSION_MAJOR 0
#define LASCHECK_VERSION_MINOR 0

class LAScheck
{
public:

  void parse(const LASpoint* laspoint);
  void check(LASheader* lasheader, CHAR* crsdescription=0);

  LAScheck(const LASheader* lasheader);
  ~LAScheck();

private:
  F64 min_x, min_y, min_z;
  F64 max_x, max_y, max_z;
  I64 points_with_return_number_zero;
  I64 points_with_number_of_returns_zero;
  I64 points_with_return_number_larger_than_number_of_returns;
  I64 points_outside_bounding_box;
  LASinventory lasinventory;
};

#endif
