#ifndef OPENPOSE_EXAMPLES_DOME_W_USER_INPUT_HPP
#define OPENPOSE_EXAMPLES_DOME_W_USER_INPUT_HPP

// OpenPose dependencies
#include <openpose/headers.hpp>
// Other files
#include "frameExtractUtil.cpp"
#include "domeDatum.hpp"

std::pair<bool, std::vector<std::vector<bool>>> openRawFiles(const int numberCameras = 480)
{
    std::vector<std::vector<bool>> bValidCamChecker(20);
    printf("numberCameras: %d\n", numberCameras); // E.g., numberCameras = 140
    // Generate camera sampling info
    // const char* cameraSamplingInfoFolder = "/media/posefs1b/Users/hanbyulj/cameraSamplingInfo";
    const char* cameraSamplingInfoFolder = "/media/posefs1b/Users/hanbyulj/cameraSamplingInfo_furthest";

    char camSamplingInfo[512];
    sprintf(camSamplingInfo,"%s/vga_camNames_%03d.txt",cameraSamplingInfoFolder,numberCameras);
    printf("## CameraSamplingInfoFile: %s\n",camSamplingInfo);
    std::ifstream fin(camSamplingInfo);

    for (auto panelIdx = 0u ; panelIdx < bValidCamChecker.size() ; panelIdx++)
        bValidCamChecker[panelIdx].resize(24,false);
    if (fin.is_open()==false)
    {
        printf("Error: cannot find vga camera sampling info\n");
        return std::make_pair(false, bValidCamChecker);
    }
    else
    {
        int camNum, panelIdx, camIdx;
        fin >> camNum;
        for (auto i = 0 ; i < camNum ; i++)
        {
            fin >> panelIdx >>camIdx;
            bValidCamChecker[panelIdx-1][camIdx-1] = true;
        }
    }
    fin.close();
    return std::make_pair(true, bValidCamChecker);
}

bool extractRawImgs(
    std::vector<cv::Mat>& imgVect, std::vector<int>& camIdxVect, VGAPanelInfo& mVgaPanelInfo, int& frameIdx,
    int& panelIdx, int& firstTC, int& prevPanelIdx, const int frameStartIdx, const int frameEndIdx, const int panelEnd,
    const std::string& rawFileNameString, const std::string& rawDir, const std::string& write_txt,
    const std::vector<std::vector<bool>>& bValidCamChecker)
{
    imgVect.clear();
    camIdxVect.clear();
    for ( ;panelIdx <=panelEnd;++panelIdx)
    {
        if (prevPanelIdx!=panelIdx)
        {
            if (mVgaPanelInfo.m_fp)
            {
                fclose(mVgaPanelInfo.m_fp);
                mVgaPanelInfo.m_fp = NULL;
            }

            char rawFileName[256];
            sprintf(rawFileName, "%s/vga/ve%02d/%s", rawDir.c_str(), panelIdx, rawFileNameString.c_str());
            printf("rawFileName: %s\n",rawFileName);

            mVgaPanelInfo.m_fp = fopen(rawFileName,"r");
            if (mVgaPanelInfo.m_fp==NULL)
            {
                printf("Failed in opening %s\n",rawFileName);
                continue;
            }
            printf("success: open raw file\n");
            prevPanelIdx = panelIdx;
            frameIdx = frameStartIdx;
            firstTC = ConvFrameToTC_vga(mVgaPanelInfo.m_fp,0);
            printf("firstTC: %d\n",firstTC);
        }

        for ( ; frameIdx <= frameEndIdx ; frameIdx++)
        {
            // Open raw files
            camIdxVect.reserve(24);
            for (auto c=1 ; c<=24 ; c++)
            {
                if (bValidCamChecker[panelIdx-1][c-1]==false)
                    continue;
                //if (out_option == PEAK)
                {
                    char filename[512];
                    sprintf(
                        filename, "%s/%03dXX/%08d/%08d_%02d_%02d.txt",
                        write_txt.c_str(), int(frameIdx/100), frameIdx, frameIdx, panelIdx, c);
                    // Check file existence
                    if (op::existFile(filename))
                    {
                        if (frameIdx%100==0 && c==1)
                            printf("# File already exists: %s\n",filename);
                        continue;
                    }
                }
                camIdxVect.push_back(c);
            }
            if (camIdxVect.size()==0)
                continue;

            //extract frames
            const bool bValid = imageExtraction_vga_withDropChecker(
                mVgaPanelInfo, frameIdx, firstTC, false, camIdxVect, imgVect);
            if (bValid ==false)
            {
                printf("Frame droped: frame %d, panel %d\n", frameIdx, panelIdx);
                continue;
            }

            // Update frameIdx for next loading
            frameIdx++;
            return true;
        }
    }
    if (mVgaPanelInfo.m_fp)
    {
        fclose(mVgaPanelInfo.m_fp);
        mVgaPanelInfo.m_fp = NULL;
    }
    // If no more available raw video
    return false;
}

