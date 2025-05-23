.. _rgb2pct:

================================================================================
rgb2pct
================================================================================

.. only:: html

    Convert a 24bit RGB image to 8bit paletted.

.. Index:: rgb2pct

Synopsis
--------

.. code-block::

    rgb2pct [--help] [--help-general] [--creation-option OPTION]
               [-n colors | -pct palette_file] [-of format] <source_file> <dest_file>

Description
-----------

This utility will compute an optimal pseudo-color table for a given RGB image
using a median cut algorithm on a downsampled RGB histogram.   Then it
converts the image into a pseudo-colored image using the color table.
This conversion utilizes Floyd-Steinberg dithering (error diffusion) to
maximize output image visual quality.

.. note::

    rgb2pct is a Python utility, and is only available if GDAL Python bindings are available.

.. program:: rgb2pct

.. include:: options/help_and_help_general.rst

.. option:: -n <color>

    Select the number of colors in the generated
    color table.  Defaults to 256.  Must be between 2 and 256.

.. option:: --creation-option OPTION

    Optional creation parameters for the GeoTIFF driver,
    for example "COMPRESS=LZW". Can be specified multiple times.

.. option:: -pct <palette_file>

    Extract the color table from <palette_file> instead of computing it.
    Can be used to have a consistent color table for multiple files.
    The <palette_file> must be either a raster file in a GDAL supported format with a palette
    or a color file in a supported format (txt, qml, qlr).

.. option:: -of <format>

    Select the output format. Starting with
    GDAL 2.3, if not specified, the format is guessed from the extension (previously
    was GTiff). Use the short format name. Only output formats
    supporting pseudo-color tables should be used.

.. option:: <source_file>

    The input RGB file.

.. option:: <dest_file>

    The output pseudo-colored file that will be created.

Example
-------

If it is desired to hand create the palette, likely the simplest text format
is the GDAL VRT format.  In the following example a VRT was created in a
text editor with a small 4 color palette with the RGBA colors 238/238/238/255,
237/237/237/255, 236/236/236/255 and 229/229/229/255.

.. code-block:: console

    $ rgb2pct -pct palette.vrt rgb.tif pseudo-colored.tif
    $ more < palette.vrt
    <VRTDataset rasterXSize="226" rasterYSize="271">
        <VRTRasterBand dataType="Byte" band="1">
            <ColorInterp>Palette</ColorInterp>
            <ColorTable>
            <Entry c1="238" c2="238" c3="238" c4="255"/>
            <Entry c1="237" c2="237" c3="237" c4="255"/>
            <Entry c1="236" c2="236" c3="236" c4="255"/>
            <Entry c1="229" c2="229" c3="229" c4="255"/>
            </ColorTable>
        </VRTRasterBand>
    </VRTDataset>
