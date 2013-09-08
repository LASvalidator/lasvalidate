/*
===============================================================================

  FILE:  lasvalidate.cpp
  
  CONTENTS:
  
    A tool to validate whether a LAS file conforms to the LAS specification

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
  
    3 September 2013 -- made open source after the ASPRS LVS contract fiasko
    1 April 2013 -- on Easter Monday all-nighting in Perth airport for PER->SYD
  
===============================================================================
*/

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lasreadopener.hpp"
#include "lasutility.hpp"
#include "xmlwriter.hpp"
#include "lascheck.hpp"

static void byebye(int return_code, BOOL wait=FALSE)
{
  if (wait)
  {
    fprintf(stderr,"<press ENTER>\n");
    getc(stdin);
  }
  exit(return_code);
}

static void usage(int return_code, BOOL wait=FALSE)
{
  fprintf(stderr,"Usage:\n");
  fprintf(stderr,"lasvalidate -v -i lidar.las\n");
  fprintf(stderr,"lasvalidate -i lidar.las -o validation.xml\n");
  fprintf(stderr,"lasvalidate -i lidar.las -oxml\n");
  fprintf(stderr,"lasvalidate -vv -i tile1.las tile2.las tile3.las -oxml\n");
  fprintf(stderr,"lasvalidate -i tile1.las tile2.las tile3.las -o summary.kml\n");
  fprintf(stderr,"lasvalidate -vv -i *.las\n");
  fprintf(stderr,"lasvalidate -i *.las -o summary.xml\n");
  fprintf(stderr,"lasvalidate -i *.las -oxml\n");
  fprintf(stderr,"lasvalidate -i c:\\data\\lidar.las -oxml\n");
  fprintf(stderr,"lasvalidate -i ..\\subfolder\\*.las -o summary.xml\n");
  fprintf(stderr,"lasvalidate -v -i ..\\..\\flight\\*.las -o oxml\n");
  fprintf(stderr,"lasvalidate -h\n");
  byebye(return_code, wait);
}

static double taketime()
{
  return (double)(clock())/CLOCKS_PER_SEC;
}

#define LAS_VALIDATE_SUCCESS                    0  // Program successfully executed all phases
#define LAS_VALIDATE_UNKNOWN_ERROR             -1  // Program failed for an undeterminable reason
#define LAS_VALIDATE_WRONG_COMMAND_LINE_SYNTAX -2  // The command line does not conform to the syntax the LAS validator is expecting
#define LAS_VALIDATE_NO_INPUT_SPECIFIED        -3  // The command line does not specify any LAS or LAZ files as input 
#define LAS_VALIDATE_INPUT_FILE_NOT_FOUND      -4  // The input file specified on the command line was not found
#define LAS_VALIDATE_INPUT_READ_ACCESS_ERROR   -5  // The LAS validator does not have read permission to a specified file or path
#define LAS_VALIDATE_WRITE_PERMISSION_ERROR    -6  // The LAS validator does not have write permission to the specified output directory

