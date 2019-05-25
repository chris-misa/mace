#
# R script to parse ping and latency data from run_params.sh test run
#
source("readPingFile.r")

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
    native = list(means = c(), sds = c(), sizes = c(), raw = list()),
    container = list(means = c(), sds = c(), sizes = c(), raw = list())
  )

  con <- file(paste(data_path, "/manifest", sep=""), "r")
  while (T) {
    line <- readLines(con, n=1)
    if (length(line) == 0) {
      break
    }
    read <- 0

    linePattern <- ".*_([0-9\\.]+)\\.ping"
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
        rtts$native$raw[[length(rtts$native$raw) + 1]] <- newRtts$rtt
        read <- 1
      }

      # Branch for container files
      if (length(grep("container", line)) != 0) {

        newRtts <- read_ping(line)
        rtts$container$means <- c(rtts$container$means, mean(newRtts$rtt))
        rtts$container$sds <- c(rtts$container$sds, sd(newRtts$rtt))
        rtts$container$sizes <- c(rtts$container$sizes, size)
        rtts$container$raw[[length(rtts$container$raw) + 1]] <- newRtts$rtt
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
#plot(rtts$native$sizes + 8, meanDiffs, pch=19, col=rgb(0,0,0,0.5),
#    xlab="Packet Size (bytes)",
#    ylab=expression(paste("RTT Mean Difference (",mu,"s)", sep="")),
#    main="")
boxplot(Y~X, data=data.frame(X=rtts$native$sizes + 8, Y=meanDiffs),
    xlab="Packet Size (bytes)",
    ylab=expression(paste("RTT Mean Difference (",mu,"s)", sep="")),
    main="")
#abline(f)
dev.off()



#
# Plot raw mean values (not as differences)
#
pdf(file=paste(data_path, "/raw_means.pdf", sep=""))
par(mar=c(4,4,2,2), cex=0.85)
plot(0, type="n", xlim=range(rtts$native$sizes),
                  ylim=range(rtts$native$means, rtts$container$means))
lines(rtts$native$sizes, rtts$native$means, type="p", pch=1)
lines(rtts$container$sizes, rtts$container$means, type="p", pch=2)

dev.off()

#
# Plot raw data values (not as means)
#

nativeRawFrame <- data.frame(rtt=c(), size=c())
for (i in 1:length(rtts$native$sizes)) {
  nativeRawFrame <- rbind(nativeRawFrame,
    data.frame(rtt=rtts$native$raw[[i]],
               size=rep(rtts$native$sizes[[i]], length(rtts$native$raw[[i]]))))
}

containerRawFrame <- data.frame(rtt=c(), size=c())
for (i in 1:length(rtts$container$sizes)) {
  containerRawFrame <- rbind(containerRawFrame,
    data.frame(rtt=rtts$container$raw[[i]],
               size=rep(rtts$container$sizes[[i]], length(rtts$container$raw[[i]]))))
}

pdf(file=paste(data_path, "/raw_data_native.pdf", sep=""))
par(mar=c(4,4,2,2), cex=0.8)
stripchart(rtt~size, data=nativeRawFrame,
        vertical=T, pch=20, col=rgb(0,0,0,0.2), ylim=c(0,100))
dev.off()

pdf(file=paste(data_path, "/raw_data_container.pdf", sep=""))
par(mar=c(4,4,2,2), cex=0.8)
stripchart(rtt~size, data=containerRawFrame,
        vertical=T, pch=20, col=rgb(0,0,0,0.2), ylim=c(0,100))
dev.off()

# This makes container strips fall into different levels of the size factor
containerRawFrame$size <- containerRawFrame$size + 2

colors <- rep(c(rgb(0,0,0,0.2), rgb(0.5,0,0,0.2)), length(rtts$native$raw))

# Absolute hack to get both these on same page. . ..
pdf(file=paste(data_path, "/raw_data_100.pdf", sep=""))
par(mar=c(4,4,2,2), cex=0.8)
stripchart(rtt~size, data=rbind(nativeRawFrame, containerRawFrame),
        vertical=T, pch=20, col=colors, ylim=c(0,100))
dev.off()

pdf(file=paste(data_path, "/raw_data_200.pdf", sep=""))
par(mar=c(4,4,2,2), cex=0.8)
stripchart(rtt~size, data=rbind(nativeRawFrame, containerRawFrame),
        vertical=T, pch=20, col=colors, ylim=c(0,200))
dev.off()

pdf(file=paste(data_path, "/raw_data_2000.pdf", sep=""))
par(mar=c(4,4,2,2), cex=0.8)
stripchart(rtt~size, data=rbind(nativeRawFrame, containerRawFrame),
        vertical=T, pch=20, col=colors, ylim=c(0,2000))
dev.off()

pdf(file=paste(data_path, "/raw_data_max.pdf", sep=""))
par(mar=c(4,4,2,2), cex=0.8)
stripchart(rtt~size, data=rbind(nativeRawFrame, containerRawFrame),
        vertical=T, pch=20, col=colors)
dev.off()


cat("Done.\n")
