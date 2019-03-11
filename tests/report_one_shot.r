#
# R script to parse ping and latency data from one_shot.sh test run
#
# Draws a CDF of native control, container control, native monitored, container monitored RTT
# Also computes container monitored - latencies 
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
# Function to apply latency overheads to gathered RTTs
# Note: step functions and nearest time-stamp don't seem to work on this data.
# Note: timestamps do not seem to be reliable: this doesn't work
#
applyLatencies <- function(in_rtts, ingresses, egresses) {
  ing <- 1
  egr <- 1
  i <- 1

  res <- c()
  timestamps <- c()
  while (i <= length(in_rtts$rtt)) {

    # Move ing, egr pointers to surround current rtt
    while ((ing < length(ingresses$ts) - 1) && (ingresses$ts[[ing + 1]] < in_rtts$ts[[i]])) {
      ing <- ing + 1
    }
    while ((egr < length(egresses$ts) - 1) && (egresses$ts[[egr + 1]] < in_rtts$ts[[i]])) {
      egr <- egr + 1
    }

    # Choose nearest ing
    if (in_rtts$ts[[i]] - ingresses$ts[[ing]] < ingresses$ts[[ing+1]] - in_rtts$ts[[i]]) {
      ing_chose <- ing
    } else {
      ing_chose <- ing + 1
    }

    # Choose nearest egr
    if (in_rtts$ts[[i]] - egresses$ts[[egr]] < egresses$ts[[egr+1]] - in_rtts$ts[[i]]) {
      egr_chose <- egr
    } else {
      egr_chose <- egr + 1
    }

    res <- c(res, in_rtts$rtt[[i]] - (ingresses$latency[[ing_chose]] + egresses$latency[[egr_chose]]))
    timestamps <- c(timestamps, in_rtts$ts[[i]])

    i <- i + 1
  }
  data.frame(rtt=res, ts=timestamps)
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
    native_control_hw = list(),
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

        # Branch for native hardware ping results
        if (length(grep("hardware", line)) != 0) {
          rtts$native_control_hw <- append(rtts$native_control_hw, read_hw_ping(line))
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

      # Branch for native monitored
      if (length(grep("native_monitored", line)) != 0) {
        latencies$native <- append(latencies$native, read_latency(line))
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
container_corrected <- data.frame(rtt=rtts$container_monitored$rtt - (latencies$container$ingress$latency + latencies$container$egress$latency),
                                  ts=rtts$container_monitored$ts)
native_corrected <- data.frame(rtt=rtts$native_monitored$rtt - (latencies$native$ingress$latency + latencies$native$egress$latency),
                                  ts=rtts$native_monitored$ts)

# container_corrected <- applyLatencies(rtts$container_monitored, latencies$container$ingress, latencies$container$egress)
# native_corrected <- applyLatencies(rtts$native_monitored, latencies$native$ingress, latencies$native$egress)

#
# Deal with sometimes not having hardware rtts
#
if (length(rtts$native_control_hw) == 0) {
	rtts$native_control_hw = data.frame(rtt=c(0), ts=c(0))
}


#
# Get cdfs
#
native_control_hw_cdf <- ecdf(rtts$native_control_hw$rtt)
native_control_cdf <- ecdf(rtts$native_control$rtt)
native_monitored_cdf <- ecdf(rtts$native_monitored$rtt)
container_control_cdf <- ecdf(rtts$container_control$rtt)
container_monitored_cdf <- ecdf(rtts$container_monitored$rtt)

container_corrected_cdf <- ecdf(container_corrected$rtt)
native_corrected_cdf <- ecdf(native_corrected$rtt)
#
# Write out summary stats
#
dput(list(native_control=list(mean=mean(rtts$native_control$rtt),
			      sd=sd(rtts$native_control$rtt),
			      median=median(rtts$native_control$rtt)),
          native_control_hw=list(mean=mean(rtts$native_control_hw$rtt),
			      sd=sd(rtts$native_control_hw$rtt),
			      median=median(rtts$native_control_hw$rtt)),
	  native_monitored=list(mean=mean(rtts$native_monitored$rtt),
				sd=sd(rtts$native_monitored$rtt),
				median=median(rtts$native_monitored$rtt)),
	  container_control=list(mean=mean(rtts$container_control$rtt),
				 sd=sd(rtts$container_control$rtt),
				 median=median(rtts$container_control$rtt)),
	  container_monitored=list(mean=mean(rtts$container_monitored$rtt),
				   sd=sd(rtts$container_monitored$rtt),
				   median=median(rtts$container_monitored$rtt)),
	  container_corrected=list(mean=mean(container_corrected$rtt),
			 sd=sd(container_corrected$rtt),
			 median=median(container_corrected$rtt)),
	  native_corrected=list(mean=mean(native_corrected$rtt),
			 sd=sd(native_corrected$rtt),
			 median=median(native_corrected$rtt)),
    native_pert_area = cdfArea(native_control_cdf, native_monitored_cdf),
    container_pert_area = cdfArea(container_control_cdf, container_monitored_cdf),
    native_corrected_area = cdfArea(native_control_hw_cdf, native_corrected_cdf),
    container_corrected_area = cdfArea(native_control_hw_cdf, container_corrected_cdf)),
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
lines(native_control_hw_cdf, col="green", do.points=F, verticals=T)
lines(native_control_cdf, col="pink", do.points=F, verticals=T)
lines(native_monitored_cdf, col="purple", do.points=F, verticals=T)
lines(container_control_cdf, col="lightblue", do.points=F, verticals=T)
lines(container_monitored_cdf, col="blue", do.points=F, verticals=T)

lines(container_corrected_cdf, col="black", do.point=F, verticals=T)
lines(native_corrected_cdf, col="gray", do.point=F, verticals=T)

legend("bottomright",
  legend=c("hardware", "native control", "native monitored", "container control", "container monitored", "container corrected", "native corrected"),
  col=c("green", "pink", "purple", "lightblue", "blue", "black", "gray"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()
cat("Done.\n")
