CAPTURES_NAS=$1
PROCESSED_NAS=$2
datasetName=$3
startIdx=$4
startEnd=$5  # end index
camNum=$6
numgpu=$7

rawFolder=/media/posefs${CAPTURES_NAS}/Captures/specialEvents
saveFolder=/media/posefs${PROCESSED_NAS}/Processed/specialEvents
mkdir -p $saveFolder

echo $numgpu
set -e
./peakdetect_vga_raw_arg.sh $rawFolder/$datasetName $saveFolder/${datasetName}/body_mpm $startIdx $startEnd BODY_25B $camNum $numgpu
./vga_merger_arg.sh $saveFolder/${datasetName}/body_mpm op25_heatmaps_org/vga_25 op25_poseDetect_mpm_org/vga_25 $startIdx $startEnd 25
echo "" > $saveFolder/$datasetName/done_pose_org.log
