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
#include "xmlwriter.hpp"
#include "lascheck.hpp"

#define VALIDATE_VERSION  131025

#define VALIDATE_PASS     0x0000
#define VALIDATE_FAIL     0x0001
#define VALIDATE_WARNING  0x0002

static void write_version(XMLwriter& xmlwriter)
{
  CHAR version[256];
  sprintf(version, "%d built with LASread version %d.%d (%d)", VALIDATE_VERSION, LASREAD_VERSION_MAJOR, LASREAD_VERSION_MINOR, LASREAD_BUILD_DATE);
  xmlwriter.begin("version");
  xmlwriter.write(version);
  xmlwriter.end("version");
}

static void write_command_line(XMLwriter& xmlwriter, int argc, char *argv[])
{
  int i, l = 0;
  CHAR command_line[4096];
  for (i = 0; i < argc; i++)
  {
    l += sprintf(command_line + l, "%s ", argv[i]);
  }
  xmlwriter.begin("command_line");
  xmlwriter.write(command_line);
  xmlwriter.end("command_line");
}

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
  fprintf(stderr,"lasvalidate -i lidar.las\n");
  fprintf(stderr,"lasvalidate -i lidar.laz\n");
  fprintf(stderr,"lasvalidate -v -i lidar.las -o report.xml\n");
  fprintf(stderr,"lasvalidate -v -i lidar.laz -oxml\n");
  fprintf(stderr,"lasvalidate -vv -i tile1.las tile2.las tile3.las -oxml\n");
  fprintf(stderr,"lasvalidate -i tile1.laz tile2.laz tile3.laz -o summary.kml\n");
  fprintf(stderr,"lasvalidate -vv -i *.las\n");
  fprintf(stderr,"lasvalidate -i *.laz -o summary.xml\n");
  fprintf(stderr,"lasvalidate -i *.las -oxml\n");
  fprintf(stderr,"lasvalidate -i c:\\data\\lidar.las -oxml\n");
  fprintf(stderr,"lasvalidate -i ..\\subfolder\\*.las -o summary.xml\n");
  fprintf(stderr,"lasvalidate -v -i ..\\..\\flight\\*.laz -o oxml\n");
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

  fprintf(stderr, "This is version %d of the LAS validator. Please contact\n", VALIDATE_VERSION);
  fprintf(stderr, "me at 'martin.isenburg@rapidlasso.com' if you disagree with\n");
  fprintf(stderr, "validation reports, want additional checks, or find bugs as\n");
  fprintf(stderr, "the software is still under development. Your feedback will\n");
  fprintf(stderr, "help to finish it sooner.\n");

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

  for (i = 1; i < argc; i++)
  {
    if (strcmp(argv[i],"-version") == 0)
    {
      fprintf(stderr, "\nlasvalidate %d with LASread (v %d.%d) and LAScheck (v %d.%d) by rapidlasso GmbH\n", LASREAD_BUILD_DATE, LASREAD_VERSION_MAJOR, LASREAD_VERSION_MINOR, LASCHECK_VERSION_MAJOR, LASCHECK_VERSION_MINOR);
      byebye(LAS_VALIDATE_SUCCESS);
    }
    else if (strcmp(argv[i],"-h") == 0 || strcmp(argv[i],"-help") == 0)
    {
      lasreadopener.usage();
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
    else if (strcmp(argv[i],"-i") == 0)
    {
      if ((i+1) >= argc)
      {
        fprintf(stderr,"ERROR: '%s' needs at least 1 argument: file_name or wild_card\n", argv[i]);
        usage(LAS_VALIDATE_WRONG_COMMAND_LINE_SYNTAX);
      }
      i+=1;
      do
      {
        lasreadopener.add_file_name(argv[i]);
        i+=1;
      } while (i < argc && *argv[i] != '-');
      i-=1;
    }
    else if (strcmp(argv[i],"-irec") == 0)
    {
      if ((i+1) >= argc)
      {
        fprintf(stderr,"ERROR: '%s' needs at least 1 argument: directory_name\n", argv[i]);
        usage(LAS_VALIDATE_WRONG_COMMAND_LINE_SYNTAX);
      }
      i+=1;
      do
      {
        lasreadopener.add_directory(argv[i], TRUE);
        i+=1;
      } while (i < argc && *argv[i] != '-');
      i-=1;
    }
    else if (strcmp(argv[i],"-stdin") == 0)
    {
      lasreadopener.set_piped(TRUE);
    }
    else if (strcmp(argv[i],"-lof") == 0)
    {
      if ((i+1) >= argc)
      {
        fprintf(stderr,"ERROR: '%s' needs 1 argument: list_of_files\n", argv[i]);
        usage(LAS_VALIDATE_WRONG_COMMAND_LINE_SYNTAX);
      }
      FILE* file = fopen(argv[i+1], "r");
      if (file == 0)
      {
        fprintf(stderr, "ERROR: cannot open '%s'\n", argv[i+1]);
        return FALSE;
      }
      char line[1024];
      while (fgets(line, 1024, file))
      {
        // find end of line
        int len = strlen(line) - 1;
        // remove extra white spaces and line return at the end 
        while (len > 0 && ((line[len] == '\n') || (line[len] == ' ') || (line[len] == '\t') || (line[len] == '\012')))
        {
          line[len] = '\0';
          len--;
        }
        lasreadopener.add_file_name(line);
      }
      fclose(file);
      i+=1;
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

  // accumulated pass

  U32 total_pass = VALIDATE_PASS;

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

    // get a pointer to the header
    LASheader* lasheader = &lasreader->header;

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
    CHAR temp[32];
    sprintf(temp, "%d.%d", lasheader->version_major, lasheader->version_minor);
    xmlwriter.write("version", temp);
    strncpy(temp, lasheader->system_identifier, 32);
    temp[31] = '\0';
    xmlwriter.write("system_identifier", temp);
    strncpy(temp, lasheader->generating_software, 32);
    temp[31] = '\0';
    xmlwriter.write("generating_software", temp);
    xmlwriter.write("point_data_format", lasheader->point_data_format);

    CHAR crsdescription[512];
    strcpy(crsdescription, "not valid or not specified");

    if (lasheader->fails == 0)
    {
      // header was loaded. now parse and check.

      LAScheck lascheck(lasheader);

      while (lasreader->read_point())
      {
        lascheck.parse(&lasreader->point);
      }

      // check header and points and get CRS description

      lascheck.check(lasheader, crsdescription);
    }

    xmlwriter.write("CRS", crsdescription);
    xmlwriter.endsub("file");    

    // report the verdict

    U32 pass = (lasheader->fails ? VALIDATE_FAIL : VALIDATE_PASS);
    if (lasheader->warnings) pass |= VALIDATE_WARNING;

    xmlwriter.beginsub("summary");
    xmlwriter.write((pass == VALIDATE_PASS ? "pass" : ((pass & VALIDATE_FAIL) ? "fail" : "warning")));
    xmlwriter.endsub("summary");

    // report details (if necessary)

    if (pass != VALIDATE_PASS)
    {
      xmlwriter.beginsub("details");
      for (i = 0; i < lasheader->fail_num; i+=2)
      {
        xmlwriter.write(lasheader->fails[i], "fail", lasheader->fails[i+1]);
      }
      for (i = 0; i < lasheader->warning_num; i+=2)
      {
        xmlwriter.write(lasheader->warnings[i], "warning", lasheader->warnings[i+1]);
      }
      xmlwriter.endsub("details");
      total_pass |= pass;
      if (pass & VALIDATE_FAIL)
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
      xmlwriter.write((total_pass == VALIDATE_PASS ? "pass" : ((total_pass & VALIDATE_FAIL) ? "fail" : "warning")));
      xmlwriter.beginsub("details");
      xmlwriter.write("pass", num_pass);
      xmlwriter.write("warning", num_warning);
      xmlwriter.write("fail", num_fail);
      xmlwriter.endsub("details");
      xmlwriter.end("total");

      num_pass = 0;
      num_warning = 0;
      num_fail = 0;

      // write which validator was used

      write_version(xmlwriter);

      // write which command line was used

      write_command_line(xmlwriter, argc, argv);

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

    // write which validator was used

    write_version(xmlwriter);

    // write which command line was used

    write_command_line(xmlwriter, argc, argv);

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
