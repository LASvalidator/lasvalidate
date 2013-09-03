Example calls to lasvalidate:

C:\LASvalidator\lasvalidate\bin> lasvalidate -i ..\data\*.las -oxml

and

C:\LASvalidator\lasvalidate\bin> lasvalidate -i ..\data\*.las -o ..\data\summary.xml

The source code already compiles under Windows and Linux and will
be provided here eventually.

--

Below is a longer test run. Note that some errors and warnings
that happen while loading the header and the VLRs are currently
output to the console instead of the XML file. Will get fixed.

C:\LASvalidator\lasvalidate\bin> lasvalidate -i mixed\*.las -o summary.xml

This is a prototype of the LAS validator
Please contact me at martin@rapidlasso.com before using it commercially.
needed 0.22 sec for '000000027_smaller.las'
needed 0.50 sec for '173750_457000_1_FME_uncompressed.las'
needed 0.50 sec for '3570_31470.las'
needed 0.80 sec for '3570_31470_1.las'
needed 0.56 sec for '3635_31450.las'
needed 0.95 sec for '3635_31450g.las'
WARNING: no payload for LASF_Projection (not specification-conform).
needed 0.05 sec for '46120h8314.las'
WARNING: no payload for LASF_Projection (not specification-conform).
needed 0.03 sec for '46123d1222.las'
needed 0.13 sec for 'brown_mp_lcc.las'
needed 0.22 sec for 'brown_mp_lcc_arclp.las'
needed 0.14 sec for 'brown_mp_lcc_arclp_grs80.las'
needed 0.83 sec for 'brown_mp_lcc_new.las'
needed 0.14 sec for 'brown_mp_utm.las'
needed 0.05 sec for 'Building_1_1.las'
needed 0.03 sec for 'Building_2_1.las'
needed 0.05 sec for 'Building_3_1.las'
needed 0.03 sec for 'Building_4_1.las'
needed 0.09 sec for 'ds_area1_original.las'
needed 0.17 sec for 'ds_area2_original.las'
needed 0.31 sec for 'E09D4_3.las'
needed 0.61 sec for 'e303n5996.las'
needed 0.34 sec for 'fusa.las'
needed 0.11 sec for 'horn.las'
needed 0.08 sec for 'horn_1.las'
needed 0.08 sec for 'horn_1_1.las'
needed 0.09 sec for 'horn_1_1_1.las'
needed 0.26 sec for 'kirche.las'
needed 0.24 sec for 'kirche_c.las'
needed 0.27 sec for 'kirche_r.las'
needed 0.22 sec for 'kirche_z.las'
needed 0.03 sec for 'las14_type7_fugro_sample.las'
needed 0.06 sec for 'las14_type7_globalmapper.las'
needed 0.03 sec for 'part1.las'
needed 0.11 sec for 'S11C2_3.las'
needed 0.42 sec for 's1480690.las'
needed 0.05 sec for 'section_classified.las'
needed 0.13 sec for 'single_scan.las'
needed 0.16 sec for 'SURFACE.las'
needed 0.19 sec for 'T13C4_3.las'
done. total time 9.36 sec.

C:\LASvalidator\lasvalidate\bin> more summary.xml

