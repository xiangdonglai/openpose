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
#include "wUserOutput.hpp"

// Custom OpenPose flags
// Dome input
DEFINE_int32(dome_mode,     1, "1 for VGA, 2 for HD cameras.");
DEFINE_bool(print_verbose,  false, "Enable or disable some extra command line verbose output. If, you might also want"
                                   " to add the flag: `--logtostderr`");
DEFINE_uint64(frame_first,  0, "Start on desired frame number. Indexes are 0-based, i.e., the first frame has index 0.");
DEFINE_uint64(frame_last,   -1, "Finish on desired frame number. Select -1 to disable. Indexes are 0-based, e.g., if"
                                " set to 10, it will process 11 frames (0-10).");
DEFINE_string(rawDir,       "", "Use a raw files");
DEFINE_string(cameraSamplingInfoFolder, "/media/posefs1b/Users/hanbyulj/cameraSamplingInfo_furthest",
    "VGA camera sampling info. It was a hard-coded value introduced by Hanbyul Joo inside the code. Not sure what it does.");
// VGA-only
DEFINE_int32(panel_start,   1, "Panel start index (only for VGA)");
DEFINE_int32(panel_end,     1, "Panel end index (only for VGA)");
DEFINE_int32(cam_sampleNum, 480, "Write joint data with json format as prefix%06d.json (only for VGA)");
// HD-only
DEFINE_int32(cam_start,     0, "HD camera start index (only for HD)");
DEFINE_int32(cam_end,       0, "HD camera end index (only for HD)");
// Dome output
DEFINE_string(write_txt, "",
    "Write joint data with json format as prefix%06d.json");

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
        // poseMode
        const auto poseMode = op::flagsToPoseMode(FLAGS_body);
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
        std::shared_ptr<op::WorkerProducer<std::shared_ptr<std::vector<std::shared_ptr<DomeDatum>>>>> wUserInput;
        if (FLAGS_dome_mode == 1)
            wUserInput = std::make_shared<WUserInputVga>(
                FLAGS_cam_sampleNum, FLAGS_rawDir, FLAGS_frame_first, FLAGS_frame_last, FLAGS_panel_start,
                FLAGS_panel_end, FLAGS_cameraSamplingInfoFolder, FLAGS_write_txt, FLAGS_print_verbose);
        else if (FLAGS_dome_mode == 2)
            wUserInput = std::make_shared<WUserInputHd>(
                FLAGS_rawDir, FLAGS_frame_first, FLAGS_frame_last, FLAGS_write_txt, FLAGS_cam_start,
                FLAGS_cam_end, FLAGS_print_verbose);
        else
            op::error("Unknown `--dome_mode`.", __LINE__, __FUNCTION__, __FILE__);
        // GUI (Display)
        const bool isHd = (FLAGS_dome_mode == 2);
        auto wUserOutput = std::make_shared<WUserOutput>(poseModel, FLAGS_write_txt, isHd);

        // Add custom input
        const auto workerInputOnNewThread = false;
        opWrapperT.setWorker(op::WorkerType::Input, wUserInput, workerInputOnNewThread);
        // Add custom output
        const auto workerOutputOnNewThread = true;
        opWrapperT.setWorker(op::WorkerType::Output, wUserOutput, workerOutputOnNewThread);

        // Pose configuration (use WrapperStructPose{} for default and recommended configuration)
        const op::WrapperStructPose wrapperStructPose{
            poseMode, netInputSize, outputSize, keypointScaleMode, FLAGS_num_gpu, FLAGS_num_gpu_start,
            FLAGS_scale_number, (float)FLAGS_scale_gap, op::flagsToRenderMode(FLAGS_render_pose, multipleView),
            poseModel, !FLAGS_disable_blending, (float)FLAGS_alpha_pose, (float)FLAGS_alpha_heatmap,
            FLAGS_part_to_show, FLAGS_model_folder, heatMapTypes, heatMapScaleMode, FLAGS_part_candidates,
            (float)FLAGS_render_threshold, FLAGS_number_people_max, FLAGS_maximize_positives, FLAGS_fps_max,
            FLAGS_prototxt_path, FLAGS_caffemodel_path, (float)FLAGS_upsampling_ratio, enableGoogleLogging};
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
