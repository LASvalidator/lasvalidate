/*
===============================================================================

  FILE:  lascheck.cpp
  
  CONTENTS:
  
    see corresponding header file

  PROGRAMMERS:
  
    martin.isenburg@rapidlasso.com  -  http://rapidlasso.com
  
  COPYRIGHT:
  
    (c) 2014, martin isenburg, rapidlasso - fast tools to catch reality

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

#include "lascheck.hpp"

#include "crscheck.hpp"

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

void LAScheck::parse(const LASpoint* laspoint)
{
  // add point to inventory

  lasinventory.add(laspoint);

  // check point against bounding box

  if (!laspoint->inside_bounding_box(min_x, min_y, min_z, max_x, max_y, max_z))
  {
    points_outside_bounding_box++;
  }
}

void LAScheck::check(LASheader* lasheader, CHAR* crsdescription, BOOL no_CRS_fail)
{
  U32 i,j;
  CHAR problem[512];
  CHAR note[512];

  // check file signature

  if ((lasheader->file_signature[0] != 'L') || (lasheader->file_signature[1] != 'A') || (lasheader->file_signature[2] != 'S') || (lasheader->file_signature[3] != 'F'))
  {
    sprintf(note, "should be 'LASF' and not '%4s'", lasheader->file_signature);
    lasheader->add_fail("file signature", note);
  }

  // check global encoding

  if ((lasheader->version_major == 1) && (lasheader->version_minor <= 1))
  {
    if (lasheader->global_encoding > 0)
    {
      sprintf(note, "should 0 for LAS version %d.%d but is %d", (I32)lasheader->version_major, (I32)lasheader->version_minor, (I32)lasheader->global_encoding);
      lasheader->add_fail("global encoding", note);
    }
  }
  else if ((lasheader->version_major == 1) && (lasheader->version_minor <= 2))
  {
    if (lasheader->global_encoding > 1)
    {
      sprintf(note, "should not be greater than 1 for LAS version %d.%d but is %d", (I32)lasheader->version_major, (I32)lasheader->version_minor, (I32)lasheader->global_encoding);
      lasheader->add_fail("global encoding", note);
    }
  }
  else if ((lasheader->version_major == 1) && (lasheader->version_minor <= 3))
  {
    if (lasheader->global_encoding > 15)
    {
      sprintf(note, "should not be greater than 1 for LAS version %d.%d but is %d", (I32)lasheader->version_major, (I32)lasheader->version_minor, (I32)lasheader->global_encoding);
      lasheader->add_fail("global encoding", note);
    }
  }
  else if ((lasheader->version_major == 1) && (lasheader->version_minor <= 4))
  {
    if (lasheader->global_encoding > 31)
    {
      sprintf(note, "should not be greater than 1 for LAS version %d.%d but is %d", (I32)lasheader->version_major, (I32)lasheader->version_minor, (I32)lasheader->global_encoding);
      lasheader->add_fail("global encoding", note);
    }
  }

  if (lasheader->global_encoding & 16)
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor <= 3))
    {
      sprintf(note, "set bit 4 not defined for LAS version %d.%d", (I32)lasheader->version_major, (I32)lasheader->version_minor);
      lasheader->add_fail("global encoding", note);
    }
  }
  else
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor >= 4) && (lasheader->point_data_format >= 6))
    {
      sprintf(note, "bit 4 must be set (OGC WKT must be used) for point data format %d", lasheader->point_data_format);
      lasheader->add_fail("global encoding", note);
    }
  }

  if (lasheader->global_encoding & 8)
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor <= 2))
    {
      sprintf(note, "set bit 3 not defined for LAS version %d.%d", lasheader->version_major, lasheader->version_minor);
      lasheader->add_fail("global encoding", note);
    }
  }

  if (lasheader->global_encoding & 4)
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor <= 2))
    {
      sprintf(note, "set bit 2 not defined for LAS version %d.%d", lasheader->version_major, lasheader->version_minor);
      lasheader->add_fail("global encoding", note);
    }
    if ((lasheader->point_data_format != 4) && (lasheader->point_data_format != 5) && (lasheader->point_data_format != 9) && (lasheader->point_data_format != 10))
    {
      sprintf(note, "set bit 2 not defined for point data format %d", lasheader->point_data_format);
      lasheader->add_fail("global encoding", note);
    }
    if (lasheader->global_encoding & 2)
    {
      sprintf(note, "although bit 1 and bit 2 are mutually exclusive they are both set");
      lasheader->add_fail("global encoding", note);
    }
  }
  else if ((lasheader->version_major == 1) && (lasheader->version_minor >= 3))
  {
    if ((lasheader->point_data_format == 4) || (lasheader->point_data_format == 5) || (lasheader->point_data_format == 9) || (lasheader->point_data_format == 10))
    {
      if ((lasheader->global_encoding & 2) == 0)
      {
        sprintf(note, "neither bit 1 nor bit 2 are set for point data format %d", lasheader->point_data_format);
        lasheader->add_fail("global encoding", note);
      }
    }
  }

  if (lasheader->global_encoding & 2)
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor <= 2))
    {
      sprintf(note, "set bit 1 not defined for LAS version %d.%d", lasheader->version_major, lasheader->version_minor);
      lasheader->add_fail("global encoding", note);
    }
    if ((lasheader->point_data_format != 4) && (lasheader->point_data_format != 5) && (lasheader->point_data_format != 9) && (lasheader->point_data_format != 10))
    {
      sprintf(note, "set bit 1 not defined for point data format %d", lasheader->point_data_format);
      lasheader->add_fail("global encoding", note);
    }
  }

  if (lasheader->global_encoding & 1)
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor <= 1))
    {
      sprintf(note, "set bit 0 not defined for LAS version %d.%d", lasheader->version_major, lasheader->version_minor);
      lasheader->add_fail("global encoding", note);
    }

    if (lasheader->point_data_format == 0)
    {
      sprintf(note, "set bit 0 not defined for point data format 0");
      lasheader->add_fail("global encoding", note);
    }
  }
  else
  {
    if (lasheader->point_data_format > 0)
    {
      if (lasinventory.is_active())
      {
        if ((lasinventory.min_gps_time < 0.0) || (lasinventory.max_gps_time > 604800.0)) 
        {
          CHAR string1[512], string2[512];
          lidardouble2string(string1, lasinventory.min_gps_time, 0.000001);
          lidardouble2string(string2, lasinventory.max_gps_time, 0.000001);
          sprintf(note, "unset bit 0 suggests GPS week time but GPS time ranges from %s to %s", string1, string2);
          lasheader->add_fail("global encoding", note);
        }
      }
    }
  }

  // check version major

  if (lasheader->version_major != 1)
  {
    sprintf(note, "should be 1 and not %d", lasheader->version_major);
    lasheader->add_fail("version major", note);
  }

  // check version minor

  if ((lasheader->version_minor != 0) && (lasheader->version_minor != 1) && (lasheader->version_minor != 2) && (lasheader->version_minor != 3) && (lasheader->version_minor != 4))
  {
    sprintf(note, "should be between 0 and 4 and not %d", lasheader->version_minor);
    lasheader->add_fail("version minor", note);
  }

  // check system identifier

  for (i = 0; i < 32; i++)
  {
    if (lasheader->system_identifier[i] == '\0')
    {
      break;
    }
  }
  if (i == 32)
  {
    sprintf(note, "string should be terminated by a '\\0' character");
    lasheader->add_fail("system identifier", note);
  }
  else if (i == 0)
  {
    sprintf(note, "empty string. first character is '\\0'");
    lasheader->add_warning("system identifier", note);
  }
  for (j = i; j < 32; j++)
  {
    if (lasheader->system_identifier[j] != '\0')
    {
      break;
    }
  }
  if (j != 32)
  {
    sprintf(note, "remaining characters should all be '\\0'");
    lasheader->add_fail("system identifier", note);
  }

  // check generating software

  for (i = 0; i < 32; i++)
  {
    if (lasheader->generating_software[i] == '\0')
    {
      break;
    }
  }
  if (i == 32)
  {
    sprintf(note, "string should be terminated by a '\\0' character");
    lasheader->add_fail("generating software", note);
  }
  else if (i == 0)
  {
    sprintf(note, "empty string. first character is '\\0'");
    lasheader->add_warning("generating software", note);
  }
  for (j = i; j < 32; j++)
  {
    if (lasheader->generating_software[j] != '\0')
    {
      break;
    }
  }
  if (j != 32)
  {
    sprintf(note, "remaining characters should all be '\\0'");
    lasheader->add_fail("generating software", note);
  }

  // check file creation date

  if (lasheader->file_creation_year == 0)
  {
    if (lasheader->file_creation_day == 0)
    {
      sprintf(note, "not set");
      lasheader->add_fail("file creation day", note);
    }
    else if (lasheader->file_creation_day > 365)
    {
      sprintf(note, "should be between 1 and 365 and not %d", lasheader->file_creation_day);
      lasheader->add_fail("file creation day", note);
    }
    sprintf(note, "not set");
    lasheader->add_fail("file creation year", note);
  }
  else
  {
    // get today's date

    time_t date;
    time(&date);
    struct tm* tm_date = gmtime(&date);
    int today_year = tm_date->tm_year + 1900;

    // does the year fall into the expected range

    if ((lasheader->file_creation_year < 1990) || (lasheader->file_creation_year > today_year))
    {
      sprintf(note, "should be between 1990 and %d and not %d", today_year, lasheader->file_creation_year);
      lasheader->add_fail("file creation year", note);
    }

    // does the day fall into the expected range

    int max_day_of_year = 365;

    if (lasheader->file_creation_year == today_year)
    {
      // for the current year we need to limit the range (plus 1 because in GPS time January 1 is day 1)

      max_day_of_year = tm_date->tm_yday + 1;
    }
    else if ((((lasheader->file_creation_year)%4) == 0))
    {
      // in a leap year we need to add one day

      max_day_of_year++;
    }

    if (lasheader->file_creation_day > max_day_of_year)
    {
      sprintf(note, "should be between 1 and %d and not %d", max_day_of_year, lasheader->file_creation_day);
      lasheader->add_fail("file creation day", note);
    }
  }

  // check header size

  int min_header_size = 227;

  if (lasheader->version_major == 1)
  {
    if (lasheader->version_minor >= 3)
    {
      min_header_size += 8;
    }
    if (lasheader->version_minor >= 4)
    {
      min_header_size += 40;
    }
  }

  if (lasheader->header_size < min_header_size)
  {
    sprintf(note, "should be at least %d and not %d", min_header_size, lasheader->header_size);
    lasheader->add_fail("header size", note);
  }

  // check offset to point data

  U32 min_offset_to_point_data = lasheader->header_size;

  // add size of all VLRs

  for (i = 0; i < lasheader->number_of_variable_length_records; i++)
  {
    min_offset_to_point_data += 54; // VLR header size
    min_offset_to_point_data += lasheader->vlrs[i].record_length_after_header; // VLR payload size
  }

  if (lasheader->offset_to_point_data < min_offset_to_point_data)
  {
    sprintf(note, "should be at least %u and not %u", min_offset_to_point_data, lasheader->offset_to_point_data);
    lasheader->add_fail("offset to point data", note);
  }

  // check point data format

  U8 max_point_data_format = 1;

  if (lasheader->version_major == 1)
  {
    if (lasheader->version_minor == 2)
    {
      max_point_data_format = 3;
    }
    else if (lasheader->version_minor == 3)
    {
      max_point_data_format = 5;
    }
    else if (lasheader->version_minor == 4)
    {
      max_point_data_format = 10;
    }
  }

  if (lasheader->point_data_format > max_point_data_format)
  {
    sprintf(note, "should be between 0 and %d and not %d", max_point_data_format, lasheader->point_data_format);
    lasheader->add_fail("point data format", note);
  }

  // check point data record length

  int min_point_data_record_length = 20;

  switch (lasheader->point_data_format)
  {
  case 1:
    min_point_data_record_length = 28;
    break;
  case 2:
    min_point_data_record_length = 26;
    break;
  case 3:
    min_point_data_record_length = 34;
    break;
  case 4:
    min_point_data_record_length = 57;
    break;
  case 5:
    min_point_data_record_length = 63;
    break;
  case 6:
    min_point_data_record_length = 30;
    break;
  case 7:
    min_point_data_record_length = 36;
    break;
  case 8:
    min_point_data_record_length = 38;
    break;
  case 9:
    min_point_data_record_length = 59;
    break;
  case 10:
    min_point_data_record_length = 67;
    break;
  }

  if (lasheader->point_data_record_length < min_point_data_record_length)
  {
    sprintf(note, "should be at least %d and not %d", min_point_data_record_length, lasheader->point_data_record_length);
    lasheader->add_fail("point data record length", note);
  }

  // check that there is at least one point

  if (lasheader->legacy_number_of_point_records == 0)
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor >= 4))
    {
      if (lasheader->number_of_point_records == 0)
      {
        sprintf(note, "files must contain at least one point record to be considered valid");
        lasheader->add_fail("zero points in file", note);
      }
    }
    else
    {
      sprintf(note, "files must contain at least one point record to be considered valid");
      lasheader->add_fail("zero points in file", note);
    }
  }

  // check integraty between legacy number of point records and number of point records for LAS 1.4 and higher

  if ((lasheader->version_major == 1) && (lasheader->version_minor >= 4))
  {
    if (lasheader->legacy_number_of_point_records != 0)
    {
      if (lasheader->legacy_number_of_point_records != U32_CLAMP(lasheader->number_of_point_records))
      {
        sprintf(note, "should be consistent with number of point records and either be 0 or %u and not %u", U32_CLAMP(lasheader->number_of_point_records), lasheader->legacy_number_of_point_records);
        lasheader->add_fail("legacy number of point records", note);
      }
    }
  }

  // check integraty between legacy number of points by return and number of points by return for LAS 1.4 and higher

  if ((lasheader->version_major == 1) && (lasheader->version_minor >= 4))
  {
    for (i = 0; i < 5; i++)
    {
      if (lasheader->legacy_number_of_points_by_return[i] != 0)
      {
        if (lasheader->number_of_points_by_return[i] == 0)
        {
          sprintf(problem, "number of points by return[%d]", i);
          sprintf(note, "is zero but should be identical to the (non-zero) legacy number of points by return of %u", lasheader->legacy_number_of_points_by_return[i]);
          lasheader->add_fail(problem, note);
        }
        else if (lasheader->legacy_number_of_points_by_return[i] != U32_CLAMP(lasheader->number_of_points_by_return[i]))
        {
          sprintf(problem, "legacy number of points by return[%d]", i);
          sprintf(note, "should be consistent with number of points by return and either be 0 or %u and not %u", U32_CLAMP(lasheader->number_of_points_by_return[i]), lasheader->legacy_number_of_points_by_return[i]);
          lasheader->add_fail(problem, note);
        }
      }
    }
  }

  // check number of point records in header against the counted inventory

  if (lasinventory.is_active())
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor >= 4))
    {
      if (lasheader->number_of_point_records != lasinventory.number_of_point_records)
      {
#ifdef _WIN32
        sprintf(note, "there are only %I64d point records and not %I64d", (I64)lasinventory.number_of_point_records, (I64)lasheader->number_of_point_records);
#else
        sprintf(note, "there are only %lld point records and not %lld", (I64)lasinventory.number_of_point_records, (I64)lasheader->number_of_point_records);
#endif
        lasheader->add_fail("number of point records", note);
      }
    }
    else
    {
      if (lasheader->legacy_number_of_point_records != U32_CLAMP(lasinventory.number_of_point_records))
      {
        sprintf(note, "there are only %u point records and not %u", U32_CLAMP(lasinventory.number_of_point_records), lasheader->legacy_number_of_point_records);
        lasheader->add_fail("number of point records", note);
      }
    }
  }

  // check number of points by return in header against the counted inventory

  if (lasinventory.is_active())
  {
    if ((lasheader->version_major == 1) && (lasheader->version_minor >= 4))
    {
      for (i = 0; i < 15; i++)
      {
        if (lasheader->number_of_points_by_return[i] != lasinventory.number_of_points_by_return[i+1])
        {
          sprintf(problem, "number of points by return[%d]", i);
#ifdef _WIN32
          sprintf(note, "the number of %2d%s returns is %I64d and not %I64d", i+1, (i == 0 ? "st" : (i == 1 ? "nd" : (i == 2 ? "rd" : "th"))), (I64)lasinventory.number_of_points_by_return[i+1], (I64)lasheader->number_of_points_by_return[i]);
#else
          sprintf(note, "the number of %d%s returns is %lld and not %lld", i+1, (i == 0 ? "st" : (i == 1 ? "nd" : (i == 2 ? "rd" : "th"))), (I64)lasinventory.number_of_points_by_return[i+1], (I64)lasheader->number_of_points_by_return[i]);
#endif
          lasheader->add_fail(problem, note);
        }
      }
    }
    else
    {
      for (i = 0; i < 5; i++)
      {
        if (lasheader->legacy_number_of_points_by_return[i] != U32_CLAMP(lasinventory.number_of_points_by_return[i+1]))
        {
          sprintf(problem, "number of points by return[%d]", i);
          sprintf(note, "the number of %d%s returns is %u and not %u", i+1, (i == 0 ? "st" : (i == 1 ? "nd" : (i == 2 ? "rd" : "th"))), U32_CLAMP(lasinventory.number_of_points_by_return[i+1]), lasheader->legacy_number_of_points_by_return[i]);
          lasheader->add_fail(problem, note);
        }
      }
    }
  }

  // check scale factor x y z

  if (lasheader->x_scale_factor <= 0.0)
  {
    sprintf(note, "%g is equal to or smaller than zero", lasheader->x_scale_factor);
    lasheader->add_fail("x scale factor", note);
  }

  if (lasheader->y_scale_factor <= 0.0)
  {
    sprintf(note, "%g is equal to or smaller than zero", lasheader->y_scale_factor);
    lasheader->add_fail("y scale factor", note);
  }

  if (lasheader->z_scale_factor <= 0.0)
  {
    sprintf(note, "%g is equal to or smaller than zero", lasheader->z_scale_factor);
    lasheader->add_fail("z scale factor", note);
  }

  if (F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.01, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.1, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.0000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.00000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.0001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.00001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 1.0, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.25, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.0025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.00025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.000025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.0000025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.5, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.05, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.0005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.00005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.000005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.0000005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->x_scale_factor, 0.00000005, 0.0000001))
  {
    CHAR string[256];
    lidardouble2string(string, lasheader->x_scale_factor);
    sprintf(note, "should be factor ten of 0.1 or 0.5 or 0.25 and not %s", string);
    lasheader->add_warning("x scale factor", note);
  }

  if (F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.01, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.1, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.0000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.00000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.0001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.00001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 1.0, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.25, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.0025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.00025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.000025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.0000025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.5, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.05, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.0005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.00005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.000005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.0000005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->y_scale_factor, 0.00000005, 0.0000001))
  {
    CHAR string[256];
    lidardouble2string(string, lasheader->y_scale_factor);
    sprintf(note, "should be factor ten of 0.1 or 0.5 or 0.25 and not %s", string);
    lasheader->add_warning("y scale factor", note);
  }
  if (F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.01, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.1, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.0000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.00000001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.0001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.00001, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 1.0, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.25, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.0025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.00025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.000025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.0000025, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.5, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.05, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.0005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.00005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.000005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.0000005, 0.0000001) &&
      F64_NOT_CLOSE_POSITIVE(lasheader->z_scale_factor, 0.00000005, 0.0000001))
  {
    CHAR string[256];
    lidardouble2string(string, lasheader->z_scale_factor);
    sprintf(note, "should be factor ten of 0.1 or 0.5 or 0.25 and not %s", string);
    lasheader->add_warning("z scale factor", note);
  }

  // check offset x y z

  I64 x_offset_quantized = I64_QUANTIZE(lasheader->x_offset/lasheader->x_scale_factor);
  F64 x_offset = lasheader->x_scale_factor * x_offset_quantized;
  if (F64_NOT_CLOSE_POSITIVE(lasheader->x_offset - x_offset, 0.0, 0.0000001))
  {
    CHAR string1[256];
    lidardouble2string(string1, lasheader->x_offset);
    CHAR string2[256];
    lidardouble2string(string2, lasheader->x_scale_factor);
    sprintf(note, "translation fluff: decimal digits of %s do not match scale factor %s", string1, string2);
    lasheader->add_warning("x offset", note);
  }

  I64 y_offset_quantized = I64_QUANTIZE(lasheader->y_offset/lasheader->y_scale_factor);
  F64 y_offset = lasheader->y_scale_factor * y_offset_quantized;
  if (F64_NOT_CLOSE_POSITIVE(lasheader->y_offset - y_offset, 0.0, 0.0000001))
  {
    CHAR string1[256];
    lidardouble2string(string1, lasheader->y_offset);
    CHAR string2[256];
    lidardouble2string(string2, lasheader->y_scale_factor);
    sprintf(note, "translation fluff: decimal digits of %s do not match scale factor %s", string1, string2);
    lasheader->add_warning("y offset", note);
  }

  I64 z_offset_quantized = I64_QUANTIZE(lasheader->z_offset/lasheader->z_scale_factor);
  F64 z_offset = lasheader->z_scale_factor * z_offset_quantized;
  if (F64_NOT_CLOSE_POSITIVE(lasheader->z_offset - z_offset, 0.0, 0.0000001))
  {
    CHAR string1[256];
    lidardouble2string(string1, lasheader->z_offset);
    CHAR string2[256];
    lidardouble2string(string2, lasheader->z_scale_factor);
    sprintf(note, "translation fluff: decimal digits of %s do not match scale factor %s", string1, string2);
    lasheader->add_warning("z offset", note);
  }

  // check global encoding 

  if ((lasheader->version_major == 1) && (lasheader->version_minor >= 3))
  {
    if (((lasheader->global_encoding & 2) == 0) && (lasheader->start_of_waveform_data_packet_record != 0))
    {
      sprintf(note, "should be 0 and not %u because global encoding bit 1 is not set", (U32)lasheader->start_of_waveform_data_packet_record);
      lasheader->add_fail("start of waveform data packet record", note);
    }
    else if (((lasheader->global_encoding & 2) == 2) && (lasheader->start_of_waveform_data_packet_record == 0))
    {
      sprintf(note, "should not be 0 because global encoding bit 1 is set");
      lasheader->add_fail("start of waveform data packet record", note);
    }
  }

  // check for resolution fluff in the coordinates

  if (lasinventory.is_active())
  {
    if (lasinventory.has_fluff())
    {
      sprintf(note, "resolution fluff (x10) in %s%s%s", (lasinventory.has_fluff(0) ? "X" : ""), (lasinventory.has_fluff(1) ? "Y" : ""), (lasinventory.has_fluff(2) ? "Z" : ""));
      lasheader->add_warning("coordinate values", note);
      if (lasinventory.has_serious_fluff())
      {
        sprintf(note, "serious resolution fluff (x100) in %s%s%s", (lasinventory.has_serious_fluff(0) ? "X" : ""), (lasinventory.has_serious_fluff(1) ? "Y" : ""), (lasinventory.has_serious_fluff(2) ? "Z" : ""));
        lasheader->add_warning("coordinate values", note);
        if (lasinventory.has_very_serious_fluff())
        {
          sprintf(note, "very serious resolution fluff (x1000) in %s%s%s", (lasinventory.has_very_serious_fluff(0) ? "X" : ""), (lasinventory.has_very_serious_fluff(1) ? "Y" : ""), (lasinventory.has_very_serious_fluff(2) ? "Z" : ""));
          lasheader->add_warning("coordinate values", note);
        }
      }
    }
  }

  // check bounding box x y z

  if (points_outside_bounding_box)
  {
#ifdef _WIN32
    sprintf(note, "there are %I64d points outside of the bounding box specified in the LAS file header", points_outside_bounding_box);
#else
    sprintf(note, "there are %lld points outside of the bounding box specified in the LAS file header", points_outside_bounding_box);
#endif
    lasheader->add_fail("bounding box", note);
  }

  if (lasinventory.is_active())
  {
    if ((lasheader->min_x - 0.5*lasheader->x_scale_factor) > lasheader->get_x(lasinventory.min_X))
    {
      CHAR string1[64], string2[64];
      lidardouble2string(string1, lasheader->get_x(lasinventory.min_X), lasheader->x_scale_factor);
      lidardouble2string(string2, lasheader->min_x, lasheader->x_scale_factor);
      sprintf(note, "should be %s and not %s", string1, string2);
      lasheader->add_fail("min x", note);
    }
    if ((lasheader->max_x + 0.5*lasheader->x_scale_factor) < lasheader->get_x(lasinventory.max_X))
    {
      CHAR string1[64], string2[64];
      lidardouble2string(string1, lasheader->get_x(lasinventory.max_X), lasheader->x_scale_factor);
      lidardouble2string(string2, lasheader->max_x, lasheader->x_scale_factor);
      sprintf(note, "should be %s and not %s", string1, string2);
      lasheader->add_fail("max x", note);
    }
    if ((lasheader->min_y - 0.5*lasheader->y_scale_factor) > lasheader->get_y(lasinventory.min_Y))
    {
      CHAR string1[64], string2[64];
      lidardouble2string(string1, lasheader->get_y(lasinventory.min_Y), lasheader->y_scale_factor);
      lidardouble2string(string2, lasheader->min_y, lasheader->y_scale_factor);
      sprintf(note, "should be %s and not %s", string1, string2);
      lasheader->add_fail("min y", note);
    }
    if ((lasheader->max_y + 0.5*lasheader->y_scale_factor) < lasheader->get_y(lasinventory.max_Y))
    {
      CHAR string1[64], string2[64];
      lidardouble2string(string1, lasheader->get_y(lasinventory.max_Y), lasheader->y_scale_factor);
      lidardouble2string(string2, lasheader->max_y, lasheader->y_scale_factor);
      sprintf(note, "should be %s and not %s", string1, string2);
      lasheader->add_fail("max y", note);
    }
    if ((lasheader->min_z - 0.5*lasheader->z_scale_factor) > lasheader->get_z(lasinventory.min_Z))
    {
      CHAR string1[64], string2[64];
      lidardouble2string(string1, lasheader->get_z(lasinventory.min_Z), lasheader->z_scale_factor);
      lidardouble2string(string2, lasheader->min_z, lasheader->z_scale_factor);
      sprintf(note, "should be %s and not %s", string1, string2);
      lasheader->add_fail("min z", note);
    }
    if ((lasheader->max_z + 0.5*lasheader->z_scale_factor) < lasheader->get_z(lasinventory.max_Z))
    {
      CHAR string1[64], string2[64];
      lidardouble2string(string1, lasheader->get_z(lasinventory.max_Z), lasheader->z_scale_factor);
      lidardouble2string(string2, lasheader->max_z, lasheader->z_scale_factor);
      sprintf(note, "should be %s and not %s", string1, string2);
      lasheader->add_fail("max z", note);
    }
  }

  // check the inventory for invalid return numbers

  if (lasinventory.is_active())
  {
    if (lasinventory.number_of_points_by_return[0] != 0)
    {
#ifdef _WIN32
      sprintf(note, "there are %I64d points with a return number of 0", (I64)lasinventory.number_of_points_by_return[0]);
#else
      sprintf(note, "there are %lld points with a return number of 0", (I64)lasinventory.number_of_points_by_return[0]);
#endif
      lasheader->add_warning("return number", note);
    }
    if ((lasheader->version_major == 1) && (lasheader->version_minor < 4))
    {
      if (lasinventory.number_of_points_by_return[6] != 0)
      {
#ifdef _WIN32
        sprintf(note, "there are %I64d points with a return number of 6", (I64)lasinventory.number_of_points_by_return[6]);
#else
        sprintf(note, "there are %lld points with a return number of 6", (I64)lasinventory.number_of_points_by_return[6]);
#endif
        lasheader->add_warning("return number", note);
      }
      if (lasinventory.number_of_points_by_return[7] != 0)
      {
#ifdef _WIN32
        sprintf(note, "there are %I64d points with a return number of 7", (I64)lasinventory.number_of_points_by_return[7]);
#else
        sprintf(note, "there are %lld points with a return number of 7", (I64)lasinventory.number_of_points_by_return[7]);
#endif
        lasheader->add_warning("return number", note);
      }
    }
  }

  // check the inventory for invalid number of returns of given pulse

  if (lasinventory.is_active())
  {
    if (lasinventory.number_of_returns_of_given_pulse[0] != 0)
    {
#ifdef _WIN32
      sprintf(note, "there are %I64d points with a number of returns of given pulse of 0", (I64)lasinventory.number_of_returns_of_given_pulse[0]);
#else
      sprintf(note, "there are %lld points with a number of returns of given pulse of 0", (I64)lasinventory.number_of_returns_of_given_pulse[0]);
#endif
      lasheader->add_warning("number of returns of given pulse", note);
    }
    if ((lasheader->version_major == 1) && (lasheader->version_minor < 4))
    {
      if (lasinventory.number_of_returns_of_given_pulse[6] != 0)
      {
#ifdef _WIN32
        sprintf(note, "there are %I64d points with a number of returns of given pulse of 6", (I64)lasinventory.number_of_returns_of_given_pulse[6]);
#else
        sprintf(note, "there are %lld points with a number of returns of given pulse of 6", (I64)lasinventory.number_of_returns_of_given_pulse[6]);
#endif
        lasheader->add_warning("return number", note);
      }
      if (lasinventory.number_of_returns_of_given_pulse[7] != 0)
      {
#ifdef _WIN32
        sprintf(note, "there are %I64d points with a number of returns of given pulse of 7", (I64)lasinventory.number_of_returns_of_given_pulse[7]);
#else
        sprintf(note, "there are %lld points with a number of returns of given pulse of 7", (I64)lasinventory.number_of_returns_of_given_pulse[7]);
#endif
        lasheader->add_warning("return number", note);
      }
    }
  }

  // check the inventory for invalid combinations of return numbers and number of returns of given pulse

  if (lasinventory.is_active())
  {
    for (i = 0; i < 16; i++)
    {
      for (j = i+1; j < 16; j++)
      {
        if (lasinventory.return_count_for_return_number[i][j] != 0)
        {
#ifdef _WIN32
          sprintf(note, "there are %I64d points with a larger return number (%d) than their number of returns of given pulse (%d)", lasinventory.return_count_for_return_number[i][j], j, i);
#else
          sprintf(note, "there are %lld points with a larger return number (%d) than their number of returns of given pulse (%d)", lasinventory.return_count_for_return_number[i][j], j, i);
#endif
          lasheader->add_fail("return number", note);
        }
      }
    }
  }

  // check for odd intensities

  if (lasinventory.is_active())
  {
    if ((lasinventory.number_of_point_records > 1) && (lasinventory.min_intensity == lasinventory.max_intensity))
    {
#ifdef _WIN32
      sprintf(note, "intensity of all %I64d points is %d", (I64)lasinventory.number_of_point_records, (I32)lasinventory.min_intensity);
#else
      sprintf(note, "intensity of all %lld points is %d", (I64)lasinventory.number_of_point_records, (I32)lasinventory.min_intensity);
#endif
      lasheader->add_warning("intensity", note);
    }
  }

  // check for odd scan angles

  if (lasinventory.is_active())
  {
    if (lasinventory.number_of_point_records > 1)
    {
      if (lasheader->point_data_format < 6)
      {
        if (lasinventory.min_scan_angle_rank == lasinventory.max_scan_angle_rank)
        {
#ifdef _WIN32
          sprintf(note, "scan angle rank of all %I64d points is %d", (I64)lasinventory.number_of_point_records, (I32)lasinventory.min_scan_angle_rank);
#else
          sprintf(note, "scan angle rank of all %lld points is %d", (I64)lasinventory.number_of_point_records, (I32)lasinventory.min_scan_angle_rank);
#endif
          lasheader->add_warning("scan angle rank", note);
        }
      }
      else
      {
        if (lasinventory.min_scan_angle == lasinventory.max_scan_angle)
        {
#ifdef _WIN32
          sprintf(note, "scan angle of all %I64d points is %.3f", (I64)lasinventory.number_of_point_records, 0.006*lasinventory.min_scan_angle);
#else
          sprintf(note, "scan angle of all %lld points is %.3f", (I64)lasinventory.number_of_point_records, 0.006*lasinventory.min_scan_angle);
#endif
          lasheader->add_warning("scan angle", note);
        }
      }
    }
  }

  // check for zero point source IDs

  if (lasinventory.is_active())
  {
    if ((lasheader->file_source_ID == 0) && (lasinventory.number_of_point_records > 1) && (lasinventory.min_point_source_ID == 0) && (lasinventory.max_point_source_ID == 0))
    {
#ifdef _WIN32
      sprintf(note, "file source ID in header and point source ID of all %I64d points is %d", (I64)lasinventory.number_of_point_records, (I32)lasinventory.min_point_source_ID);
#else
      sprintf(note, "file source ID in header and point source ID of all %lld points is %d", (I64)lasinventory.number_of_point_records, (I32)lasinventory.min_point_source_ID);
#endif
      lasheader->add_warning("point source ID", note);
    }
  }

  // check for file source ID and point source IDs disagreement

  if (lasinventory.is_active())
  {
    if ((lasheader->file_source_ID != 0) && (lasinventory.number_of_point_records > 1) && ((lasheader->file_source_ID != lasinventory.min_point_source_ID) || (lasheader->file_source_ID != lasinventory.max_point_source_ID)) && ((lasinventory.min_point_source_ID != 0) || (lasinventory.max_point_source_ID != 0)))
    {
#ifdef _WIN32
      sprintf(note, "file source ID in header is %d but point source IDs of all %I64d points range from %d to %d", (I32)lasheader->file_source_ID, (I64)lasinventory.number_of_point_records, (I32)lasinventory.min_point_source_ID, (I32)lasinventory.max_point_source_ID);
#else
      sprintf(note, "file source ID in header is %d but point source IDs of all %lld points range from %d to %d", (I32)lasheader->file_source_ID, (I64)lasinventory.number_of_point_records, (I32)lasinventory.min_point_source_ID, (I32)lasinventory.max_point_source_ID);
#endif
      lasheader->add_warning("point source ID", note);
    }
  }

  // check for point data formats 1, 3, 4, and higher in the inventory whether all GPS time stamps are identical

  if ((lasheader->point_data_format != 0) && (lasheader->point_data_format != 2))
  {
    if (lasinventory.is_active())
    {
      if ((lasinventory.number_of_point_records > 1) && (lasinventory.min_gps_time == lasinventory.max_gps_time))
      {
#ifdef _WIN32
        sprintf(note, "time stamps of all %I64d points are %g", (I64)lasinventory.number_of_point_records, lasinventory.min_gps_time);
#else
        sprintf(note, "time stamps of all %lld points are %g", (I64)lasinventory.number_of_point_records, lasinventory.min_gps_time);
#endif
        lasheader->add_warning("GPS time", note);
      }
    }
  }

  // check for point data formats 2, 3, 7, 8, and 10 in the inventory whether all RGB values are identical

  if ((lasheader->point_data_format == 2) || (lasheader->point_data_format == 3) || (lasheader->point_data_format == 7) || (lasheader->point_data_format == 8) || (lasheader->point_data_format == 10))
  {
    if (lasinventory.is_active())
    {
      if ((lasinventory.number_of_point_records > 1) && (lasinventory.min_R == lasinventory.max_R) && (lasinventory.min_G == lasinventory.max_G) && (lasinventory.min_B == lasinventory.max_B))
      {
#ifdef _WIN32
        sprintf(note, "color of all %I64d points is (%d/%d/%d)", (I64)lasinventory.number_of_point_records, lasinventory.max_R, lasinventory.max_G, lasinventory.max_B);
#else
        sprintf(note, "color of all %lld points is (%d/%d/%d)", (I64)lasinventory.number_of_point_records, lasinventory.max_R, lasinventory.max_G, lasinventory.max_B);
#endif
        lasheader->add_warning("RGB", note);
      }
    }
  }

  if (lasheader->point_data_format <= 5)
  {
    // a GEOTIFF tag CRS must be there when the point type is 5 or lower
    if (lasheader->geokeys == 0)
    {
      sprintf(note, "file does not specify a Coordinate Reference System with GEOTIFF tags");
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
  else
  {
    // an OCG WKT CRS must be there when the point type is 6 or higher
    if (lasheader->ogc_wkt == 0)
    {
      sprintf(note, "file with point data format %d does not specify Coordinate Reference System with OGC WKT string", lasheader->point_data_format);
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

  // check for wrong wave packet indices

  if ((lasheader->point_data_format == 4) || (lasheader->point_data_format == 5) || (lasheader->point_data_format == 9) || (lasheader->point_data_format == 10))
  {
    U16 index;
    for (index = 1; index < 256; index++)
    {
      if (lasinventory.has_wave_packet_index((U8)index))
      {
        if (lasheader->wave_packet_descriptor == 0)
        {
          sprintf(note, "points reference wave packet descriptor with index %u that does not exist", index);
          lasheader->add_fail("wave packet descriptor", note);
        }
        else if (lasheader->wave_packet_descriptor[index] == 0)
        {
          sprintf(note, "points reference wave packet descriptor with index %u that does not exist", index);
          lasheader->add_fail("wave packet", note);
        }
      }
    }
  }

  if (lasheader->geokeys || lasheader->ogc_wkt)
  {
    CRScheck crscheck;
    crscheck.check(lasheader, crsdescription, no_CRS_fail);
  }
}

LAScheck::LAScheck(const LASheader* lasheader)
{
  min_x = lasheader->min_x - lasheader->x_scale_factor;
  min_y = lasheader->min_y - lasheader->y_scale_factor;
  min_z = lasheader->min_z - lasheader->z_scale_factor;
  max_x = lasheader->max_x + lasheader->x_scale_factor;
  max_y = lasheader->max_y + lasheader->y_scale_factor;
  max_z = lasheader->max_z + lasheader->z_scale_factor;
  points_outside_bounding_box = 0;
}

LAScheck::~LAScheck()
{
}
