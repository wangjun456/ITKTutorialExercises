//
//  Excercise on the use of the GeodesicActiveContour filter
//

#include "itkSmoothingRecursiveGaussianImageFilter.h"
#include "itkIntensityWindowingImageFilter.h"

#include "itkGradientMagnitudeRecursiveGaussianImageFilter.h"
#include "itkSigmoidImageFilter.h"

#include "itkGeodesicActiveContourLevelSetImageFilter.h"
#include "itkBinaryThresholdImageFilter.h"
#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"
#include "itkImage.h"


int main( int argc, char *argv[] )
{


  if( argc < 10 )
    {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " inputImage inputBinaryMask outputImage";
    std::cerr << " minimumBasin minimumBorder" << std::endl;
    std::cerr << " propagationScaling curvatureScaling advectionScaling maximumIterations" << std::endl;
    return 1;
    }

  typedef float           InternalPixelType;
  typedef unsigned char   OutputPixelType;
  const   unsigned int    Dimension = 2;
  
  typedef itk::Image< InternalPixelType, Dimension >  InternalImageType;
  typedef itk::Image< OutputPixelType,   Dimension >  OutputImageType;

  typedef  itk::ImageFileReader< InternalImageType > ReaderType;
  typedef  itk::ImageFileWriter<  OutputImageType  > WriterType;


  ReaderType::Pointer reader1 = ReaderType::New();
  ReaderType::Pointer reader2 = ReaderType::New();
  WriterType::Pointer writer  = WriterType::New();

  reader1->SetFileName( argv[1] );
  reader2->SetFileName( argv[2] );
  writer->SetFileName(  argv[3] );


  //
  // Prepare the initial level set image
  //
  typedef   itk::SmoothingRecursiveGaussianImageFilter< 
                               InternalImageType, 
                               InternalImageType >  SmoothingFilterType;

  SmoothingFilterType::Pointer smoothing = SmoothingFilterType::New();



  typedef   itk::IntensityWindowingImageFilter< 
                               InternalImageType, 
                               InternalImageType >  RescaleFilterType;

  RescaleFilterType::Pointer rescaler = RescaleFilterType::New();



  typedef  itk::GeodesicActiveContourLevelSetImageFilter< 
                                        InternalImageType, 
                                        InternalImageType 
                                                  >  GeodesicActiveContourFilterType;


  GeodesicActiveContourFilterType::Pointer  geodesicActiveContourFilter = GeodesicActiveContourFilterType::New();


  geodesicActiveContourFilter->SetMaximumRMSError( 0.001 );
  geodesicActiveContourFilter->SetNumberOfIterations(atoi(argv[9]));

  smoothing->SetInput( reader2->GetOutput() );

  smoothing->SetNormalizeAcrossScale( true );
  smoothing->SetSigma( 5.0 );  

  // invert the input level set to have negative values inside
  rescaler->SetWindowMinimum(  0  );
  rescaler->SetWindowMaximum( 255 );

  rescaler->SetOutputMinimum(  5.0 );
  rescaler->SetOutputMaximum( -5.0 );  

  rescaler->SetInput( smoothing->GetOutput() );


  geodesicActiveContourFilter->SetInput( rescaler->GetOutput() );
  
  geodesicActiveContourFilter->SetPropagationScaling( atof( argv[ 6 ] ) );
  geodesicActiveContourFilter->SetCurvatureScaling(   atof( argv[ 7 ] ) );
  geodesicActiveContourFilter->SetAdvectionScaling(   atof( argv[ 8 ] ) );

  //
  // Prepare the speed image
  //
  typedef   itk::GradientMagnitudeRecursiveGaussianImageFilter< 
                               InternalImageType, 
                               InternalImageType >  GradientFilterType;

  GradientFilterType::Pointer  gradientMagnitude = GradientFilterType::New();

  gradientMagnitude->SetSigma( 1.0 );

  gradientMagnitude->SetInput( reader1->GetOutput() );


  typedef   itk::SigmoidImageFilter<                               
                               InternalImageType, 
                               InternalImageType >  SigmoidFilterType;


  SigmoidFilterType::Pointer sigmoid = SigmoidFilterType::New();

  

  const double basinMinimum  = atof( argv[4] );
  const double borderMinimum = atof( argv[5] );

  const double alpha =  - ( borderMinimum - basinMinimum ) / 6.0; 
  const double beta  =    ( borderMinimum + basinMinimum ) / 2.0;

  sigmoid->SetOutputMinimum(  0.0  );
  sigmoid->SetOutputMaximum(  1.0  );

  sigmoid->SetAlpha( alpha );
  sigmoid->SetBeta(  beta  );

  sigmoid->SetInput( gradientMagnitude->GetOutput() ); 



  //
  //  Connect inputs to the Shape Detection filter
  //
  geodesicActiveContourFilter->SetInput( rescaler->GetOutput() );

  geodesicActiveContourFilter->SetFeatureImage( sigmoid->GetOutput() );




  //
  //  Threshold
  //
  typedef itk::BinaryThresholdImageFilter< 
                        InternalImageType, 
                        OutputImageType    >    ThresholdingFilterType;
  
  ThresholdingFilterType::Pointer thresholder = ThresholdingFilterType::New();
                        

  thresholder->SetLowerThreshold( -1e7  );  
  thresholder->SetUpperThreshold(  0.0  ); // cut at the zero set

  thresholder->SetOutsideValue(  0  );
  thresholder->SetInsideValue(  255 );


  thresholder->SetInput( geodesicActiveContourFilter->GetOutput() );

  writer->SetInput( thresholder->GetOutput() );


  try
    {
    writer->Update();
    }
  catch( itk::ExceptionObject & excep )
    {
    std::cerr << "Exception caught !" << std::endl;
    std::cerr << excep << std::endl;
    }


  std::cout << "Elapsed Iterations = " << geodesicActiveContourFilter->GetElapsedIterations() << std::endl;
  std::cout << "RMS last change    = " << geodesicActiveContourFilter->GetRMSChange() << std::endl;

  return 0;

}




