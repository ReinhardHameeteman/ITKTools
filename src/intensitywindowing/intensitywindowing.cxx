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
 \brief Intensity windowing.
 
 \verbinclude intensitywindowing.help
 */

#include "itkCommandLineArgumentParser.h"
#include "CommandLineArgumentHelper.h"
#include "itkImage.h"
#include "itkIntensityWindowingImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

//-------------------------------------------------------------------------------------

/** run: A macro to call a function. */
#define run(function,type,dim) \
if ( ComponentType == #type && Dimension == dim ) \
{ \
  typedef itk::Image< type, dim > InputImageType; \
  function< InputImageType >( inputFileName, outputFileName, window ); \
  supported = true; \
}

//-------------------------------------------------------------------------------------

/* Declare IntensityWindowing. */
template< class InputImageType >
void IntensityWindowing(
  const std::string & inputFileName,
  const std::string & outputFileName,
  const std::vector<double> & window );

/** Declare GetHelpString. */
std::string GetHelpString( void );

//-------------------------------------------------------------------------------------

int main( int argc, char **argv )
{
  /** Create a command line argument parser. */
  itk::CommandLineArgumentParser::Pointer parser = itk::CommandLineArgumentParser::New();
  parser->SetCommandLineArguments( argc, argv );
  parser->SetProgramHelpText(GetHelpString());

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

  /** Get input file name. */
  std::string inputFileName = "";
  parser->GetCommandLineArgument( "-in", inputFileName );

  /** Determine input image properties. */
  std::string ComponentType = "short";
  std::string PixelType; //we don't use this
  unsigned int Dimension = 3;
  unsigned int NumberOfComponents = 1;
  std::vector<unsigned int> imagesize( Dimension, 0 );
  int retgip = GetImageProperties(
    inputFileName,
    PixelType,
    ComponentType,
    Dimension,
    NumberOfComponents,
    imagesize );
  if ( retgip != 0 )
  {
    return 1;
  }

  /** Let the user overrule this. */
  parser->GetCommandLineArgument( "-pt", ComponentType );

  /** Error checking. */
  if ( NumberOfComponents > 1 )
  {
    std::cerr << "ERROR: The NumberOfComponents is larger than 1!" << std::endl;
    std::cerr << "Vector images are not supported!" << std::endl;
    return 1;
  }

  /** Get the output file name. */
  std::string outputFileName = inputFileName.substr( 0, inputFileName.rfind( "." ) );
  outputFileName += "WINDOWED.mhd";
  parser->GetCommandLineArgument( "-out", outputFileName );

  /** Get the window. */
  std::vector<double> window;
  bool retw = parser->GetCommandLineArgument( "-w", window );

  //unsigned int Dimension = 3;
  //bool retdim = parser->GetCommandLineArgument( "-dim", Dimension );

  /** Check if the required arguments are given. */
  if ( !retw )
  {
    std::cerr << "ERROR: You should specify \"-w\"." << std::endl;
    return 1;
  }

  /** Check window. */
  if( window.size() != 2 )
  {
    std::cout << "ERROR: The window should consist of two numbers." << std::endl;
    return 1;
  }
  if ( window[ 1 ] < window[ 0 ] )
  {
    double temp = window[ 0 ];
    window[ 0 ] = window[ 1 ];
    window[ 1 ] = temp;
  }
  if ( window[ 0 ] == window[ 1 ] )
  {
    std::cerr << "ERROR: The window should be larger." << std::endl;
    return 1;
  }

  /** Get rid of the possible "_" in ComponentType. */
  ReplaceUnderscoreWithSpace( ComponentType );

  /** Run the program. */
  bool supported = false;
  try
  {
    run( IntensityWindowing, unsigned char, 2 );
    run( IntensityWindowing, unsigned char, 3 );
    run( IntensityWindowing, char, 2 );
    run( IntensityWindowing, char, 3 );
    run( IntensityWindowing, unsigned short, 2 );
    run( IntensityWindowing, unsigned short, 3 );
    run( IntensityWindowing, short, 2 );
    run( IntensityWindowing, short, 3 );
    run( IntensityWindowing, float, 2 );
    run( IntensityWindowing, float, 3 );
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
   * ******************* IntensityWindowing *******************
   */

template< class InputImageType >
void IntensityWindowing(
  const std::string & inputFileName,
  const std::string & outputFileName,
  const std::vector<double> & window )
{
  /** Typedefs. */
  typedef itk::IntensityWindowingImageFilter<
    InputImageType, InputImageType >                  WindowingType;
  typedef itk::ImageFileReader< InputImageType >      ReaderType;
  typedef itk::ImageFileWriter< InputImageType >      WriterType;
  typedef typename InputImageType::PixelType          InputPixelType;

  /** Declarations. */
  typename WindowingType::Pointer windowfilter = WindowingType::New();
  typename ReaderType::Pointer reader = ReaderType::New();
  typename WriterType::Pointer writer = WriterType::New();

  /** Setup the pipeline. */
  reader->SetFileName( inputFileName.c_str() );
  writer->SetFileName( outputFileName.c_str() );
  InputPixelType min = static_cast<InputPixelType>( window[ 0 ] );
  InputPixelType max = static_cast<InputPixelType>( window[ 1 ] );
  windowfilter->SetWindowMinimum( min );
  windowfilter->SetWindowMaximum( max );
  windowfilter->SetOutputMinimum( min );
  windowfilter->SetOutputMaximum( max );

  /** Connect and execute the pipeline. */
  windowfilter->SetInput( reader->GetOutput() );
  writer->SetInput( windowfilter->GetOutput() );
  writer->Update();

} // end IntensityWindowing


/**
 * ******************* GetHelpString *******************
 */
std::string GetHelpString()
{
  std::stringstream ss;
  ss << "Usage:" << std::endl
  << "pxintensitywindowing" << std::endl
  << "  -in      inputFilename" << std::endl
  << "  [-out]   outputFilename, default in + WINDOWED.mhd" << std::endl
  << "  -w       windowMinimum windowMaximum" << std::endl
  << "  [-pt]    pixel type of input and output images;" << std::endl
  << "           default: automatically determined from the first input image." << std::endl
  << "Supported: 2D, 3D, (unsigned) short, (unsigned) char, float.";

  return ss.str();

} // end GetHelpString

