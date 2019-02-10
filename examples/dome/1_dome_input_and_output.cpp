// --- OpenPose C++ API Tutorial - Example 1 - Dome Input and Output ---
// Synchronous mode: ideal for production integration. It provides the fastest results with respect to runtime
// performance.
// In this function, the user can implement its own way to read frames, implement its own post-processing (i.e., his
// function will be called after OpenPose has processed the frames but before saving), visualizing any result
// render/display/storage the results, and use their custom Datum structure

// Command-line user intraface
#define OPENPOSE_FLAGS_DISABLE_PRODUCER
// #define OPENPOSE_FLAGS_DISABLE_DISPLAY
#include <openpose/flags.hpp>
// OpenPose dependencies
#include <openpose/headers.hpp>
// Other files
#include "domeDatum.hpp"
#include "wUserInput.hpp"

// Custom OpenPose flags
// Producer
DEFINE_string(image_dir,                "examples/media/",
    "Process a directory of images. Read all standard formats (jpg, png, bmp, etc.).");
// Display
DEFINE_bool(no_display,                 false,
    "Enable to disable the visual display.");
// Dome
DEFINE_uint64(frame_first,              0,
    "Start on desired frame number. Indexes are 0-based, i.e., the first frame has index 0.");
DEFINE_uint64(frame_last,               -1,
    "Finish on desired frame number. Select -1 to disable. Indexes are 0-based, e.g., if set to"
    " 10, it will process 11 frames (0-10).");
DEFINE_string(write_txt, "",
              "Write joint data with json format as prefix%06d.json");
DEFINE_int32(panel_start, 1,
             "Panel start index");
DEFINE_int32(panel_end, 1,
             "Panel end index");
DEFINE_int32(cam_sampleNum, 480,
             "Write joint data with json format as prefix%06d.json");
DEFINE_string(rawDir, "",
              "Use a raw files");

// // This worker will just invert the image
// class WUserPostProcessing : public op::Worker<std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>>
// {
// public:
//     WUserPostProcessing()
//     {
//         // User's constructor here
//     }

//     void initializationOnThread() {}

//     void work(std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>& datumsPtr)
//     {
//         // User's post-processing (after OpenPose processing & before OpenPose outputs) here
//             // datumPtr->cvOutputData: rendered frame with pose or heatmaps
//             // datumPtr->poseKeypoints: Array<float> with the estimated pose
//         try
//         {
//             if (datumsPtr != nullptr && !datumsPtr->empty())
//                 for (auto& datumPtr : *datumsPtr)
//                     cv::bitwise_not(datumPtr->cvOutputData, datumPtr->cvOutputData);
//         }
//         catch (const std::exception& e)
//         {
//             this->stop();
//             op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
//         }
//     }
// };

// This worker will just read and return all the jpg files in a directory
class WUserOutput : public op::WorkerConsumer<std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>>
{
public:
    void initializationOnThread() {}

    void workConsumer(const std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>& datumsPtr)
    {
        try
        {
            // User's displaying/saving/other processing here
                // datumPtr->cvOutputData: rendered frame with pose or heatmaps
                // datumPtr->poseKeypoints: Array<float> with the estimated pose
            if (datumsPtr != nullptr && !datumsPtr->empty())
            {
                // Show in command line the resulting pose keypoints for body, face and hands
                op::log("\nKeypoints:");
                // Accesing each element of the keypoints
                const auto& poseKeypoints = datumsPtr->at(0)->poseKeypoints;
                op::log("Person pose keypoints:");
                for (auto person = 0 ; person < poseKeypoints.getSize(0) ; person++)
                {
                    op::log("Person " + std::to_string(person) + " (x, y, score):");
                    for (auto bodyPart = 0 ; bodyPart < poseKeypoints.getSize(1) ; bodyPart++)
                    {
                        std::string valueToPrint;
                        for (auto xyscore = 0 ; xyscore < poseKeypoints.getSize(2) ; xyscore++)
                        {
                            valueToPrint += std::to_string(   poseKeypoints[{person, bodyPart, xyscore}]   ) + " ";
                        }
                        op::log(valueToPrint);
                    }
                }
                op::log(" ");
                // Alternative: just getting std::string equivalent
                op::log("Face keypoints: " + datumsPtr->at(0)->faceKeypoints.toString());
                op::log("Left hand keypoints: " + datumsPtr->at(0)->handKeypoints[0].toString());
                op::log("Right hand keypoints: " + datumsPtr->at(0)->handKeypoints[1].toString());
                // Heatmaps
                const auto& poseHeatMaps = datumsPtr->at(0)->poseHeatMaps;
                if (!poseHeatMaps.empty())
                {
                    op::log("Pose heatmaps size: [" + std::to_string(poseHeatMaps.getSize(0)) + ", "
                            + std::to_string(poseHeatMaps.getSize(1)) + ", "
                            + std::to_string(poseHeatMaps.getSize(2)) + "]");
                    const auto& faceHeatMaps = datumsPtr->at(0)->faceHeatMaps;
                    op::log("Face heatmaps size: [" + std::to_string(faceHeatMaps.getSize(0)) + ", "
                            + std::to_string(faceHeatMaps.getSize(1)) + ", "
                            + std::to_string(faceHeatMaps.getSize(2)) + ", "
                            + std::to_string(faceHeatMaps.getSize(3)) + "]");
                    const auto& handHeatMaps = datumsPtr->at(0)->handHeatMaps;
                    op::log("Left hand heatmaps size: [" + std::to_string(handHeatMaps[0].getSize(0)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(1)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(2)) + ", "
                            + std::to_string(handHeatMaps[0].getSize(3)) + "]");
                    op::log("Right hand heatmaps size: [" + std::to_string(handHeatMaps[1].getSize(0)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(1)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(2)) + ", "
                            + std::to_string(handHeatMaps[1].getSize(3)) + "]");
                }

                // Display results (if enabled)
                if (!FLAGS_no_display)
                {
                    // Display rendered output image
                    cv::imshow(OPEN_POSE_NAME_AND_VERSION + " - Tutorial C++ API", datumsPtr->at(0)->cvOutputData);
                    // Display image and sleeps at least 1 ms (it usually sleeps ~5-10 msec to display the image)
                    const char key = (char)cv::waitKey(1);
                    if (key == 27)
                        this->stop();
                }
            }
        }
        catch (const std::exception& e)
        {
            this->stop();
            op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
        }
    }
};

