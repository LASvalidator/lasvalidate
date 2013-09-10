/*
===============================================================================

  FILE:  xmlwriter.hpp
  
  CONTENTS:
  
    Writes a LAScheck report to a file in a very simple XML format.

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
  
    1 April 2013 -- on Easter Monday all-nighting in Perth airport for PER->SYD

===============================================================================
*/
#ifndef XML_WRITER_HPP
#define XML_WRITER_HPP

#include "mydefs.hpp"

#include <stdio.h>

class XMLwriter
{
public:

  BOOL is_open() const;
  BOOL open(const CHAR* file_name, const CHAR* key);
  BOOL begin(const CHAR* key);
  BOOL beginsub(const CHAR* key);
  BOOL write(I32 value);
  BOOL write(const CHAR* value);
  BOOL write(const CHAR* key, int value);
  BOOL write(const CHAR* key, const CHAR* value);
  BOOL write(const CHAR* variable, const CHAR* key, const CHAR* note);
  BOOL endsub(const CHAR* key);
  BOOL end(const CHAR* key);
  BOOL close(const CHAR* key);

  XMLwriter();
  ~XMLwriter();

private:
  BOOL sub;
  FILE* file;
};

#endif
