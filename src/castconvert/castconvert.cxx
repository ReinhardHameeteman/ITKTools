/*=========================================================================
*
* Copyright Marius Staring, Stefan Klein, David Doria. 2011.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0.txt
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
*=========================================================================*/
/** \file
 \brief This program converts and possibly casts images.
 
 \verbinclude castconvert.help
 */
/*
 * authors:       Marius Staring and Stefan Klein
 *
 * Thanks to Hans J. Johnson for a modification to this program. This
 * modification breaks down the program into smaller compilation units,
 * so that the compiler does not overflow.
 *
 * ENH: on 23-05-2006 we added multi-component support.
 * ENH: on 09-06-2006 we added support for extracting a specific DICOM serie.
 */

#include <iostream>
#include "castconverthelpers2.h"

#include "CommandLineArgumentHelper.h"
#include "itkCommandLineArgumentParser.h"

#include "itkImageFileReader.h"
#include "itkGDCMImageIO.h"

// Some non-standard IO Factories
#include "itkGE4ImageIOFactory.h"
#include "itkGE5ImageIOFactory.h"
#include "itkGEAdwImageIOFactory.h"
//#include "itkBrains2MaskImageIOFactory.h"
#include "itkPhilipsRECImageIOFactory.h"

/* Functions to do the actual conversion. */
extern int FileConverterScalar(
  const std::string & inputPixelComponentType,
  const std::string & outputPixelComponentType,
  const std::string & inputFileName,
  const std::string & outputFileName,
  const unsigned int inputDimension, bool useCompression );
extern int DicomFileConverterScalarA(
  const std::string & inputPixelComponentType,
  const std::string & outputPixelComponentType,
  const std::string & inputDirectoryName,
  const std::string & seriesUID,
  const std::vector<std::string> & restrictions,
  const std::string & outputFileName,
  const unsigned int inputDimension, bool useCompression );
extern int DicomFileConverterScalarB(
  const std::string & inputPixelComponentType,
  const std::string & outputPixelComponentType,
  const std::string & inputDirectoryName,
  const std::string & seriesUID,
  const std::vector<std::string> & restrictions,
  const std::string & outputFileName,
  const unsigned int inputDimension, bool useCompression );
extern int FileConverterMultiComponent(
  const std::string & inputPixelComponentType,
  const std::string & outputPixelComponentType,
  const unsigned int numberOfComponents,
  const std::string & inputFileName,
  const std::string & outputFileName,
  const unsigned int inputDimension, bool useCompression );

//-------------------------------------------------------------------------------------

