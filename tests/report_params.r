#
# R script to parse ping and latency data from run_params.sh test run
#
source("readPingFile.r")
source("readHWPingFile.r")
source("readLatencyFile.r")

cdfArea <- function(q, p) {
  Reduce(function(x,y) { x+y }, Map(function(x) { (q(x) - p(x))^2 }, knots(q))) / length(knots(q))
}

#
# Argument should be directory holding raw data
#
args <- commandArgs(trailingOnly=T)
if (length(args) != 1) {
  stop("Need path to data")
}

options(digits=22)

data_path <- args[1]
SAVED_DATA_PATH <- paste(data_path, "/saved_r_data", sep="")
SUMMARY_DATA_PATH <- paste(data_path, "/summary", sep="")

#
# Wrapers around utility parsing functions sourced above
#
read_ping <- function(line) {
  d <- readPingFile(paste(data_path, "/", line, sep=""))
  cat("Read ", length(d$rtt), " rtts from ", line, "\n")
  d
}

read_hw_ping <- function(line) {
  d <- readHWPingFile(paste(data_path, "/", line, sep=""))
  cat("Read ", length(d$rtt), " hardware rtts from ", line, "\n")
  d
}

read_latency <- function(line) {
  d <- readLatencyFile(paste(data_path, "/", line, sep=""))
  cat("Read ", length(d$egress$latency), " egresses and ",
               length(d$ingress$latency), " ingresses from ", line, "\n")
  d
}

#
# Begin main work
#

cat("---------------Running Report for run_params.sh------------\n")

#
# Load data, either saved from previous run or from raw files
#
if (file.exists(SAVED_DATA_PATH)) {
  cat("  Found saved data\n")
  rtts <- dget(SAVED_DATA_PATH)

} else {
  cat("  Reading raw data\n")

  rtts <- list(
    native = list(means = c(), sds = c(), sizes = c()),
    container = list(means = c(), sds = c(), sizes = c())
  )

  con <- file(paste(data_path, "/manifest", sep=""), "r")
  while (T) {
    line <- readLines(con, n=1)
    if (length(line) == 0) {
      break
    }
    read <- 0

    linePattern <- ".*_([0-9]+)\\.ping"
    matches <- grep(linePattern, line, value=T)

    # Branch for ping files
    if (length(matches) != 0) {

      size <- as.numeric(sub(linePattern, "\\1", matches))
      
      # Branch for native ping files
      if (length(grep("native", line)) != 0) {

        newRtts <- read_ping(line)
        rtts$native$means <- c(rtts$native$means, mean(newRtts$rtt))
        rtts$native$sds <- c(rtts$native$sds, sd(newRtts$rtt))
        rtts$native$sizes <- c(rtts$native$sizes, size)
        read <- 1
      }

      # Branch for container files
      if (length(grep("container", line)) != 0) {

        newRtts <- read_ping(line)
        rtts$container$means <- c(rtts$container$means, mean(newRtts$rtt))
        rtts$container$sds <- c(rtts$container$sds, sd(newRtts$rtt))
        rtts$container$sizes <- c(rtts$container$sizes, size)
        read <- 1
      }
    }

    # Print per-file message
    if (read == 0) {
      cat("Warning: skipping unknown file:", line, "\n")
    }
  }
  close(con)

  if (!identical(rtts$container$sizes, rtts$native$sizes)) {
    cat("Native and contaienr runs have different sizes!\n")
  }

  dput(rtts, file=SAVED_DATA_PATH)
}

#
# Plot means
#
meanDiffs <- rtts$container$means - rtts$native$means

f <- lm(Y~X, data=data.frame(X=rtts$native$sizes, Y=meanDiffs))
s <- summary(f)
capture.output(s, file=paste(data_path, "/summary.txt", sep=""))

pdf(file=paste(data_path, "/mean_diffs.pdf", sep=""))
par(mar=c(4,4,2,2))
# add 8 to payload size to get packet size
plot(rtts$native$sizes + 8, meanDiffs, pch=19, col=rgb(0,0,0,0.5),
    xlab="Packet Size (bytes)",
    ylab=expression(paste("RTT Mean Difference (",mu,"s)", sep="")),
    main="")
abline(f)
dev.off()


cat("Done.\n")