void configureWrapper(op::WrapperT<DomeDatum>& opWrapperT)
{
    try
    {
        // Configuring OpenPose

        // logging_level
        op::check(0 <= FLAGS_logging_level && FLAGS_logging_level <= 255, "Wrong logging_level value.",
                  __LINE__, __FUNCTION__, __FILE__);
        op::ConfigureLog::setPriorityThreshold((op::Priority)FLAGS_logging_level);
        op::Profiler::setDefaultX(FLAGS_profile_speed);

        // Applying user defined configuration - GFlags to program variables
        // outputSize
        const auto outputSize = op::flagsToPoint(FLAGS_output_resolution, "-1x-1");
        // netInputSize
        const auto netInputSize = op::flagsToPoint(FLAGS_net_resolution, "-1x368");
        // faceNetInputSize
        const auto faceNetInputSize = op::flagsToPoint(FLAGS_face_net_resolution, "368x368 (multiples of 16)");
        // handNetInputSize
        const auto handNetInputSize = op::flagsToPoint(FLAGS_hand_net_resolution, "368x368 (multiples of 16)");
        // poseModel
        const auto poseModel = op::flagsToPoseModel(FLAGS_model_pose);
        // JSON saving
        if (!FLAGS_write_keypoint.empty())
            op::log("Flag `write_keypoint` is deprecated and will eventually be removed."
                    " Please, use `write_json` instead.", op::Priority::Max);
        // keypointScaleMode
        const auto keypointScaleMode = op::flagsToScaleMode(FLAGS_keypoint_scale);
        // heatmaps to add
        const auto heatMapTypes = op::flagsToHeatMaps(FLAGS_heatmaps_add_parts, FLAGS_heatmaps_add_bkg,
                                                      FLAGS_heatmaps_add_PAFs);
        const auto heatMapScaleMode = op::flagsToHeatMapScaleMode(FLAGS_heatmaps_scale);
        // >1 camera view?
        const auto multipleView = (FLAGS_3d || FLAGS_3d_views > 1);
        // Face and hand detectors
        const auto faceDetector = op::flagsToDetector(FLAGS_face_detector);
        const auto handDetector = op::flagsToDetector(FLAGS_hand_detector);
        // Enabling Google Logging
        const bool enableGoogleLogging = true;

        // Initializing the user custom classes
        // Frames producer (e.g., video, webcam, ...)
        auto wUserInput = std::make_shared<WUserInput>(
            FLAGS_cam_sampleNum, FLAGS_rawDir, FLAGS_frame_first, FLAGS_frame_last, FLAGS_panel_start, FLAGS_panel_end,
            FLAGS_write_txt);
        // // Processing
        // auto wUserPostProcessing = std::make_shared<WUserPostProcessing>();
        // // GUI (Display)
        // auto wUserOutput = std::make_shared<WUserOutput>();

        // Add custom input
        const auto workerInputOnNewThread = false;
        opWrapperT.setWorker(op::WorkerType::Input, wUserInput, workerInputOnNewThread);
        // // Add custom processing
        // const auto workerProcessingOnNewThread = false;
        // opWrapperT.setWorker(op::WorkerType::PostProcessing, wUserPostProcessing, workerProcessingOnNewThread);
        // // Add custom output
        // const auto workerOutputOnNewThread = true;
        // opWrapperT.setWorker(op::WorkerType::Output, wUserOutput, workerOutputOnNewThread);

        // Pose configuration (use WrapperStructPose{} for default and recommended configuration)
        const op::WrapperStructPose wrapperStructPose{
            !FLAGS_body_disable, netInputSize, outputSize, keypointScaleMode, FLAGS_num_gpu, FLAGS_num_gpu_start,
            FLAGS_scale_number, (float)FLAGS_scale_gap, op::flagsToRenderMode(FLAGS_render_pose, multipleView),
            poseModel, !FLAGS_disable_blending, (float)FLAGS_alpha_pose, (float)FLAGS_alpha_heatmap,
            FLAGS_part_to_show, FLAGS_model_folder, heatMapTypes, heatMapScaleMode, FLAGS_part_candidates,
            (float)FLAGS_render_threshold, FLAGS_number_people_max, FLAGS_maximize_positives, FLAGS_fps_max,
            FLAGS_prototxt_path, FLAGS_caffemodel_path, enableGoogleLogging};
        opWrapperT.configure(wrapperStructPose);
        // Face configuration (use op::WrapperStructFace{} to disable it)
        const op::WrapperStructFace wrapperStructFace{
            FLAGS_face, faceDetector, faceNetInputSize,
            op::flagsToRenderMode(FLAGS_face_render, multipleView, FLAGS_render_pose),
            (float)FLAGS_face_alpha_pose, (float)FLAGS_face_alpha_heatmap, (float)FLAGS_face_render_threshold};
        opWrapperT.configure(wrapperStructFace);
        // Hand configuration (use op::WrapperStructHand{} to disable it)
        const op::WrapperStructHand wrapperStructHand{
            FLAGS_hand, handDetector, handNetInputSize, FLAGS_hand_scale_number, (float)FLAGS_hand_scale_range,
            op::flagsToRenderMode(FLAGS_hand_render, multipleView, FLAGS_render_pose), (float)FLAGS_hand_alpha_pose,
            (float)FLAGS_hand_alpha_heatmap, (float)FLAGS_hand_render_threshold};
        opWrapperT.configure(wrapperStructHand);
        // Extra functionality configuration (use op::WrapperStructExtra{} to disable it)
        const op::WrapperStructExtra wrapperStructExtra{
            FLAGS_3d, FLAGS_3d_min_views, FLAGS_identification, FLAGS_tracking, FLAGS_ik_threads};
        opWrapperT.configure(wrapperStructExtra);
        // Output (comment or use default argument to disable any output)
        const op::WrapperStructOutput wrapperStructOutput{
            FLAGS_cli_verbose, FLAGS_write_keypoint, op::stringToDataFormat(FLAGS_write_keypoint_format),
            FLAGS_write_json, FLAGS_write_coco_json, FLAGS_write_coco_foot_json, FLAGS_write_coco_json_variant,
            FLAGS_write_images, FLAGS_write_images_format, FLAGS_write_video, FLAGS_write_video_fps,
            FLAGS_write_video_with_audio, FLAGS_write_heatmaps, FLAGS_write_heatmaps_format, FLAGS_write_video_3d,
            FLAGS_write_video_adam, FLAGS_write_bvh, FLAGS_udp_host, FLAGS_udp_port};
        opWrapperT.configure(wrapperStructOutput);
// GUI (comment or use default argument to disable any visual output)
const op::WrapperStructGui wrapperStructGui{
    op::flagsToDisplayMode(FLAGS_display, FLAGS_3d), !FLAGS_no_gui_verbose, FLAGS_fullscreen};
opWrapperT.configure(wrapperStructGui);
        // No GUI. Equivalent to: opWrapper.configure(op::WrapperStructGui{});
        // Set to single-thread (for sequential processing and/or debugging and/or reducing latency)
        if (FLAGS_disable_multi_thread)
            opWrapperT.disableMultiThreading();
    }
    catch (const std::exception& e)
    {
        op::error(e.what(), __LINE__, __FUNCTION__, __FILE__);
    }
}

int tutorialDome()
{
    try
    {
        op::log("Starting OpenPose demo...", op::Priority::High);
        const auto opTimer = op::getTimerInit();

        // OpenPose wrapper
        op::log("Configuring OpenPose...", op::Priority::High);
        op::WrapperT<DomeDatum> opWrapperT;
        configureWrapper(opWrapperT);

        // Start, run, and stop processing - exec() blocks this thread until OpenPose wrapper has finished
        op::log("Starting thread(s)...", op::Priority::High);
        opWrapperT.exec();

        // Measuring total time
        op::printTime(opTimer, "OpenPose demo successfully finished. Total time: ", " seconds.", op::Priority::High);

        // Return
        return 0;
    }
    catch (const std::exception& e)
    {
        return -1;
    }
}

int main(int argc, char *argv[])
{
    // Parsing command line flags
    gflags::ParseCommandLineFlags(&argc, &argv, true);

    // Running tutorialDome
    return tutorialDome();
}
