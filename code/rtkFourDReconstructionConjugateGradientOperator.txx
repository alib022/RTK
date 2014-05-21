#ifndef __rtkFourDReconstructionConjugateGradientOperator_txx
#define __rtkFourDReconstructionConjugateGradientOperator_txx

#include "rtkFourDReconstructionConjugateGradientOperator.h"

namespace rtk
{

template< typename VolumeSeriesType, typename ProjectionStackType>
FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>::FourDReconstructionConjugateGradientOperator()
{
  this->SetNumberOfRequiredInputs(2);

  // Set default behavior
  m_UseCudaInterpolation = false;
  m_UseCudaSplat = false;

  // Create the filters
  m_ConstantVolumeSource1 = ConstantVolumeSourceType::New();
  m_ConstantVolumeSource2 = ConstantVolumeSourceType::New();
  m_ExtractFilter = ExtractFilterType::New();
  m_ZeroMultiplyVolumeSeriesFilter = MultiplyVolumeSeriesType::New();
  m_ZeroMultiplyProjectionStackFilter = MultiplyProjectionStackType::New();

  // Set permanent connections
  m_ExtractFilter->SetInput(m_ZeroMultiplyProjectionStackFilter->GetOutput());

  // Set constant parameters
  m_ZeroMultiplyVolumeSeriesFilter->SetConstant2(itk::NumericTraits<typename VolumeSeriesType::PixelType>::ZeroValue());
  m_ZeroMultiplyProjectionStackFilter->SetConstant2(itk::NumericTraits<typename ProjectionStackType::PixelType>::ZeroValue());
}

template< typename VolumeSeriesType, typename ProjectionStackType>
void FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>::SetInputVolumeSeries(const VolumeSeriesType* VolumeSeries)
{
  this->SetNthInput(0, const_cast<VolumeSeriesType*>(VolumeSeries));
}

template< typename VolumeSeriesType, typename ProjectionStackType>
void FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>::SetInputProjectionStack(const ProjectionStackType* Projection)
{
  this->SetNthInput(1, const_cast<ProjectionStackType*>(Projection));
}

template< typename VolumeSeriesType, typename ProjectionStackType>
typename VolumeSeriesType::ConstPointer FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>::GetInputVolumeSeries()
{
  return static_cast< const VolumeSeriesType * >
          ( this->itk::ProcessObject::GetInput(0) );
}

template< typename VolumeSeriesType, typename ProjectionStackType>
typename ProjectionStackType::ConstPointer FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>::GetInputProjectionStack()
{
  return static_cast< const ProjectionStackType * >
          ( this->itk::ProcessObject::GetInput(1) );
}

template< typename VolumeSeriesType, typename ProjectionStackType>
void
FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>
::SetBackProjectionFilter (const typename BackProjectionFilterType::Pointer _arg)
{
  m_BackProjectionFilter = _arg;
  this->Modified();
}

template< typename VolumeSeriesType, typename ProjectionStackType>
void
FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>
::SetForwardProjectionFilter (const typename ForwardProjectionFilterType::Pointer _arg)
{
  m_ForwardProjectionFilter = _arg;
  this->Modified();
}

template< typename VolumeSeriesType, typename ProjectionStackType>
void
FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>
::SetGeometry(const ThreeDCircularProjectionGeometry::Pointer _arg)
{
  m_BackProjectionFilter->SetGeometry(_arg.GetPointer());
  m_ForwardProjectionFilter->SetGeometry(_arg.GetPointer());
  this->Modified();
}

template< typename VolumeSeriesType, typename ProjectionStackType>
void
FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>
::InitializeConstantSource()
{
  int Dimension = 3;

  // Configure the constant image source that is connected to input 2 of the m_SingleProjToFourDFilter
  typename VolumeType::SizeType ConstantVolumeSourceSize;
  ConstantVolumeSourceSize.Fill(0);
  for(unsigned int i=0; i < Dimension; i++)
      ConstantVolumeSourceSize[i] = GetInputVolumeSeries()->GetLargestPossibleRegion().GetSize()[i];

  typename VolumeType::SpacingType ConstantVolumeSourceSpacing;
  ConstantVolumeSourceSpacing.Fill(0);
  for(unsigned int i=0; i < Dimension; i++)
      ConstantVolumeSourceSpacing[i] = GetInputVolumeSeries()->GetSpacing()[i];

  typename VolumeType::PointType ConstantVolumeSourceOrigin;
  ConstantVolumeSourceOrigin.Fill(0);
  for(unsigned int i=0; i < Dimension; i++)
      ConstantVolumeSourceOrigin[i] = GetInputVolumeSeries()->GetOrigin()[i];;

  typename VolumeType::DirectionType ConstantVolumeSourceDirection;
  ConstantVolumeSourceDirection.SetIdentity();

  m_ConstantVolumeSource1->SetOrigin( ConstantVolumeSourceOrigin );
  m_ConstantVolumeSource1->SetSpacing( ConstantVolumeSourceSpacing );
  m_ConstantVolumeSource1->SetDirection( ConstantVolumeSourceDirection );
  m_ConstantVolumeSource1->SetSize( ConstantVolumeSourceSize );
  m_ConstantVolumeSource1->SetConstant( 0. );

  m_ConstantVolumeSource2->SetOrigin( ConstantVolumeSourceOrigin );
  m_ConstantVolumeSource2->SetSpacing( ConstantVolumeSourceSpacing );
  m_ConstantVolumeSource2->SetDirection( ConstantVolumeSourceDirection );
  m_ConstantVolumeSource2->SetSize( ConstantVolumeSourceSize );
  m_ConstantVolumeSource2->SetConstant( 0. );
}

template< typename VolumeSeriesType, typename ProjectionStackType>
void
FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>
::GenerateOutputInformation()
{
  // Create the interpolation and splat filters at runtime
  if (m_UseCudaInterpolation) m_InterpolationFilter = rtk::CudaInterpolateImageFilter::New();
    else  m_InterpolationFilter = InterpolationFilterType::New();
  if (m_UseCudaSplat) m_SplatFilter = rtk::CudaSplatImageFilter::New();
    else m_SplatFilter = SplatFilterType::New();

  // Set runtime connections
  m_ZeroMultiplyVolumeSeriesFilter->SetInput1(this->GetInputVolumeSeries());
  m_ZeroMultiplyProjectionStackFilter->SetInput1(this->GetInputProjectionStack());

  m_InterpolationFilter->SetInputVolume(m_ConstantVolumeSource1->GetOutput());
  m_InterpolationFilter->SetInputVolumeSeries(this->GetInputVolumeSeries());

  m_ForwardProjectionFilter->SetInput(0, m_ExtractFilter->GetOutput());
  m_ForwardProjectionFilter->SetInput(1, m_InterpolationFilter->GetOutput());

  m_BackProjectionFilter->SetInput(0, m_ConstantVolumeSource2->GetOutput());
  m_BackProjectionFilter->SetInput(1, m_ForwardProjectionFilter->GetOutput());

  m_SplatFilter->SetInputVolumeSeries(m_ZeroMultiplyVolumeSeriesFilter->GetOutput());
  m_SplatFilter->SetInputVolume(m_BackProjectionFilter->GetOutput());

  // Set runtime parameters
  int Dimension = ProjectionStackType::ImageDimension; // Dimension = 3
  typename ProjectionStackType::RegionType extractRegion;
  extractRegion = this->GetInputProjectionStack()->GetLargestPossibleRegion();
  extractRegion.SetSize(Dimension-1, 1);
  extractRegion.SetIndex(Dimension-1, 0);
  m_ExtractFilter->SetExtractionRegion(extractRegion);

  m_InterpolationFilter->SetWeights(m_Weights);
  m_SplatFilter->SetWeights(m_Weights);
  m_InterpolationFilter->SetProjectionNumber(0);
  m_SplatFilter->SetProjectionNumber(0);

  // Initialize m_ConstantVolumeSource
  this->InitializeConstantSource();

  // Have the last filter calculate its output information
  m_SplatFilter->UpdateOutputInformation();

  // Copy it as the output information of the composite filter
  this->GetOutput()->CopyInformation( m_SplatFilter->GetOutput() );
}

template< typename VolumeSeriesType, typename ProjectionStackType>
void
FourDReconstructionConjugateGradientOperator<VolumeSeriesType, ProjectionStackType>
::GenerateData()
{
  int Dimension = ProjectionStackType::ImageDimension;

  // Set the Extract filter
  typename ProjectionStackType::RegionType extractRegion;
  extractRegion = this->GetInputProjectionStack()->GetLargestPossibleRegion();
  extractRegion.SetSize(Dimension-1, 1);

  int NumberProjs = this->GetInputProjectionStack()->GetLargestPossibleRegion().GetSize(2);
  for(int proj=0; proj<NumberProjs; proj++)
    {

    // After the first update, we need to use the output as input.
    if(proj>0)
      {
      typename VolumeSeriesType::Pointer pimg = m_SplatFilter->GetOutput();
      pimg->DisconnectPipeline();
      m_SplatFilter->SetInputVolumeSeries( pimg );
      }

    // Set the Extract Filter
    extractRegion.SetIndex(Dimension-1, proj);
    m_ExtractFilter->SetExtractionRegion(extractRegion);

    // Set the Interpolation filter
    m_InterpolationFilter->SetProjectionNumber(proj);
    m_SplatFilter->SetProjectionNumber(proj);

    // Update the last filter
    m_SplatFilter->Update();
    }

  // Graft its output
  this->GraftOutput( m_SplatFilter->GetOutput() );
}

}// end namespace


#endif