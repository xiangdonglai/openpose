#ifndef OPENPOSE_EXAMPLES_DOME_W_USER_INPUT_HPP
#define OPENPOSE_EXAMPLES_DOME_W_USER_INPUT_HPP

// OpenPose dependencies
#include <openpose/headers.hpp>
// Other files
#include <openpose/utilities/frameExtractUtil.hpp>
#include <openpose/utilities/syncManager.hpp>
#include "domeDatum.hpp"


std::pair<bool, std::vector<std::vector<bool>>> openRawFilesVga(
    const std::string& cameraSamplingInfoFolder, const int numberCameras = 480)
{
    std::vector<std::vector<bool>> bValidCamChecker(20);
    // // Generate camera sampling info
    // // const char* cameraSamplingInfoFolder = "/media/posefs1b/Users/hanbyulj/cameraSamplingInfo";
    // const char* cameraSamplingInfoFolder = "/media/posefs1b/Users/hanbyulj/cameraSamplingInfo_furthest";

    char camSamplingInfo[512];
    sprintf(camSamplingInfo, "%s/vga_camNames_%03d.txt", cameraSamplingInfoFolder.c_str(), numberCameras);
    printf("## CameraSamplingInfoFile: %s\n", camSamplingInfo);
    std::ifstream fin(camSamplingInfo);

    for (auto panelIdx = 0u ; panelIdx < bValidCamChecker.size() ; panelIdx++)
        bValidCamChecker[panelIdx].resize(24,false);
    if (fin.is_open()==false)
    {
        op::error("Cannot find VGA camera sampling info in the following folder (does it exist? Can you ls it from"
                  " the terminal?):\n" + cameraSamplingInfoFolder, __LINE__, __FUNCTION__, __FILE__);
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

bool extractRawImgsVga(
    std::vector<cv::Mat>& imgVect, std::vector<int>& camIdxVect, VGAPanelInfo& mVgaPanelInfo, int& frameIdx,
    int& panelIdx, int& firstTC, int& prevPanelIdx, const int frameStartIdx, const int frameEndIdx, const int panelEnd,
    const std::string& rawFileNameString, const std::string& rawDir, const std::string& write_txt,
    const std::vector<std::vector<bool>>& bValidCamChecker, const bool printVerbose)
{
    imgVect.clear();
    camIdxVect.clear();
    for ( ; panelIdx <= panelEnd ; panelIdx++)
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
            if (printVerbose)
                printf("rawFileName: %s\n", rawFileName);

            mVgaPanelInfo.m_fp = fopen(rawFileName,"r");
            if (mVgaPanelInfo.m_fp==NULL)
            {
                printf("Failed in opening %s\n",rawFileName);
                continue;
            }
            if (printVerbose)
                printf("Success: open raw file\n");
            prevPanelIdx = panelIdx;
            frameIdx = frameStartIdx;
            firstTC = ConvFrameToTC_vga(mVgaPanelInfo.m_fp,0);
            if (printVerbose)
                printf("FirstTC: %d\n",firstTC);
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
class WUserInputVga : public op::WorkerProducer<std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>>
{
public:
    WUserInputVga(
        const int numberCameras, const std::string& rawDirectory, const int frameFirst,
        const int frameLast, const int panelStart, const int panelEnd, const std::string& cameraSamplingInfoFolder,
        const std::string& writeTxt, const bool printVerbose) :
        mPrintVerbose{printVerbose},
        mFrameFirst{frameFirst},
        mFrameLast{frameLast},
        mPanelEnd{panelEnd},
        mRawDirectory{rawDirectory},
        mCameraSamplingInfoFolder{cameraSamplingInfoFolder},
        mWriteTxt{writeTxt},
        mPanelCurrent{panelStart},
        mFrameCurrent{mFrameFirst},
        mPrevPanelIdx{-1},
        mImgIdx{std::numeric_limits<unsigned int>::max()}
    {
        try
        {
            printf("numberCameras: %d\n",numberCameras); // E.g., numberCameras = 140

            bool validCamChecker;
            std::tie(validCamChecker, mBValidCamChecker) = openRawFilesVga(mCameraSamplingInfoFolder, numberCameras);
            if (!validCamChecker)
            {
                printf("!validCamChecker");
                op::error("getFrameFromCam finished earlier.\n", __LINE__, __FUNCTION__, __FILE__);
            }

            if (!mRawDirectory.empty())
            {
                // boost::filesystem::path pp(mRawDirectory);
                // mRawFileName = pp.stem().string();
                mRawFileName = op::getFileNameNoExtension(mRawDirectory);
                // E.g., mRawDirectory = /media/posefs12a/Captures/specialEvents/190125_pose2
                printf("mRawDirectory: %s\n", mRawDirectory.c_str());
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
                if (extractRawImgsVga(
                        mImgVect, mCamIdxVect, mVgaPanelInfo, mFrameCurrent, mPanelCurrent, mFirstTC, mPrevPanelIdx,
                        mFrameFirst, mFrameLast, mPanelEnd, mRawFileName, mRawDirectory, mWriteTxt, mBValidCamChecker,
                        mPrintVerbose)
                    ==false)
                {
                    op::log("No more images, closing software...", op::Priority::High);
                    this->stop();
                    return nullptr;
                }
                mImgIdx = 0u;
            }

            if (mImgVect.size() != mCamIdxVect.size())
            {
                op::error("Check failed (mImgVect.size() == mCamIdxVect.size()): " + std::to_string(mImgVect.size())
                          + " vs " + std::to_string(mCamIdxVect.size()), __LINE__, __FUNCTION__, __FILE__);
            }
            if (mPrintVerbose)
                printf("mImgIdx: %d\n", mImgIdx);

            // Create new datum
            auto datumsPtr = std::make_shared<std::vector<std::shared_ptr<DomeDatum>>>();
            datumsPtr->emplace_back();
            auto& datumPtr = datumsPtr->at(0);
            datumPtr = std::make_shared<DomeDatum>();

            // Fill datum
            datumPtr->cvInputData = mImgVect[mImgIdx];
            datumPtr->frameNumber = mFrameCurrent-1;
            datumPtr->panelIdx = mPanelCurrent;
            datumPtr->camIdx = mCamIdxVect[mImgIdx];
            mImgIdx++;

            // If empty frame -> return nullptr
            if (datumPtr->cvInputData.empty())
            {
                op::log("Empty frame detected. Ignoring frame.", op::Priority::High);
                datumsPtr = nullptr;
            }

            return datumsPtr;
        }
        catch (const std::exception& e)
        {
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return nullptr;
        }
    }

private:
    const bool mPrintVerbose;
    const int mFrameFirst;
    const int mFrameLast;
    const int mPanelEnd;
    const std::string mRawDirectory;
    const std::string mCameraSamplingInfoFolder;
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
};

bool extractRawImgsHd(
    std::vector<cv::Mat>& imgVect, std::vector<int>& camIdxVect,
    CameraInfo& cameraInfo, CSyncManager& syncManager, SUnivSyncUnitForEachSensor*& sUnivSyncUnitForEachSensorPtr,
    int& frameCurrent, int& hdMachineIdx, int& hdDiskIdx, int& prevHdMachineIdx, int& prevPanelIdx,
    const std::string& rawDirectory, const std::string& writeTxt, const std::string& rawFileNameString,
    const int cameraIndexStart, const int cameraIndexEnd, const int hdMachineEnd, const int frameFirst,
    const int frameLast, const bool printVerbose)
{
    camIdxVect.clear();
    imgVect.clear();
    // hdDiskIdx = 1;
    for ( ; hdMachineIdx <= hdMachineEnd ; hdMachineIdx++)
    {
        if (hdMachineIdx !=prevHdMachineIdx)
            hdDiskIdx = 1;
        for ( ; hdDiskIdx <=2;++hdDiskIdx )
        {
            int hdCamIdx = 2*(hdMachineIdx-31) + hdDiskIdx -1;
            if (hdCamIdx < cameraIndexStart)
                continue;

            if (hdCamIdx > cameraIndexEnd)
                return false;

            // Load New File and Table
            if (prevPanelIdx != hdCamIdx)
            {
                if (cameraInfo.m_fp)
                {
                    fclose(cameraInfo.m_fp);
                    cameraInfo.m_fp = NULL;
                }

                char rawFileName[256];
                sprintf(rawFileName, "%s/hd/ve%02d-%d/%s.hdraw",
                    rawDirectory.c_str(), hdMachineIdx, hdDiskIdx, rawFileNameString.c_str());
                if (printVerbose)
                    printf("rawFileName: %s\n", rawFileName);
            
                cameraInfo.m_fp = fopen(rawFileName,"r");
                if(cameraInfo.m_fp == NULL)
                {   
                    printf("Failed in opening %s\n",rawFileName);
                    continue;
                }
                if (printVerbose)
                    printf("success: open raw file\n");
                prevPanelIdx = hdCamIdx;
                prevHdMachineIdx = hdMachineIdx;
                frameCurrent = frameFirst;


                if (printVerbose)
                    printf("Trying to load syncdata\n");
                char syncFileFolder[521];
                sprintf(syncFileFolder, "%s/sync/syncTables", rawDirectory.c_str());

                CSyncManager::LoadMode loadModeOption = CSyncManager::HDSingleRaw_30;
                CSyncManager syncMan;
                syncMan.loadSelectedTables(syncFileFolder, loadModeOption, hdMachineIdx, hdDiskIdx);
                // Just copy should be fine..all the memeber variables are vectors.
                syncManager = syncMan;

                sUnivSyncUnitForEachSensorPtr = syncManager.GetFirstHDSyncSyncUnit();
                if (sUnivSyncUnitForEachSensorPtr==NULL)
                {
                    printf("## ERROR:: HD sync table is not valid\n");
                    return false;
                }
            }

            for ( ; frameCurrent<=frameLast;++frameCurrent)
            {

                if (frameCurrent%100==0)
                    printf("## Processing: %08d_00_%02d\n", frameCurrent, hdCamIdx);

                // File exist Check
                char fileName[256];
                char save_subFolder[512];
                sprintf(save_subFolder, "%s/%03dXX/%08d", writeTxt.c_str(), int(frameCurrent/100), frameCurrent);
                sprintf(fileName, "%s/hd%08d_%02d_%02d.txt", save_subFolder, frameCurrent, 0, hdCamIdx);
                if (op::existFile(fileName))
                {
                    if (frameCurrent%100==0)
                        printf("# File already exists: %s\n",fileName);
                    frameCurrent++;
                    continue;
                }

                // Find closest frame
                double expectedUnivTime = syncManager.m_hdTCTimeTable_for30fps[frameCurrent].second;
                double univTimeDiff;
                int frameIdxInRaw = CSyncManager::findClosetFrameIdx_fromUnivTime(
                    expectedUnivTime, *sUnivSyncUnitForEachSensorPtr, univTimeDiff);
                bool bValid;
                if (abs(univTimeDiff) <1e-3)     //should be zero
                {
                    cv::Mat hdImg;
                    bValid = imageExtraction_hd(cameraInfo, frameIdxInRaw, hdImg);
                    imgVect.push_back(hdImg);
                    camIdxVect.push_back(hdCamIdx);
                }
                else
                {
                    printf("FrameDrop detected: frame %d of %d-%d (%d_%d):: univTimeDiff: %f\n",
                           frameCurrent, hdMachineIdx, hdDiskIdx, 0, hdCamIdx, univTimeDiff);
                    continue;
                }
                if (bValid ==false)
                {
                    //Frame droped: frame %d, panel %d\n",frameCurrent,p);
                    printf("Something wrong\n");
                    continue;
                }
                // For next loading... Should do -1 to save "currently" extracte data
                frameCurrent++;
                return true;
            }
        }
    }
    if (cameraInfo.m_fp)
    {
        fclose(cameraInfo.m_fp);
        cameraInfo.m_fp = NULL;
    }
    return false;   //no more available raw video
}

// This worker will just read and return all the basic image file formats in a directory
class WUserInputHd : public op::WorkerProducer<std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>>
{
public:
    WUserInputHd(
        const std::string& rawDirectory, const int frameFirst, const int frameLast, const std::string& writeTxt,
        const int cameraIndexStart, const int cameraIndexEnd, const bool printVerbose) :
        mPrintVerbose{printVerbose},
        mRawDirectory{rawDirectory},
        mFrameFirst{frameFirst},
        mFrameLast{frameLast},
        mWriteTxt{writeTxt},
        mCameraIndexStart{cameraIndexStart},
        mCameraIndexEnd{cameraIndexEnd},
        mHdMachineEnd{46},
        mFrameCurrent{mFrameFirst},
        mHdMachineIdx{31},
        mPrevHdMachineIdx{-1},
        mImgIdx{std::numeric_limits<unsigned int>::max()},
        mPrevPanelIdx{-1},
        pSUnivSyncUnitForEachSensor{NULL}
    {
        try
        {
            if (!mRawDirectory.empty())
            {
                // boost::filesystem::path pp(mRawDirectory);
                // mRawFileName = pp.stem().string();
                mRawFileName = op::getFileNameNoExtension(mRawDirectory);
                // E.g., mRawDirectory = /media/posefs12a/Captures/specialEvents/190125_pose2
                printf("mRawDirectory: %s\n", mRawDirectory.c_str());
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
                if (extractRawImgsHd(
                    mImgVect, mCamIdxVect, mCameraInfo, mSyncManager, pSUnivSyncUnitForEachSensor,
                    mFrameCurrent, mHdMachineIdx, mHdDiskIdx, mPrevHdMachineIdx, mPrevPanelIdx,
                    mRawDirectory, mWriteTxt, mRawFileName, mCameraIndexStart, mCameraIndexEnd, mHdMachineEnd,
                    mFrameFirst, mFrameLast, mPrintVerbose
                    ) == false)
                {
                    op::log("No more images, closing software...", op::Priority::High);
                    this->stop();
                    return nullptr;
                }
                mImgIdx = 0u;
            }

            if (mImgVect.size() != mCamIdxVect.size())
            {
                op::error("Check failed (mImgVect.size() == mCamIdxVect.size()): " + std::to_string(mImgVect.size())
                          + " vs " + std::to_string(mCamIdxVect.size()), __LINE__, __FUNCTION__, __FILE__);
            }
            if (mPrintVerbose)
                printf("mImgIdx: %d\n", mImgIdx);

            // Create new datum
            auto datumsPtr = std::make_shared<std::vector<std::shared_ptr<DomeDatum>>>();
            datumsPtr->emplace_back();
            auto& datumPtr = datumsPtr->at(0);
            datumPtr = std::make_shared<DomeDatum>();

            // Fill datum
            datumPtr->cvInputData = mImgVect[mImgIdx];
            datumPtr->frameNumber = mFrameCurrent-1;
            datumPtr->panelIdx = 0;
            datumPtr->camIdx = mCamIdxVect[mImgIdx];
            mImgIdx++;

            // If empty frame -> return nullptr
            if (datumPtr->cvInputData.empty())
            {
                op::log("Empty frame detected. Ignoring frame.", op::Priority::High);
                datumsPtr = nullptr;
            }

            return datumsPtr;
        }
        catch (const std::exception& e)
        {
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
            return nullptr;
        }
    }

private:
    const bool mPrintVerbose;
    const std::string mRawDirectory;
    const int mFrameFirst;
    const int mFrameLast;
    const std::string mWriteTxt;
    const int mCameraIndexStart;
    const int mCameraIndexEnd;
    const int mHdMachineEnd;
    int mFrameCurrent;
    int mHdMachineIdx;
    int mPrevHdMachineIdx;
    int mPrevPanelIdx;
    int mHdDiskIdx;
    unsigned int mImgIdx;
    std::vector<cv::Mat> mImgVect;
    std::vector<int> mCamIdxVect;
    std::string mRawFileName;
    SUnivSyncUnitForEachSensor* pSUnivSyncUnitForEachSensor;
    CameraInfo mCameraInfo;
    CSyncManager mSyncManager;
};

#endif // OPENPOSE_EXAMPLES_DOME_W_USER_INPUT_HPP
