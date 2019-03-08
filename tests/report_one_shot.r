#
# R script to parse ping and latency data from one_shot.sh test run
#
# Draws a CDF of native control, container control, native monitored, container monitored RTT
# Also computes container monitored - latencies 
#
source("readPingFile.r")
source("readLatencyFile.r")

#
# Argument should be directory holding raw data
#
args <- commandArgs(trailingOnly=T)
if (length(args) != 1) {
  stop("Need path to data")
}

data_path <- args[1]
SAVED_DATA_PATH <- paste(data_path, "/saved_r_data", sep="")
SUMMARY_DATA_PATH <- paste(data_path, "/means", sep="")

#
# Wrapers around utility parsing functions sourced above
#
read_ping <- function(line) {
  d <- readPingFile(paste(data_path, "/", line, sep=""))
  cat("Read ", length(d$rtt), " rtts from ", line, "\n")
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

cat("---------------Running Report for one_shot.sh------------\n")

#
# Load data, either saved from previous run or from raw files
#
if (file.exists(SAVED_DATA_PATH)) {
  cat("  Found saved data\n")
  data <- dget(SAVED_DATA_PATH)
  rtts <- data$rtts
  latencies <- data$latencies

} else {
  cat("  Reading raw data\n")

  rtts <- list(
    native_control = list(),
    native_monitored = list(),
    container_control = list(),
    container_monitored = list()
  )

  latencies <- list(
    native = list(),
    container = list()
  )

  con <- file(paste(data_path, "/manifest", sep=""), "r")
  while (T) {
    line <- readLines(con, n=1)
    if (length(line) == 0) {
      break
    }
    read <- 0

    # Branch for ping files
    if (length(grep("\\.ping", line)) != 0) {
      
      # Branch for native ping files
      if (length(grep("native", line)) != 0) {

        # Branch for native ping control files
        if (length(grep("control", line)) != 0) {
          rtts$native_control <- append(rtts$native_control, read_ping(line))
          read <- 1
        }

        # Branch for native ping monitored files
        if (length(grep("monitored", line)) != 0) {
          rtts$native_monitored <- append(rtts$native_monitored, read_ping(line))
          read <- 1
        }
      }

      # Branch for container files
      if (length(grep("container", line)) != 0) {

        # Branch for container ping control files
        if (length(grep("control", line)) != 0) {
          rtts$container_control <- append(rtts$container_control, read_ping(line))
          read <- 1
        }

        # Branch for container ping monitored files
        if (length(grep("monitored", line)) != 0) {
          rtts$container_monitored <- append(rtts$container_monitored, read_ping(line))
          read <- 1
        }
      }
    }

    # Branch for latency files
    if (length(grep("\\.lat", line)) != 0) {
      
      # Branch for container monitored
      if (length(grep("container_monitored", line)) != 0) {
        latencies$container <- append(latencies$container, read_latency(line))
        read <- 1
      }

    }

    # Print per-file message
    if (read == 0) {
      cat("Warning: skipping unknown file:", line, "\n")
    }
  }
  close(con)

  dput(list(rtts=rtts, latencies=latencies), file=SAVED_DATA_PATH)
}

#
# Crude application of latency data to container monitored RTT
#
corrected <- rtts$container_monitored$rtt - (latencies$container$ingress$latency + latencies$container$egress$latency)


#
# Write out summary stats
#
dput(list(native_control=list(mean=mean(rtts$native_control$rtt),
			      sd=sd(rtts$native_control$rtt)),
	  native_monitored=list(mean=mean(rtts$native_monitored$rtt),
				sd=sd(rtts$native_monitored$rtt)),
	  container_control=list(mean=mean(rtts$container_control$rtt),
				 sd=sd(rtts$container_control$rtt)),
	  container_monitored=list(mean=mean(rtts$container_monitored$rtt),
				   sd=sd(rtts$container_monitored$rtt))),
     file=SUMMARY_DATA_PATH)

#
# Draw cdfs
#
xbnds <- c(0, 100)
pdf(file=paste(data_path, "/cdfs.pdf", sep=""))
plot(0, type="n", ylim=c(0,1), xlim=xbnds,
     xlab=expression(paste("RTT (",mu,"s)", sep="")),
     ylab="CDF",
     main="")
lines(ecdf(rtts$native_control$rtt), col="pink", do.points=F, verticals=T)
lines(ecdf(rtts$native_monitored$rtt), col="purple", do.points=F, verticals=T)
lines(ecdf(rtts$container_control$rtt), col="lightblue", do.points=F, verticals=T)
lines(ecdf(rtts$container_monitored$rtt), col="blue", do.points=F, verticals=T)

lines(ecdf(corrected), col="black", do.point=F, verticals=T)

legend("bottomright",
  legend=c("native control", "native monitored", "container control", "container monitored", "container corrected"),
  col=c("pink", "purple", "lightblue", "blue", "black"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()
cat("Done.\n")