<?xml version="1.0" encoding="UTF-8"?>
<LASvalidator>
  <report>
    <file>
      <name>000000027_smaller.las</name>
      <path>000000027_smaller.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>173750_457000_1_FME_uncompressed.las</name>
      <path>173750_457000_1_FME_uncompressed.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>3</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>global encoding</variable>
        <note>unset bit 0 suggests GPS week time but GPS time ranges from 9.52161e+008 to 9.52163e+008</note>
      </fail>
      <warning>
        <variable>x scale factor</variable>
        <note>should be factor ten of 0.1 or 0.25 and not 5.15864e-008</note>
      </warning>
      <warning>
        <variable>y scale factor</variable>
        <note>should be factor ten of 0.1 or 0.25 and not 5.82075e-008</note>
      </warning>
      <warning>
        <variable>z scale factor</variable>
        <note>should be factor ten of 0.1 or 0.25 and not 7.69855e-009</note>
      </warning>
    </details>
  </report>
  <report>
    <file>
      <name>3570_31470.las</name>
      <path>3570_31470.las</path>
      <version_major>1</version_major>
      <version_minor>1</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 1st return(s) is 488249 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 2nd return(s) is 10459 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 3rd return(s) is 61 and not 0</note>
      </fail>
      <warning>
        <variable>GPS time</variable>
        <note>time stamps of all 498769 points are 0</note>
      </warning>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>3570_31470_1.las</name>
      <path>3570_31470_1.las</path>
      <version_major>1</version_major>
      <version_minor>1</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <warning>
        <variable>GPS time</variable>
        <note>time stamps of all 498769 points are 0</note>
      </warning>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>3635_31450.las</name>
      <path>3635_31450.las</path>
      <version_major>1</version_major>
      <version_minor>1</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 1st return(s) is 523807 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 2nd return(s) is 172705 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 3rd return(s) is 24589 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 4th return(s) is 1207 and not 0</note>
      </fail>
      <warning>
        <variable>GPS time</variable>
        <note>time stamps of all 722308 points are 0</note>
      </warning>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>3635_31450g.las</name>
      <path>3635_31450g.las</path>
      <version_major>1</version_major>
      <version_minor>1</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 1st return(s) is 523807 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 2nd return(s) is 172705 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 3rd return(s) is 24589 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 4th return(s) is 1207 and not 0</note>
      </fail>
      <warning>
        <variable>GPS time</variable>
        <note>time stamps of all 722308 points are 0</note>
      </warning>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>46120h8314.las</name>
      <path>46120h8314.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>46123d1222.las</name>
      <path>46123d1222.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>brown_mp_lcc.las</name>
      <path>brown_mp_lcc.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>brown_mp_lcc_arclp.las</name>
      <path>brown_mp_lcc_arclp.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>file creation year</variable>
        <note>should be between 1990 and 2013 and not 1970</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>brown_mp_lcc_arclp_grs80.las</name>
      <path>brown_mp_lcc_arclp_grs80.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>file creation year</variable>
        <note>should be between 1990 and 2013 and not 1970</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>brown_mp_lcc_new.las</name>
      <path>brown_mp_lcc_new.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>brown_mp_utm.las</name>
      <path>brown_mp_utm.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>Building_1_1.las</name>
      <path>Building_1_1.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>Building_2_1.las</name>
      <path>Building_2_1.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>Building_3_1.las</name>
      <path>Building_3_1.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>Building_4_1.las</name>
      <path>Building_4_1.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>ds_area1_original.las</name>
      <path>ds_area1_original.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
      <warning>
        <variable>return number</variable>
        <note>there are 57 points with a return number of 6</note>
      </warning>
      <warning>
        <variable>return number</variable>
        <note>there are 4 points with a return number of 7</note>
      </warning>
      <warning>
        <variable>return number</variable>
        <note>there are 317 points with a number of returns of given pulse of 6</note>
      </warning>
      <warning>
        <variable>return number</variable>
        <note>there are 28 points with a number of returns of given pulse of 7</note>
      </warning>
    </details>
  </report>
  <report>
    <file>
      <name>ds_area2_original.las</name>
      <path>ds_area2_original.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
      <warning>
        <variable>return number</variable>
        <note>there are 5 points with a return number of 6</note>
      </warning>
      <warning>
        <variable>return number</variable>
        <note>there are 30 points with a number of returns of given pulse of 6</note>
      </warning>
    </details>
  </report>
  <report>
    <file>
      <name>E09D4_3.las</name>
      <path>E09D4_3.las</path>
      <version_major>1</version_major>
      <version_minor>0</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <warning>
        <variable>system identifier</variable>
        <note>empty string. first character is '\0'</note>
      </warning>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 1st return(s) is 309017 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 2nd return(s) is 11918 and not 0</note>
      </fail>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>e303n5996.las</name>
      <path>e303n5996.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>global encoding</variable>
        <note>unset bit 0 suggests GPS week time but GPS time ranges from -2.37754e+007 to -2.37754e+007</note>
      </fail>
      <warning>
        <variable>system identifier</variable>
        <note>empty string. first character is '\0'</note>
      </warning>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>fusa.las</name>
      <path>fusa.las</path>
      <version_major>1</version_major>
      <version_minor>1</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>horn.las</name>
      <path>horn.las</path>
      <version_major>1</version_major>
      <version_minor>4</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>horn_1.las</name>
      <path>horn_1.las</path>
      <version_major>1</version_major>
      <version_minor>4</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>horn_1_1.las</name>
      <path>horn_1_1.las</path>
      <version_major>1</version_major>
      <version_minor>4</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>horn_1_1_1.las</name>
      <path>horn_1_1_1.las</path>
      <version_major>1</version_major>
      <version_minor>4</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>kirche.las</name>
      <path>kirche.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>0</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>kirche_c.las</name>
      <path>kirche_c.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>0</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>min x</variable>
        <note>should be 3.3757e+006 and not 3.3757e+006</note>
      </fail>
      <fail>
        <variable>max x</variable>
        <note>should be 3.376e+006 and not 3.376e+006</note>
      </fail>
      <fail>
        <variable>min y</variable>
        <note>should be 6.0202e+006 and not 6.0202e+006</note>
      </fail>
      <fail>
        <variable>max y</variable>
        <note>should be 6.0205e+006 and not 6.0205e+006</note>
      </fail>
      <fail>
        <variable>min z</variable>
        <note>should be -7.45 and not -7.44</note>
      </fail>
      <fail>
        <variable>max z</variable>
        <note>should be 108.89 and not 108.88</note>
      </fail>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>kirche_r.las</name>
      <path>kirche_r.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>0</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>min x</variable>
        <note>should be 3.3757e+006 and not 3.3757e+006</note>
      </fail>
      <fail>
        <variable>max x</variable>
        <note>should be 3.376e+006 and not 3.376e+006</note>
      </fail>
      <fail>
        <variable>min y</variable>
        <note>should be 6.0202e+006 and not 6.0202e+006</note>
      </fail>
      <fail>
        <variable>max y</variable>
        <note>should be 6.0205e+006 and not 6.0205e+006</note>
      </fail>
      <fail>
        <variable>min z</variable>
        <note>should be -16.57 and not -16.56</note>
      </fail>
      <fail>
        <variable>max z</variable>
        <note>should be 99.08 and not 99.07</note>
      </fail>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>kirche_z.las</name>
      <path>kirche_z.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>0</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>las14_type7_fugro_sample.las</name>
      <path>las14_type7_fugro_sample.las</path>
      <version_major>1</version_major>
      <version_minor>4</version_minor>
      <point_data_format>7</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      pass
    </summary>
  </report>
  <report>
    <file>
      <name>las14_type7_globalmapper.las</name>
      <path>las14_type7_globalmapper.las</path>
      <version_major>1</version_major>
      <version_minor>4</version_minor>
      <point_data_format>7</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>global encoding</variable>
        <note>bit 4 must be set (OGC WKT must be used) for point data format 7</note>
      </fail>
      <warning>
        <variable>system identifier</variable>
        <note>empty string. first character is '\0'</note>
      </warning>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of  1st return(s) is 0 and not 22600</note>
      </fail>
      <warning>
        <variable>return number</variable>
        <note>there are 22600 points with a return number of 0</note>
      </warning>
      <warning>
        <variable>number of returns of given pulse</variable>
        <note>there are 22600 points with a number of returns of given pulse of 0</note>
      </warning>
      <warning>
        <variable>GPS time</variable>
        <note>time stamps of all 22600 points are 0</note>
      </warning>
    </details>
  </report>
  <report>
    <file>
      <name>part1.las</name>
      <path>part1.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 1st return(s) is 0 and not 26282</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 2nd return(s) is 0 and not 4986</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 3rd return(s) is 0 and not 66</note>
      </fail>
      <fail>
        <variable>min x</variable>
        <note>should be 0 and not 619584</note>
      </fail>
      <fail>
        <variable>min y</variable>
        <note>should be 0 and not 7.5432e+006</note>
      </fail>
      <fail>
        <variable>min z</variable>
        <note>should be 0 and not 308.81</note>
      </fail>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>S11C2_3.las</name>
      <path>S11C2_3.las</path>
      <version_major>1</version_major>
      <version_minor>0</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <warning>
        <variable>system identifier</variable>
        <note>empty string. first character is '\0'</note>
      </warning>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 1st return(s) is 44157 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 2nd return(s) is 409 and not 0</note>
      </fail>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>s1480690.las</name>
      <path>s1480690.las</path>
      <version_major>1</version_major>
      <version_minor>0</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <warning>
        <variable>system identifier</variable>
        <note>empty string. first character is '\0'</note>
      </warning>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 1st return(s) is 423542 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 2nd return(s) is 21693 and not 0</note>
      </fail>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>section_classified.las</name>
      <path>section_classified.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>3</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <warning>
        <variable>GPS time</variable>
        <note>time stamps of all 32085 points are 0</note>
      </warning>
      <warning>
        <variable>RGB</variable>
        <note>color of all 32085 points is (0/0/0)</note>
      </warning>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>single_scan.las</name>
      <path>single_scan.las</path>
      <version_major>1</version_major>
      <version_minor>3</version_minor>
      <point_data_format>4</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      warning
    </summary>
    <details>
      <warning>
        <variable>return number</variable>
        <note>there are 1 points with a return number of 6</note>
      </warning>
      <warning>
        <variable>return number</variable>
        <note>there are 6 points with a number of returns of given pulse of 6</note>
      </warning>
    </details>
  </report>
  <report>
    <file>
      <name>SURFACE.las</name>
      <path>SURFACE.las</path>
      <version_major>1</version_major>
      <version_minor>2</version_minor>
      <point_data_format>0</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <report>
    <file>
      <name>T13C4_3.las</name>
      <path>T13C4_3.las</path>
      <version_major>1</version_major>
      <version_minor>0</version_minor>
      <point_data_format>1</point_data_format>
      <CRS>not implemented (yet)</CRS>
    </file>
    <summary>
      fail
    </summary>
    <details>
      <warning>
        <variable>system identifier</variable>
        <note>empty string. first character is '\0'</note>
      </warning>
      <fail>
        <variable>file creation day</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>file creation year</variable>
        <note>not set</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 1st return(s) is 69555 and not 0</note>
      </fail>
      <fail>
        <variable>number of point by return</variable>
        <note>the number of 2nd return(s) is 7 and not 0</note>
      </fail>
      <fail>
        <variable>CRS</variable>
        <note>file does not specify a Coordinate Reference System</note>
      </fail>
    </details>
  </report>
  <total>
    fail
    <details>
      <pass>1</pass>
      <warning>1</warning>
      <fail>37</fail>
    </details>
  </total>
</LASvalidator>
