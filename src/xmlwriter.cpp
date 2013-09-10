/*
===============================================================================

  FILE:  xmlwriter.cpp
  
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
#include "xmlwriter.hpp"

XMLwriter::XMLwriter()
{
  sub = FALSE;
  file = 0;
}

XMLwriter::~XMLwriter()
{
  if (file && (file != stdout)) fclose(file);
}

BOOL XMLwriter::is_open() const
{
  return (BOOL)(file != 0);
}

BOOL XMLwriter::open(const CHAR* file_name, const CHAR* key)
{
  if (file_name)
  {
    file = fopen(file_name, "w");
    if (file == 0)
    {
      fprintf(stderr,"ERROR: cannot open XML file '%s'\n", file_name);
      return FALSE;
    }
  }
  else
  {
    file = stdout;
  }
  fprintf(file, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\012");
  fprintf(file, "<%s>\012", key);
  return TRUE;
}

BOOL XMLwriter::begin(const CHAR* key)
{
  fprintf(file, "  <%s>\012", key);
  return TRUE;
}

BOOL XMLwriter::beginsub(const CHAR* key)
{
  if (sub)
  {
    return FALSE;
  }
  sub = TRUE;
  fprintf(file, "    <%s>\012", key);
  return TRUE;
}

BOOL XMLwriter::write(const CHAR* value)
{
  if (sub)
  {
    fprintf(file, "      %s\012", value);
  }
  else
  {
    fprintf(file, "    %s\012", value);
  }
  return TRUE;
}

BOOL XMLwriter::write(int value)
{
  if (sub)
  {
    fprintf(file, "      %d\012", value);
  }
  else
  {
    fprintf(file, "    %d\012", value);
  }
  return TRUE;
}

BOOL XMLwriter::write(const CHAR* key, const CHAR* value)
{
  if (sub)
  {
    fprintf(file, "      <%s>%s</%s>\012", key, value, key);
  }
  else
  {
    fprintf(file, "    <%s>%s</%s>\012", key, value, key);
  }
  return TRUE;
}

BOOL XMLwriter::write(const CHAR* key, I32 value)
{
  if (sub)
  {
    fprintf(file, "      <%s>%d</%s>\012", key, value, key);
  }
  else
  {
    fprintf(file, "    <%s>%d</%s>\012", key, value, key);
  }
  return TRUE;
}

BOOL XMLwriter::write(const CHAR* variable, const CHAR* key, const CHAR* note)
{
  if (sub)
  {
    fprintf(file, "      <%s>\012", key);
    fprintf(file, "        <variable>%s</variable>\012", variable);
    if (note)
    {
      fprintf(file, "        <note>%s</note>\012", note);
    }
    fprintf(file, "      </%s>\012", key);
  }
  else
  {
    fprintf(file, "    <%s>\012", key);
    fprintf(file, "      <variable>%s</variable>\012", variable);
    if (note)
    {
      fprintf(file, "      <note>%s</note>\012", note);
    }
    fprintf(file, "    </%s>\012", key);
  }
  return TRUE;
}

BOOL XMLwriter::endsub(const CHAR* key)
{
  if (!sub)
  {
    return FALSE;
  }
  sub = FALSE;
  fprintf(file, "    </%s>\012", key);
  return TRUE;
}

BOOL XMLwriter::end(const CHAR* key)
{
  fprintf(file, "  </%s>\012", key);
  return TRUE;
}

BOOL XMLwriter::close(const CHAR* key)
{
  fprintf(file, "</%s>\012", key);
  if (file != stdout) fclose(file);
  file = 0;
  return TRUE;
}
