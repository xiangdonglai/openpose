#ifndef OPENPOSE_EXAMPLES_DOME_W_USER_OUTPUT_HPP
#define OPENPOSE_EXAMPLES_DOME_W_USER_OUTPUT_HPP

#include <fstream>
// OpenPose dependencies
#include <openpose/headers.hpp>
// Other files
// #include "frameExtractUtil.cpp"
#include "domeDatum.hpp"

// This worker will just read and return all the basic image file formats in a directory
class WUserOutput : public op::WorkerConsumer<std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>>
{
public:
    WUserOutput(const op::PoseModel poseModel, const std::string writeTxt) :
        mNumberBodyParts{op::getPoseNumberBodyParts(poseModel)},
        mWriteTxt{writeTxt}
    {
        try
        {
            if (!mWriteTxt.empty())
            {
                char save_subFolder[512];
                sprintf(save_subFolder, "%s", mWriteTxt.c_str());
                mkdir(save_subFolder, 755);
            }
        }
        catch (const std::exception& e)
        {
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    void initializationOnThread() {}

    void workConsumer(const std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>& datumsPtr)
    {
        try
        {
            if (datumsPtr != nullptr && !datumsPtr->empty())
            {
                // Sanity check
                if (datumsPtr->size() != 1)
                    op::error("Problem! datumsPtr->size() != 1!!!", __LINE__, __FUNCTION__, __FILE__);
                // Saving Dome output
                const auto& datumPtr = datumsPtr->at(0);
                if (!mWriteTxt.empty())
                {
                    // Create directories
                    char save_subFolder[512];
                    sprintf(save_subFolder, "%s/%03dXX", mWriteTxt.c_str(), int(datumPtr->frameNumber/100));
                    // printf("save_subFolder: %s\n",save_subFolder);
                    mkdir(save_subFolder, 755);
                    sprintf(
                        save_subFolder, "%s/%03dXX/%08d",
                        mWriteTxt.c_str(), int(datumPtr->frameNumber/100), (int)datumPtr->frameNumber);
                    // printf("save_subFolder: %s\n",save_subFolder);
                    mkdir(save_subFolder, 755);

                    // Get final file name
                    char filename[256];
                    sprintf(
                        filename, "%s/%08d_%02d_%02d.txt",
                        save_subFolder, (int)datumPtr->frameNumber, datumPtr->panelIdx, datumPtr->camIdx);

                    // Save final file
                    const auto numberPeople = datumPtr->poseKeypoints.getSize(0);
                    const auto poseKeypointsPtr = datumPtr->poseKeypoints.getConstPtr();
                    const auto volume = datumPtr->poseKeypoints.getVolume();
                    const auto saveInBinary = true;
                    // Save in binary format so full float-point quality is kept (hard to visually debug)
                    if (saveInBinary)
                    {
                        std::ofstream fout(filename, std::ios::binary);
                        fout.write((char*)&numberPeople, sizeof(int));
                        fout.write((char*)poseKeypointsPtr, sizeof(float) * volume);
                        fout.close();
                    }
                    // Debugging - Save in ASCII format so it can be human readable
                    else
                    {
                        std::ofstream fout(filename);
                        fout << "numberPeople: " << numberPeople << " ";
                        for (auto i = 0 ; i < volume ; i++)
                            fout << poseKeypointsPtr[i] << " ";
                        fout.close();
                    }
                }
            }
        }
        catch (const std::exception& e)
        {
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

private:
    const unsigned int mNumberBodyParts;
    const std::string mWriteTxt;
};

#endif // OPENPOSE_EXAMPLES_DOME_W_USER_OUTPUT_HPP
