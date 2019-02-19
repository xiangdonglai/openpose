#include <limits> // std::numeric_limits
#include <openpose/gpu/cuda.hpp>
#include <openpose/pose/poseParameters.hpp>
#include <openpose/utilities/check.hpp>
#include <openpose/utilities/fastMath.hpp>
#include <openpose/utilities/keypoint.hpp>
#include <openpose/utilities/openCv.hpp>
#include <openpose/utilities/standard.hpp>
#include <openpose/pose/poseExtractorCaffeStaf.hpp>

namespace op
{
    PoseExtractorCaffeStaf::PoseExtractorCaffeStaf(
        const PoseModel poseModel, const std::string& modelFolder, const int gpuId,
        const std::vector<HeatMapType>& heatMapTypes, const ScaleMode heatMapScaleMode, const bool addPartCandidates,
        const bool maximizePositives, const std::string& protoTxtPath, const std::string& caffeModelPath,
        const float upsamplingRatio, const bool enableNet, const bool enableGoogleLogging) :
        PoseExtractorCaffe{poseModel, modelFolder, gpuId, heatMapTypes, heatMapScaleMode, addPartCandidates,
        maximizePositives, protoTxtPath, caffeModelPath, upsamplingRatio, enableNet, enableGoogleLogging}
    {
log("RUNNING PoseExtractorCaffeStaf::PoseExtractorCaffeStaf");
    }

    PoseExtractorCaffeStaf::~PoseExtractorCaffeStaf()
    {
    }

    void PoseExtractorCaffeStaf::netInitializationOnThread()
    {
        try
        {
            PoseExtractorCaffe::netInitializationOnThread();
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    void PoseExtractorCaffeStaf::forwardPass(
        const std::vector<Array<float>>& inputNetData, const Point<int>& inputDataSize,
        const std::vector<double>& scaleInputToNetInputs, const Array<float>& poseNetOutput)
    {
        try
        {
            PoseExtractorCaffe::forwardPass(
                inputNetData, inputDataSize, scaleInputToNetInputs, poseNetOutput);
        }
        catch (const std::exception& e)
        {
            error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
}