// This worker will just read and return all the basic image file formats in a directory
class WUserInput : public op::WorkerProducer<std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>>
{
public:
    WUserInput(
        const int numberCameras, const std::string rawDirectory, const int frameFirst,
        const int frameLast, const int panelStart, const int panelEnd, const std::string writeTxt) :
        mFrameFirst{frameFirst},
        mFrameLast{frameLast},
        mPanelEnd{panelEnd},
        mRawDirectory{rawDirectory},
        mWriteTxt{writeTxt},
        mPanelCurrent{panelStart},
        mFrameCurrent{mFrameFirst},
        mPrevPanelIdx{-1},
        mImgIdx{std::numeric_limits<unsigned int>::max()}
    {
        try
        {
            printf("numberCameras: %d\n",numberCameras);

            bool validCamChecker;
            std::tie(validCamChecker, mBValidCamChecker) = openRawFiles(numberCameras);
            if (!validCamChecker)
            {
                printf("!validCamChecker");
                // global.quit_threads = true;
                // printf("getFrameFromCam finished earlier.\n");
                op::error("getFrameFromCam finished earlier.\n", __LINE__, __FUNCTION__, __FILE__);
                // return nullptr;
            }

            if (!mRawDirectory.empty())
            {
                // boost::filesystem::path pp(rawDirectory);
                // mRawFileName = pp.stem().string();
                mRawFileName = op::getFileNameNoExtension(mRawDirectory);
                // E.g., rawDirectory = /media/posefs12a/Captures/specialEvents/190125_pose2
                printf("rawDirectory: %s\n", mRawDirectory.c_str());
                // E.g., mRawFileName = 190125_pose2
                printf("mRawFileName: %s\n", mRawFileName.c_str());
            }
        }
        catch (const std::exception& e)
        {
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }

    void initializationOnThread() {}

    std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>> workProducer()
    {
        try
        {
            // Read more images if previous batch finished
            if (mImgIdx >= mImgVect.size())
            {
                if (extractRawImgs(
                        mImgVect, mCamIdxVect, mVgaPanelInfo, mFrameCurrent, mPanelCurrent, mFirstTC, mPrevPanelIdx,
                        mFrameFirst, mFrameLast, mPanelEnd, mRawFileName, mRawDirectory, mWriteTxt, mBValidCamChecker)
                    ==false)
                {
                    printf("No more images, closing software...\n");
                    this->stop();
                    return nullptr;
                }
                mImgIdx = 0u;
            }

            printf("check: %d vs %d\n", (int)mImgVect.size(), (int)mCamIdxVect.size());
            printf("mImgIdx: %d\n", mImgIdx);

            // // Copy to new Datum
            // cv::Mat image_uchar = mImgVect[mImgIdx];
            // Frame f;
            // if (!image_uchar.empty())
            // {
            //     f.index = mGlobalCounter++;
            //     f.video_frame_number = global.uistate.current_frame;
            //     f.panelIdx = mPanelCurrent;

            //     f.camIdx = mCamIdxVect[mImgIdx];
            //     // f.camIdx = mImgIdx+1;
            // }
            // mImgIdx++;

            // Create new datum
            auto datumsPtr = std::make_shared<std::vector<std::shared_ptr<DomeDatum>>>();
            datumsPtr->emplace_back();
            auto& datumPtr = datumsPtr->at(0);
            datumPtr = std::make_shared<DomeDatum>();

            // Fill datum
            datumPtr->cvInputData = mImgVect[mImgIdx];
            // datumPtr->index = mGlobalCounter++;
            datumPtr->frameNumber = mFrameCurrent-1; // datumPtr->video_frame_number = mFrameCurrent-1;
            datumPtr->panelIdx = mPanelCurrent;
            datumPtr->camIdx = mCamIdxVect[mImgIdx];
            mImgIdx++;

            // If empty frame -> return nullptr
            if (datumPtr->cvInputData.empty())
            {
                op::log("Empty frame detected on path: " + mImageFiles.at(mCounter-1) + ". Closing program.",
                    op::Priority::High);
                this->stop();
                datumsPtr = nullptr;
            }

            return datumsPtr;

            // // ORIGINAL DEMO CODE
            // // Close program when empty frame
            // if (mImageFiles.size() <= mCounter)
            // {
            //     op::log("Last frame read and added to queue. Closing program after it is processed.",
            //             op::Priority::High);
            //     // This funtion stops this worker, which will eventually stop the whole thread system once all the
            //     // frames have been processed
            //     this->stop();
            //     return nullptr;
            // }
            // else
            // {
            //     // Create new datum
            //     auto datumsPtr = std::make_shared<std::vector<std::shared_ptr<DomeDatum>>>();
            //     datumsPtr->emplace_back();
            //     auto& datumPtr = datumsPtr->at(0);
            //     datumPtr = std::make_shared<DomeDatum>();

            //     // Fill datum
            //     datumPtr->cvInputData = cv::imread(mImageFiles.at(mCounter++));

            //     // If empty frame -> return nullptr
            //     if (datumPtr->cvInputData.empty())
            //     {
            //         op::log("Empty frame detected on path: " + mImageFiles.at(mCounter-1) + ". Closing program.",
            //             op::Priority::High);
            //         this->stop();
            //         datumsPtr = nullptr;
            //     }

            //     return datumsPtr;
            // }
        }
        catch (const std::exception& e)
        {
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return nullptr;
        }
    }

private:
    const int mFrameFirst;
    const int mFrameLast;
    const int mPanelEnd;
    const std::string mRawDirectory;
    const std::string mWriteTxt;
    int mPanelCurrent;
    int mFrameCurrent;
    int mPrevPanelIdx;
    unsigned int mImgIdx;
    std::vector<cv::Mat> mImgVect;
    std::vector<int> mCamIdxVect;
    int mFirstTC;
    VGAPanelInfo mVgaPanelInfo;
    std::string mRawFileName;
    std::vector<std::vector<bool>> mBValidCamChecker;

    // Old
    const std::vector<std::string> mImageFiles;
    unsigned long long mCounter;
};

#endif // OPENPOSE_EXAMPLES_DOME_W_USER_INPUT_HPP
