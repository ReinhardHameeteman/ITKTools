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
 \brief This program inverts the intensities of an image.  new = max - old (where max is the maximum of the image).
 
 \verbinclude invertintensityimagefilter.help
 */
#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkStatisticsImageFilter.h"
#include "itkInvertIntensityImageFilter.h"

// Vector image support
#include "itkVectorIndexSelectionCastImageFilter.h" // decompose
#include "itkImageToVectorImageFilter.h" // reassemble
#include "itkChannelByChannelVectorImageFilter2.h"
//-------------------------------------------------------------------------------------

/** run: A macro to call a function. */
#define run( function, type, dim ) \
if ( ComponentTypeIn == #type && Dimension == dim ) \
{ \
  typedef itk::Image< type, dim > InputImageType; \
  function< InputImageType >( inputFileName, outputFileName ); \
  supported = true; \
}

//-------------------------------------------------------------------------------------

/* Declare InvertIntensity. */
template< class InputImageType >
void InvertIntensity(
  const std::string & inputFileName,
  const std::string & outputFileName );

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

  std::string outputFileName = inputFileName.substr( 0, inputFileName.rfind( "." ) );
  outputFileName += "INVERTED.mhd";
  parser->GetCommandLineArgument( "-out", outputFileName );

  /** Determine image properties. */
  std::string ComponentTypeIn = "short";
  std::string PixelType; //we don't use this
  unsigned int Dimension = 2;
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
    return 1;
  }

  /** Check for vector images. */
  if ( NumberOfComponents > 1 )
  {
    std::cerr << "ERROR: The NumberOfComponents is larger than 1!" << std::endl;
    std::cerr << "Vector images are not supported." << std::endl;
    return 1;
  }

  /** Get rid of the possible "_" in ComponentType. */
  ReplaceUnderscoreWithSpace( ComponentTypeIn );

  /** Run the program. */
  bool supported = false;
  try
  {
    /** 2D. */
    run( InvertIntensity, char, 2 );
    run( InvertIntensity, unsigned char, 2 );
    run( InvertIntensity, short, 2 );
    run( InvertIntensity, unsigned short, 2 );
    run( InvertIntensity, float, 2 );
    run( InvertIntensity, double, 2 );

    /** 3D. */
    run( InvertIntensity, char, 3 );
    run( InvertIntensity, unsigned char, 3 );
    run( InvertIntensity, short, 3 );
    run( InvertIntensity, unsigned short, 3 );
    run( InvertIntensity, float, 3 );
    run( InvertIntensity, double, 3 );

  } // end run
  catch ( itk::ExceptionObject &e )
  {
    std::cerr << "Caught ITK exception: " << e << std::endl;
    return 1;
  }
  if ( !supported )
  {
    std::cerr << "ERROR: this combination of pixeltype and dimension is not supported!" << std::endl;
    std::cerr
      << "pixel (component) type = " << ComponentTypeIn
      << " ; dimension = " << Dimension
      << std::endl;
    return 1;
  }

  /** End program. */
  return 0;

} // end main


/**
 * ******************* GetHelpString *******************
 */

std::string GetHelpString( void )
{
  std::stringstream ss;
  ss << "This program inverts the intensities of an image." << std::endl
    << "Usage:" << std::endl
    << "pxinvertintensityimagefilter" << std::endl
    << "  -in      inputFilename" << std::endl
    << "  [-out]   outputFilename; default: in + INVERTED.mhd" << std::endl
    << "Supported: 2D, 3D, (unsigned) char, (unsigned) short, float, double.";

  return ss.str();

} // end GetHelpString()


/*
 * ******************* InvertIntensity *******************
 *
 * The resize function templated over the input pixel type.
 */

template< class ScalarImageType >
void InvertIntensity( const std::string & inputFileName,
  const std::string & outputFileName )
{
  /** Some typedef's. */
  typedef typename ScalarImageType::PixelType                InputPixelType;
  typedef itk::VectorImage<InputPixelType, ScalarImageType::ImageDimension> VectorImageType;
  typedef itk::ImageFileReader< VectorImageType >            ReaderType;
  typedef itk::ImageFileWriter< VectorImageType >            WriterType;
  typedef itk::StatisticsImageFilter< ScalarImageType >      StatisticsFilterType;
  typedef typename StatisticsFilterType::RealType           RealType;
  typedef itk::InvertIntensityImageFilter< ScalarImageType > InvertIntensityFilterType;

  /** Create reader. */
  typename ReaderType::Pointer reader = ReaderType::New();
  reader->SetFileName( inputFileName.c_str() );

  // In this case, we must manually disassemble the image rather than use a
  // ChannelByChannel filter because the image is not the output,
  // but rather the GetMaximum() function is what we want.
  
  // Create the disassembler
  typedef itk::VectorIndexSelectionCastImageFilter<VectorImageType, ScalarImageType> IndexSelectionType;
  typename IndexSelectionType::Pointer indexSelectionFilter = IndexSelectionType::New();
  indexSelectionFilter->SetInput(reader->GetOutput());

  double max = std::numeric_limits<double>::min(); // Initialize so that any number will be bigger than this one

  // Get the max of each channel, keeping the largest
  for(unsigned int channel = 0; channel < reader->GetOutput()->GetNumberOfComponentsPerPixel(); channel++)
    {
    // Extract the current channel
    indexSelectionFilter->SetIndex(channel);
    indexSelectionFilter->Update();

    /** Create statistics filter. */
    typename StatisticsFilterType::Pointer statistics = StatisticsFilterType::New();
    statistics->SetInput( indexSelectionFilter->GetOutput() );
    statistics->Update();
    if(statistics->GetMaximum() > max)
      {
      max = statistics->GetMaximum();
      }
    }

  /** Create invert filter. */
  typename InvertIntensityFilterType::Pointer invertFilter = InvertIntensityFilterType::New();
  invertFilter->SetMaximum( max );

  // Setup the filter to apply the invert filter to every channel
  typedef itk::ChannelByChannelVectorImageFilter2<VectorImageType, InvertIntensityFilterType> ChannelByChannelInvertType;
  typename ChannelByChannelInvertType::Pointer channelByChannelInvertFilter = ChannelByChannelInvertType::New();
  channelByChannelInvertFilter->SetInput(reader->GetOutput());
  channelByChannelInvertFilter->SetFilter(invertFilter);
  channelByChannelInvertFilter->Update();

  /** Create writer. */
  typename WriterType::Pointer writer = WriterType::New();
  writer->SetFileName( outputFileName.c_str() );
  writer->SetInput( channelByChannelInvertFilter->GetOutput() );
  writer->Update();

} // end InvertIntensity()
