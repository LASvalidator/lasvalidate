.. raw:: pdf

    SetPageCounter 1 arabic

.. footer::

   This document describes all of the checks that lasvalidate puts LAS files through.

   Page ###Page###

Date: 

initial draft created on May 10th, 2013

***************************************************************************************
 lasvalidate - a tool for validating LAS files
***************************************************************************************

.. class:: heading4
    
This document describes all of the checks that lasvalidate puts LAS files through.

==============================================================================
Introduction
==============================================================================

This document describes all of the checks that lasvalidate puts LAS files through.

==============================================================================
Header checks
==============================================================================

.. csv-table:: File Signature (char[4])
    :widths: 10, 50, 10

    "1.X", "must always equal 'LASF'", "fail"

The file signature must contain the four characters "LASF".

.. csv-table:: Global Encoding
    :widths: 10, 50, 10

    "1.0", "must be less or equal 1", "fail"
    "1.1", "must be less or equal 1", "fail"
    "1.2", "must be less or equal 1", "fail"
    "1.3", "must be less or equal 7", "fail"
    "1.4", "must be less or equal 31", "fail"

The Global Encoding bit field must have all those bits set to zero that are undefined in the specification of the respective version of the LAS file.
