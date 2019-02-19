saveFolder=/media/posefs3b/Users/gines/dome/Processed/specialEventsOP
rm -rf $saveFolder





# rawFolder=/media/posefs12a/Captures/specialEvents
# datasetName=190125_pose2

# # saveFolder=/media/posefs11b/Processed/specialEvents
# mkdir -p $saveFolder
# # startIdx=100;
# # startEnd=3700;
# startIdx=998;
# startEnd=1000;
# # camNum=480;
# camNum=140;
# ./peakdetect_vga_raw_arg.sh $rawFolder/$datasetName $saveFolder/${datasetName}/body_mpm $startIdx $startEnd BODY_25B $camNum
# # echo ./vga_merger_arg.sh $saveFolder/$datasetName heatmaps_org/vga_25 poseDetect_pm_org/vga_25 $startIdx $startEnd 25
# # ./vga_merger_arg.sh $saveFolder/${datasetName}/body_mpm heatmaps_org/vga_25 poseDetect_mpm_org/vga_25 $startIdx $startEnd 25 &
# # echo "" > $saveFolder/$datasetName/done_pose_org.log





rawFolder=/media/posefs5a/Captures/SocialGames
datasetName=160226_ultimatum1

# saveFolder=/media/posefs11b/Processed/specialEvents
mkdir -p $saveFolder
startIdx=4698;
startEnd=4700;
camNum=480;
# camNum=10;
./peakdetect_vga_raw_arg.sh $rawFolder/$datasetName $saveFolder/${datasetName}/body_mpm $startIdx $startEnd BODY_25B $camNum
echo ./vga_merger_arg.sh $saveFolder/${datasetName}/body_mpm heatmaps_org/vga_25 poseDetect_mpm_org/vga_25 $startIdx $startEnd 25
./vga_merger_arg.sh $saveFolder/${datasetName}/body_mpm heatmaps_org/vga_25 poseDetect_mpm_org/vga_25 $startIdx $startEnd 25
echo "" > $saveFolder/$datasetName/done_pose_org.log





rawFolder=/media/posefs5a/Captures/SocialGames
datasetName=160226_ultimatum1

# saveFolder=/media/posefs11b/Processed/specialEvents
mkdir -p $saveFolder
startIdx=3698;
startEnd=3700;
./peakdetect_hd_raw_arg.sh $rawFolder/$datasetName $saveFolder/${datasetName}/body_mpm $startIdx $startEnd BODY_25B
echo ./hd_merger_arg.sh $saveFolder/${datasetName}/body_mpm heatmaps_org/hd_30 poseDetect_mpm_org/hd_30 $startIdx $startEnd 25
./hd_merger_arg.sh $saveFolder/${datasetName}/body_mpm heatmaps_org/hd_30 poseDetect_mpm_org/hd_30 $startIdx $startEnd 25
echo "" > $saveFolder/$datasetName/done_pose_hd_org.log