int main(int argc, char *argv[])
{
  int i;
  BOOL verbose = TRUE;
  BOOL very_verbose = TRUE;
  F64 start_time = 0.0;
  F64 full_start_time = 0.0;
  const CHAR* xml_output_file = 0;
  BOOL one_report_per_file = FALSE;
  U32 num_pass = 0;
  U32 num_fail = 0;
  U32 num_warning = 0;

  fprintf(stderr, "This is version %d of the LAS validator. Please contact\n", LASREAD_BUILD_DATE);
  fprintf(stderr, "me at 'martin.isenburg@rapidlasso.com' if you disagree with\n");
  fprintf(stderr, "a validation report, want additional checks, or find a bug.\n");

  LASreadOpener lasreadopener;

  if (argc == 1)
  {
    fprintf(stderr,"lasvalidate.exe is best run with arguments in the command line\n");
    char file_name[256];
    fprintf(stderr,"enter input LAS file name: "); fgets(file_name, 256, stdin);
    file_name[strlen(file_name)-1] = '\0';
    lasreadopener.set_file_name(file_name);
    fprintf(stderr,"enter output XML file name: "); fgets(file_name, 256, stdin);
    file_name[strlen(file_name)-1] = '\0';
    xml_output_file = strdup(file_name);
  }
  else
  {
    if (!lasreadopener.parse(argc, argv))
    {
      byebye(LAS_VALIDATE_WRONG_COMMAND_LINE_SYNTAX);
    }
  }

  for (i = 1; i < argc; i++)
  {
    if (argv[i][0] == '\0')
    {
      continue;
    }
    else if (strcmp(argv[i],"-version") == 0)
    {
      fprintf(stderr, "\nlasvalidate built %d with LASread (v %d.%d) and LAScheck (v %d.%d) by rapidlasso\n", LASREAD_BUILD_DATE, LASREAD_VERSION_MAJOR, LASREAD_VERSION_MINOR, LASCHECK_VERSION_MAJOR, LASCHECK_VERSION_MINOR);
      byebye(LAS_VALIDATE_SUCCESS);
    }
    else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0)
    {
      usage(LAS_VALIDATE_SUCCESS);
    }
    else if (strcmp(argv[i],"-v") == 0 || strcmp(argv[i],"-verbose") == 0)
    {
      verbose = TRUE;
    }
    else if (strcmp(argv[i],"-vv") == 0 || strcmp(argv[i],"-very_verbose") == 0)
    {
      verbose = TRUE;
      very_verbose = TRUE;
    }
    else if (strcmp(argv[i],"-o") == 0)
    {
      i++;
      xml_output_file = argv[i];
    }
    else if (strcmp(argv[i],"-oxml") == 0)
    {
      one_report_per_file = TRUE;
    }
    else
    {
      fprintf(stderr, "ERROR: cannot understand argument '%s'\n", argv[i]);
      usage(LAS_VALIDATE_WRONG_COMMAND_LINE_SYNTAX);
    }
  }

  // in verbose mode we measure the total time

  if (verbose) full_start_time = taketime();

  // check input

  if (!lasreadopener.is_active())
  {
    fprintf(stderr,"ERROR: no input specified\n");
    byebye(LAS_VALIDATE_NO_INPUT_SPECIFIED, argc == 1);
  }

  // output logging

  XMLwriter xmlwriter;

  if (lasreadopener.is_active())
  {
    if (xml_output_file)
    {
      one_report_per_file = FALSE;
    }
    else if (!one_report_per_file)
    {
      xml_output_file = "validate.xml";
    }
  }

  // maybe we are doing one summary report

  if (xml_output_file)
  {
    if (!xmlwriter.open(xml_output_file, "LASvalidator"))
    {
      byebye(LAS_VALIDATE_WRITE_PERMISSION_ERROR, argc == 1);
    }
  }

  // create LAScheck 

  LAScheck lascheck;

  // accumulated pass

  U32 total_pass = LAS_PASS;

  // possibly loop over multiple input files

  while (lasreadopener.is_active())
  {
    // in very verbose mode we measure the time for each file

    if (very_verbose) start_time = taketime();

    // open lasreader

    LASreader* lasreader = lasreadopener.open();
    if (lasreader == 0)
    {
      fprintf(stderr, "ERROR: could not open lasreader\n");
      byebye(LAS_VALIDATE_INPUT_FILE_NOT_FOUND, argc == 1);
    }    

    // parse points

    LASinventory lasinventory;

    while (lasreader->read_point())
    {
      lasinventory.add(&lasreader->point);
    }

    // check correctness (without output, but with CRS description)

    CHAR description[512];
    strcpy(description, "not valid or not specified");
    U32 pass = lascheck.check(&lasreader->header, &lasinventory, description);

    // maybe we are doing one report per file

    if (one_report_per_file)
    {
      int len = strlen(lasreadopener.get_path());
      CHAR* current_xml_output_file = (CHAR*)malloc(len + 5);
      strcpy(current_xml_output_file, lasreadopener.get_path());
      current_xml_output_file[len-4] = '_';
      current_xml_output_file[len-3] = 'L';
      current_xml_output_file[len-2] = 'V';
      current_xml_output_file[len-1] = 'S';
      current_xml_output_file[len  ] = '.';
      current_xml_output_file[len+1] = 'x';
      current_xml_output_file[len+2] = 'm';
      current_xml_output_file[len+3] = 'l';
      current_xml_output_file[len+4] = '\0';
      if (!xmlwriter.open(current_xml_output_file, "LASvalidator"))
      {
        byebye(LAS_VALIDATE_WRITE_PERMISSION_ERROR, argc == 1);
      }
      free(current_xml_output_file);
    }

    // start a new report
    
    xmlwriter.begin("report");

    // report description of file

    xmlwriter.beginsub("file");
    xmlwriter.write("name", lasreadopener.get_file_name());
    xmlwriter.write("path", lasreadopener.get_path());
    xmlwriter.write("version_major", lasreader->header.version_major);
    xmlwriter.write("version_minor", lasreader->header.version_minor);
    xmlwriter.write("point_data_format", lasreader->header.point_data_format);
    xmlwriter.write("CRS", description);
    xmlwriter.endsub("file");

    // report the verdict

    xmlwriter.beginsub("summary");
    xmlwriter.write((pass == LAS_PASS ? "pass" : ((pass & LAS_FAIL) ? "fail" : "warning")));
    xmlwriter.endsub("summary");

    // report details (if necessary)

    if (pass != LAS_PASS)
    {
      xmlwriter.beginsub("details");
      pass = lascheck.check(&lasreader->header, &lasinventory, 0, &xmlwriter);
      xmlwriter.endsub("details");
      total_pass |= pass;
      if (pass & LAS_FAIL)
      {
        num_fail++;
      }
      else
      {
        num_warning++;
      }
    }
    else
    {
      num_pass++;
    }

    // end the report

    xmlwriter.end("report");

    // maybe we are doing one report per file

    if (one_report_per_file)
    {
      // report the total verdict

      xmlwriter.begin("total");
      xmlwriter.write((total_pass == LAS_PASS ? "pass" : ((total_pass & LAS_FAIL) ? "fail" : "warning")));
      xmlwriter.beginsub("details");
      xmlwriter.write("pass", num_pass);
      xmlwriter.write("warning", num_warning);
      xmlwriter.write("fail", num_fail);
      xmlwriter.endsub("details");
      xmlwriter.end("total");

      num_pass = 0;
      num_warning = 0;
      num_fail = 0;

      // close the LASvalidator XML output file

      xmlwriter.close("LASvalidator");
    }

    lasreader->close();
    delete lasreader;

    // in very verbose mode we report the time for each file

    if (very_verbose)
    {
      fprintf(stderr,"needed %.2f sec for '%s'\n", taketime()-start_time, lasreadopener.get_file_name());
      start_time = taketime();
    }
  }

  // maybe we are doing one summary report

  if (!one_report_per_file)
  {
    // report the total verdict

    xmlwriter.begin("total");
    xmlwriter.write((total_pass == 0 ? "pass" : ((total_pass & 1) ? "fail" : "warning")));
    xmlwriter.beginsub("details");
    xmlwriter.write("pass", num_pass);
    xmlwriter.write("warning", num_warning);
    xmlwriter.write("fail", num_fail);
    xmlwriter.endsub("details");
    xmlwriter.end("total");

    // close the LASvalidator XML output file

    xmlwriter.close("LASvalidator");
  }

  // in verbose mode we report the total time

  if (verbose && (lasreadopener.get_file_name_number() > 1))
  {
    fprintf(stderr,"done. total time %.2f sec.\n", taketime()-full_start_time);
  }

  byebye(argc==1);

  return 0;
}
