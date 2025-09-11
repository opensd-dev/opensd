.. _io_settings:

======================================
Settings Specification -- settings.xml
======================================

All simulation parameters and miscellaneous options are specified in the
settings.xml file.

-----------------------
``<verbosity>`` Element
-----------------------

The ``<verbosity>`` element tells the code how much information to display to
the standard output/files. A higher verbosity corresponds to more information being
displayed/written to files. The text of this element should be an integer between between 1
and 10. The verbosity levels are defined as follows:

  :1: don't display any output
  :2: only show OpenSD logo
  :3: all of the above + headers
  :4: all of the above + results
  :5: all of the above + timing statistics and initialization messages
  :6: all of the above + debug file I/O
  :10: all of the above + event information

  *Default*: 7