int main( int argc, char **argv )
{
  /** Register some non-standard IO Factories to make the tool more useful.
   * Copied from the Insight Applications.
   */
  //itk::Brains2MaskImageIOFactory::RegisterOneFactory();
  itk::GE4ImageIOFactory::RegisterOneFactory();
  itk::GE5ImageIOFactory::RegisterOneFactory();
  itk::GEAdwImageIOFactory::RegisterOneFactory();
  itk::PhilipsRECImageIOFactory::RegisterOneFactory();

  itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
  parser->SetCommandLineArguments( argc, argv );
  parser->SetProgramHelpText( GetHelpString() );

  parser->MarkArgumentAsRequired( "-in", "The input filename." );
  parser->MarkArgumentAsRequired( "-out", "The output filename." );

  /** Get the command line arguments. */
  std::string input = "";
  std::string outputFileName = "";
  std::string outputPixelComponentType = "";
  std::string seriesUID = "";
  std::vector<std::string> restrictions;
  std::string errorMessage = "";
  bool useCompression = false;
  bool argsRetrievedSuccessfully = GetCommandLineArguments( parser,
    input, outputFileName, outputPixelComponentType,
    seriesUID, restrictions, useCompression );

  itk::CommandLineArgumentParser::ReturnValue validateArguments = parser->CheckForRequiredArguments();

  if(validateArguments == itk::CommandLineArgumentParser::FAILED || !argsRetrievedSuccessfully)
  {
    return EXIT_FAILURE;
  }
  else if(validateArguments == itk::CommandLineArgumentParser::HELPREQUESTED)
  {
    return EXIT_SUCCESS;
  }

  /** Are we dealing with an image or a dicom series? */
  bool isDICOM = false;
  int returnValue2 = IsDICOM( input, errorMessage, isDICOM );
  if ( returnValue2 )
  {
    std::cout << errorMessage << std::endl;
    return returnValue2;
  }

  /** Get inputFileName or inputDirectoryName. */
  std::string inputFileName, inputDirectoryName;
  if ( !isDICOM ) inputFileName = input;
  else inputDirectoryName = input;

  /** TASK 2:
   * Typedefs and test reading to determine correct image types.
   * *******************************************************************
   */

  /** Initial image type. */
  const unsigned int Dimension = 3;
  typedef short      PixelType;

  /** Some typedef's. */
  typedef itk::Image< PixelType, Dimension >   ImageType;
  typedef itk::ImageFileReader< ImageType >    ReaderType;
  typedef itk::ImageIOBase                     ImageIOBaseType;
  typedef itk::GDCMImageIO                     GDCMImageIOType;

  /** Create a testReader. */
  ReaderType::Pointer testReader = ReaderType::New();

  /** Setup the testReader. */
  if ( !isDICOM )
  {
    /** Set the inputFileName in the testReader. */
    testReader->SetFileName( inputFileName.c_str() );
  }
  else
  {
    std::string fileName = "";
    int returnValue3 = GetFileNameFromDICOMDirectory(
      inputDirectoryName, fileName, seriesUID, restrictions, errorMessage );
    if ( returnValue3 )
    {
      std::cout << errorMessage << std::endl;
      return returnValue3;
    }

    /** Create a dicom ImageIO and set it in the testReader. */
    GDCMImageIOType::Pointer dicomIO = GDCMImageIOType::New();
    testReader->SetImageIO( dicomIO );

    /** Set the name of the 2D dicom image in the testReader. */
    testReader->SetFileName( fileName.c_str() );

  } // end isDICOM

  /** Generate all information. */
  try
  {
    testReader->GenerateOutputInformation();
  }
  catch( itk::ExceptionObject  &  err  )
  {
    std::cerr  << "ExceptionObject caught !"  << std::endl;
    std::cerr  << err <<  std::endl;
    return 1;
  }

  /** Extract the ImageIO from the testReader. */
  ImageIOBaseType::Pointer testImageIOBase = testReader->GetImageIO();

  /** Get the component type, number of components, dimension and pixel type. */
  unsigned int inputDimension = testImageIOBase->GetNumberOfDimensions();
  unsigned int numberOfComponents = testImageIOBase->GetNumberOfComponents();
  std::string inputPixelComponentType = testImageIOBase->GetComponentTypeAsString(
    testImageIOBase->GetComponentType() );
  std::string pixelType = testImageIOBase->GetPixelTypeAsString(
    testImageIOBase->GetPixelType() );

  /** TASK 3:
   * Do some preparations.
   * *******************************************************************
   */

  /** Check inputPixelType. */
  if ( inputPixelComponentType != "unsigned_char"
    && inputPixelComponentType != "char"
    && inputPixelComponentType != "unsigned_short"
    && inputPixelComponentType != "short"
    && inputPixelComponentType != "unsigned_int"
    && inputPixelComponentType != "int"
    && inputPixelComponentType != "unsigned_long"
    && inputPixelComponentType != "long"
    && inputPixelComponentType != "float"
    && inputPixelComponentType != "double" )
  {
    /** In this case an illegal inputPixelComponentType is found. */
    std::cerr << "The found inputPixelComponentType is \"" << inputPixelComponentType
      << "\", which is not supported." << std::endl;
    return 1;
  }

  /** Check outputPixelType. */
  if ( outputPixelComponentType == "" )
  {
    /** In this case this option is not given, and by default
    * we set it to the inputPixelComponentType.
    */
    outputPixelComponentType = inputPixelComponentType;
  }

  /** Get rid of the "_" in inputPixelComponentType and outputPixelComponentType. */
  ReplaceUnderscoreWithSpace( inputPixelComponentType );
  ReplaceUnderscoreWithSpace( outputPixelComponentType );

  /** TASK 4:
   * Now we are ready to check on image type and subsequently call the
   * correct ReadCastWrite-function.
   * *******************************************************************
   */

  try
  {
    if ( !isDICOM )
    {
      /**
       * ****************** Support for SCALAR pixel types. **********************************
       */
      if ( pixelType == "scalar" && numberOfComponents == 1 )
      {
        const int ret_value = FileConverterScalar(
          inputPixelComponentType, outputPixelComponentType,
          inputFileName, outputFileName, inputDimension, useCompression );
        if ( ret_value != 0 )
        {
          return ret_value;
        }
      } // end scalar support
      /**
       * ****************** Support for multi-component pixel types. **********************************
       */
      else if ( numberOfComponents > 1 )
      {
        const int ret_value = FileConverterMultiComponent(
          inputPixelComponentType, outputPixelComponentType, numberOfComponents,
          inputFileName, outputFileName, inputDimension, useCompression );
        if ( ret_value != 0 )
        {
          return ret_value;
        }
      } // end multi-component support
      else
      {
        std::cerr << "Pixel type is " << pixelType
          << ", component type is " << inputPixelComponentType
          << " and number of components equals " << numberOfComponents << "." << std::endl;
        std::cerr << "ERROR: This image type is not supported." << std::endl;
        return 1;
      }
    } // end NonDicom image
    else
    {
      /** In this case input is a DICOM series, from which we only support
       * SCALAR pixel types, with component type:
       * DICOMImageIO2: (unsigned) char, (unsigned) short, float
       * GDCMImageIO: (unsigned) char, (unsigned) short, (unsigned) int, double
       * It is also assumed that the dicom series consist of multiple
       * 2D images forming a 3D image.
       */

      if ( strcmp( pixelType.c_str(), "scalar" ) == 0 && numberOfComponents == 1 )
      {
        const int ret_value = DicomFileConverterScalarA(
          inputPixelComponentType, outputPixelComponentType,
          inputDirectoryName, seriesUID, restrictions,
          outputFileName, inputDimension, useCompression )
          || DicomFileConverterScalarB(
          inputPixelComponentType, outputPixelComponentType,
          inputDirectoryName, seriesUID, restrictions,
          outputFileName, inputDimension, useCompression );
        if ( ret_value != 0 )
        {
          return ret_value;
        }
      }
      else
      {
        std::cerr << "Pixel type is " << pixelType
          << ", component type is " << inputPixelComponentType
          << " and number of components equals " << numberOfComponents << "." << std::endl;
        std::cerr << "ERROR: This image type is not supported." << std::endl;
        return 1;
      }

    } // end isDICOM

  } // end try
  /** If any errors have occurred, catch and print the exception and return false. */
  catch ( itk::ExceptionObject & err )
  {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return 1;
  }

  /** End  program. Return success. */
  return 0;

}  // end main
