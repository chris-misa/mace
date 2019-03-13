export ANALYSIS_CMD="Rscript report_one_shot.r"
export FINAL_ANALYSIS_CMD="Rscript report_iperf_bench.r"

export MANIFEST="manifest"

CDFS_HTML_PATH=${1}/cdfs.html

cat > $CDFS_HTML_PATH << END
<!DOCTYPE html>
<head>
<title>CDFS</title>
</head>
<body>
END

echo "  Starting initial analysis"
mkdir ${1}/cdfs
for i in `cat ${1}/${MANIFEST}`
do
	$ANALYSIS_CMD ${1}/${i}/
	mv ${1}/${i}/cdfs.pdf ${1}/cdfs/${i}_cdfs.pdf
	echo "<embed src='cdfs/${i}_cdfs.pdf' width='500' height='500' type='application.pdf' />" >> $CDFS_HTML_PATH
done

echo "</body>" >> $CDFS_HTML_PATH

$FINAL_ANALYSIS_CMD ${1}/
