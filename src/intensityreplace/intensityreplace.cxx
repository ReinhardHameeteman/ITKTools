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
 \brief This program replaces some user specified intensity values in an image.
 
 \verbinclude intensityreplace.help
 */
#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"

#include "itkImageFileReader.h"
#include "itkChangeLabelImageFilter.h"
#include "itkImageFileWriter.h"

//-------------------------------------------------------------------------------------

/** run: A macro to call a function. */
#define run( function, type, dim ) \
if ( ComponentType == #type && Dimension == dim ) \
{ \
  function< type, dim >( inputFileName, outputFileName, inValues, outValues ); \
  supported = true; \
}

//-------------------------------------------------------------------------------------

/* Declare IntensityReplaceImageFilter. */
template< class TOutputPixel, unsigned int NDimension >
void IntensityReplaceImageFilter(
  const std::string & inputFileName,
  const std::string & outputFileName,
  const std::vector<std::string> & inValues,
  const std::vector<std::string> & outValues );

/** Declare GetHelpString. */
std::string GetHelpString( void );

//-------------------------------------------------------------------------------------

int main( int argc, char ** argv )
{
  /** Create a command line argument parser. */
  itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
  parser->SetCommandLineArguments( argc, argv );
  parser->SetProgramHelpText( GetHelpString() );

  parser->MarkArgumentAsRequired( "-in", "The input filename." );
  parser->MarkArgumentAsRequired( "-i", "In values." );
  parser->MarkArgumentAsRequired( "-o", "Out values." );

  itk::CommandLineArgumentParser::ReturnValue validateArguments = parser->CheckForRequiredArguments();

  if(validateArguments == itk::CommandLineArgumentParser::FAILED)
  {
    return EXIT_FAILURE;
  }
  else if(validateArguments == itk::CommandLineArgumentParser::HELPREQUESTED)
  {
    return EXIT_SUCCESS;
  }

  /** Get arguments. */
  std::string inputFileName = "";
  parser->GetCommandLineArgument( "-in", inputFileName );

  /** Read as vector of strings, since we don't know yet if it will be
   * integers or floats */
  std::vector< std::string > inValues;
  parser->GetCommandLineArgument( "-i", inValues );
  std::vector< std::string > outValues;
  parser->GetCommandLineArgument( "-o", outValues );

  std::string outputFileName = inputFileName.substr( 0, inputFileName.rfind( "." ) );
  outputFileName += "LUTAPPLIED.mhd";
  parser->GetCommandLineArgument( "-out", outputFileName );

  std::string ComponentType = "";
  bool retpt = parser->GetCommandLineArgument( "-pt", ComponentType );

  /** Check if the required arguments are given. */
  if ( inValues.size() != outValues.size() )
  {
    std::cerr << "ERROR: \"-i\" and \"-o\" should be followed by an equal number of values!" << std::endl;
    return 1;
  }

  /** Determine image properties. */
  std::string ComponentTypeIn = "short";
  std::string PixelType; //we don't use this
  unsigned int Dimension = 3;
  unsigned int NumberOfComponents = 1;
  std::vector<unsigned int> imagesize( Dimension, 0 );
  int retgip = GetImageProperties(
    inputFileName,
    PixelType,
    ComponentTypeIn,
    Dimension,
    NumberOfComponents,
    imagesize );
  if ( retgip != 0 )
  {
    std::cerr << "ERROR: error while getting image properties of the input image!" << std::endl;
    return 1;
  }

  /** The default output is equal to the input, but can be overridden by
   * specifying -pt in the command line.   */
  if ( !retpt ) ComponentType = ComponentTypeIn;

  /** Check for vector images. */
  if ( NumberOfComponents > 1 )
  {
    std::cerr << "ERROR: The NumberOfComponents is larger than 1!" << std::endl;
    std::cerr << "Cannot make vector of vector images." << std::endl;
    return 1;
  }

  /** Get rid of the possible "_" in ComponentType. */
  ReplaceUnderscoreWithSpace( ComponentType );


  /** Run the program. */
  bool supported = false;
  try
  {
    run( IntensityReplaceImageFilter, char, 2 );
    run( IntensityReplaceImageFilter, unsigned char, 2 );
    run( IntensityReplaceImageFilter, short, 2 );
    run( IntensityReplaceImageFilter, unsigned short, 2 );
    run( IntensityReplaceImageFilter, int, 2 );
    run( IntensityReplaceImageFilter, unsigned int, 2 );
    run( IntensityReplaceImageFilter, long, 2 );
    run( IntensityReplaceImageFilter, unsigned long, 2 );
    run( IntensityReplaceImageFilter, float, 2 );
    run( IntensityReplaceImageFilter, double, 2 );

    run( IntensityReplaceImageFilter, char, 3 );
    run( IntensityReplaceImageFilter, unsigned char, 3 );
    run( IntensityReplaceImageFilter, short, 3 );
    run( IntensityReplaceImageFilter, unsigned short, 3 );
    run( IntensityReplaceImageFilter, int, 3 );
    run( IntensityReplaceImageFilter, unsigned int, 3 );
    run( IntensityReplaceImageFilter, long, 3 );
    run( IntensityReplaceImageFilter, unsigned long, 3 );
    run( IntensityReplaceImageFilter, float, 3 );
    run( IntensityReplaceImageFilter, double, 3 );

  }
  catch( itk::ExceptionObject &e )
  {
    std::cerr << "Caught ITK exception: " << e << std::endl;
    return 1;
  }
  if ( !supported )
  {
    std::cerr << "ERROR: this combination of pixeltype and dimension is not supported!" << std::endl;
    std::cerr
      << "pixel (component) type = " << ComponentType
      << " ; dimension = " << Dimension
      << std::endl;
    return 1;
  }

  /** End program. */
  return 0;

} // end main


/*
 * ******************* IntensityReplaceImageFilter *******************
 */

template< class TOutputPixel, unsigned int NDimension >
void IntensityReplaceImageFilter( const std::string & inputFileName,
  const std::string & outputFileName,
  const std::vector<std::string> & inValues,
  const std::vector<std::string> & outValues )
{
  /** Typedefs. */
  typedef TOutputPixel                                    OutputPixelType;
  const unsigned int Dimension = NDimension;

  typedef OutputPixelType                                 InputPixelType;

  typedef itk::Image< InputPixelType, Dimension >         InputImageType;
  typedef itk::Image< OutputPixelType, Dimension >        OutputImageType;

  typedef itk::ImageFileReader< InputImageType >          ReaderType;
  typedef itk::ChangeLabelImageFilter<
    InputImageType, OutputImageType >                     ReplaceFilterType;
  typedef itk::ImageFileWriter< OutputImageType >         WriterType;

  /** Read in the input image. */
  typename ReaderType::Pointer reader = ReaderType::New();
  typename ReplaceFilterType::Pointer replaceFilter = ReplaceFilterType::New();
  typename WriterType::Pointer writer = WriterType::New();

  /** Set up reader */
  reader->SetFileName( inputFileName );

  /** Setup the the input and the 'change map' of the replace filter. */
  replaceFilter->SetInput( reader->GetOutput() );
  if ( itk::NumericTraits<OutputPixelType>::is_integer )
  {
    for (unsigned int i = 0; i < inValues.size(); ++i)
    {
      const InputPixelType inval = static_cast< InputPixelType >(
        atoi( inValues[i].c_str() )   );
      const OutputPixelType outval = static_cast< OutputPixelType >(
        atoi( outValues[i].c_str() )   );
      replaceFilter->SetChange( inval, outval );
    }
  }
  else
  {
    for (unsigned int i = 0; i < inValues.size(); ++i)
    {
      const InputPixelType inval = static_cast< InputPixelType >(
        atof( inValues[i].c_str() )   );
      const OutputPixelType outval = static_cast< OutputPixelType >(
        atof( outValues[i].c_str() )   );
      replaceFilter->SetChange( inval, outval );
    }
  }

  /** Set up writer. */
  writer->SetFileName( outputFileName );
  writer->SetInput( replaceFilter->GetOutput() );
  writer->Update();

} // end IntensityReplaceImageFilter()


/**
 * ******************* GetHelpString *******************
 */
std::string GetHelpString()
{
  std::stringstream ss;
  ss << "This program replaces some user specified intensity values in an image." << std::endl
  << "Usage:" << std::endl
  << "pxintensityreplace" << std::endl
  << "  -in      inputFilename" << std::endl
  << "  [-out]   outputFilename, default in + LUTAPPLIED.mhd" << std::endl
  << "  -i       input pixel values that should be replaced" << std::endl
  << "  -o       output pixel values that replace the corresponding input values" << std::endl
  << "  [-pt]    output pixel type, default equal to input" << std::endl
  << "Supported: 2D, 3D, (unsigned) char, (unsigned) short, (unsigned) int," << std::endl
  << "(unsigned) long, float, double." << std::endl
  << "If \"-pt\" is used, the input is immediately converted to that particular" << std::endl
  << "type, after which the intensity replacement is performed.";

  return ss.str();
} // end GetHelpString()

