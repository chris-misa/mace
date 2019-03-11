export ANALYSIS_CMD="Rscript report_one_shot.r"
export FINAL_ANALYSIS_CMD="Rscript report_iperf_bench.r"

export MANIFEST="manifest"

echo "  Starting initial analysis"
mkdir ${1}/cdfs
for i in `cat ${1}/${MANIFEST}`
do
	$ANALYSIS_CMD ${1}/${i}/
	mv ${1}/${i}/cdfs.pdf ${1}/cdfs/${i}_cdfs.pdf
done

$FINAL_ANALYSIS_CMD ${1}/
