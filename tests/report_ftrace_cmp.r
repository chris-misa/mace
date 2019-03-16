#
# R script to gather data from ftrace comparison runs
#
source("readPingFile.r")

#
# Argument should point to directory of iperf bench run
#
args <- commandArgs(trailingOnly=T)
if (length(args) != 1) {
	stop("Need path to data")
}

data_path <- args[1]

#
# Compute rough confidence interval
#
confidence <- 0.95
getConfidence <- function(sd, length) {
  a <- confidence + 0.5 * (1.0 - confidence)
  t_an <- qt(a, df=length-1)
  t_an * sd / sqrt(length)
}

#
# Work around to draw intervals around
# points in graph
#
drawArrows <- function(xs, ys, sds, color) {
  arrows(xs, ys - sds,
         xs, ys + sds,
         length=0.005, angle=90, code=3, col=color)
}

#
# Wrapper around read ping utility function
#
read_ping <- function(line) {
  d <- readPingFile(paste(data_path, "/", line, sep=""))
  cat("Read ", length(d$rtt), " rtts from ", line, "\n")
  d
}

#
# Begin main work
#


#
# Collect means and sds from all traffic settings
#

SAVED_DATA_PATH <- paste(data_path, "/saved_r_data", sep="")

if (file.exists(SAVED_DATA_PATH)) {
  cat("Found saved data.\n")
  data <- dget(SAVED_DATA_PATH)

  native_control <- data$native_control
  container_control <- data$container_control

  mace_native_monitored <- data$mace_native_monitored
  mace_container_monitored <- data$mace_container_monitored

  ftrace_native_monitored <- data$ftrace_native_monitored
  ftrace_container_monitored <- data$ftrace_container_monitored

  container_counts <- data$container_counts

} else {

  native_control <- list(mean=c(), median=c(), sd=c(), len=c())
  container_control <- list(mean=c(), median=c(), sd=c(), len=c())

  mace_native_monitored <- list(mean=c(), median=c(), sd=c(), len=c())
  mace_container_monitored <- list(mean=c(), median=c(), sd=c(), len=c())

  ftrace_native_monitored <- list(mean=c(), median=c(), sd=c(), len=c())
  ftrace_container_monitored <- list(mean=c(), median=c(), sd=c(), len=c())

  container_counts <- c()

  con <- file(paste(data_path, "/manifest", sep=""), "r")
  while (T) {
    line <- readLines(con, n=1)
    if (length(line) == 0) {
      break
    }
    container_counts <- c(container_counts, as.numeric(line))

    data <- read_ping(paste(line, "/native_control.ping", sep=""))
    native_control$mean <- c(native_control$mean, mean(data$rtt))
    native_control$median <- c(native_control$median, median(data$rtt))
    native_control$sd <- c(native_control$sd, sd(data$rtt))
    native_control$len <- c(native_control$len, length(data$rtt))

    data <- read_ping(paste(line, "/container_control.ping", sep=""))
    container_control$mean <- c(container_control$mean, mean(data$rtt))
    container_control$median <- c(container_control$median, median(data$rtt))
    container_control$sd <- c(container_control$sd, sd(data$rtt))
    container_control$len <- c(container_control$len, length(data$rtt))
     
    data <- read_ping(paste(line, "/mace_native_monitored.ping", sep=""))
    mace_native_monitored$mean <- c(mace_native_monitored$mean, mean(data$rtt))
    mace_native_monitored$median <- c(mace_native_monitored$median, median(data$rtt))
    mace_native_monitored$sd <- c(mace_native_monitored$sd, sd(data$rtt))
    mace_native_monitored$len <- c(mace_native_monitored$len, length(data$rtt))

    data <- read_ping(paste(line, "/mace_container_monitored.ping", sep=""))
    mace_container_monitored$mean <- c(mace_container_monitored$mean, mean(data$rtt))
    mace_container_monitored$median <- c(mace_container_monitored$median, median(data$rtt))
    mace_container_monitored$sd <- c(mace_container_monitored$sd, sd(data$rtt))
    mace_container_monitored$len <- c(mace_container_monitored$len, length(data$rtt))


    data <- read_ping(paste(line, "/ftrace_native_monitored.ping", sep=""))
    ftrace_native_monitored$mean <- c(ftrace_native_monitored$mean, mean(data$rtt))
    ftrace_native_monitored$median <- c(ftrace_native_monitored$median, median(data$rtt))
    ftrace_native_monitored$sd <- c(ftrace_native_monitored$sd, sd(data$rtt))
    ftrace_native_monitored$len <- c(ftrace_native_monitored$len, length(data$rtt))


    data <- read_ping(paste(line, "/ftrace_container_monitored.ping", sep=""))
    ftrace_container_monitored$mean <- c(ftrace_container_monitored$mean, mean(data$rtt))
    ftrace_container_monitored$median <- c(ftrace_container_monitored$median, median(data$rtt))
    ftrace_container_monitored$sd <- c(ftrace_container_monitored$sd, sd(data$rtt))
    ftrace_container_monitored$len <- c(ftrace_container_monitored$len, length(data$rtt))

  }
  close(con)

  dput(list(
    native_control=native_control,
    container_control=container_control,
    mace_native_monitored=mace_native_monitored,
    mace_container_monitored=mace_container_monitored,
    ftrace_native_monitored=ftrace_native_monitored,
    ftrace_container_monitored=ftrace_container_monitored,
    container_counts=container_counts
  ), file=SAVED_DATA_PATH)
}


#
# Perturbation Diffs
#

mace_native_perturbation_mean <- mace_native_monitored$mean - native_control$mean
mace_native_perturbation_conf <- getConfidence(mace_native_monitored$sd + native_control$sd, min(mace_native_monitored$len, native_control$len))

mace_container_perturbation_mean <- mace_container_monitored$mean - container_control$mean
mace_container_perturbation_conf <- getConfidence(mace_container_monitored$sd + container_control$sd, min(mace_container_monitored$len, container_control$len))

ftrace_native_perturbation_mean <- ftrace_native_monitored$mean - native_control$mean
ftrace_native_perturbation_conf <- getConfidence(ftrace_native_monitored$sd + native_control$sd, min(ftrace_native_monitored$len, native_control$len))

ftrace_container_perturbation_mean <- ftrace_container_monitored$mean - container_control$mean
ftrace_container_perturbation_conf <- getConfidence(ftrace_container_monitored$sd + container_control$sd, min(ftrace_container_monitored$len, container_control$len))

xbnds <- range(container_counts)
ybnds <- c(0, max(ftrace_container_perturbation_mean + ftrace_container_perturbation_conf,
                  mace_container_perturbation_mean + mace_container_perturbation_conf))
pdf(file=paste(data_path, "/perturbations.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("RTT Mean Perturbation (",mu,"s)", sep="")),
     main="")


lines(container_counts, mace_native_perturbation_mean, col="gray", type="l")
drawArrows(container_counts,
           mace_native_perturbation_mean,
           mace_native_perturbation_conf,
           "gray")

lines(container_counts, ftrace_native_perturbation_mean, col="green", type="l")
drawArrows(container_counts,
           ftrace_native_perturbation_mean,
           ftrace_native_perturbation_conf,
           "green")

lines(container_counts, mace_container_perturbation_mean, col="black", type="l")
drawArrows(container_counts,
           mace_container_perturbation_mean,
           mace_container_perturbation_conf,
           "black")

lines(container_counts, ftrace_container_perturbation_mean, col="blue", type="l")
drawArrows(container_counts,
           ftrace_container_perturbation_mean,
           ftrace_container_perturbation_conf,
           "blue")

legend("topleft",
  legend=c("MACE Native", "MACE Container", "ftrace Native", "ftrace Container"),
  col=c("gray", "black", "green", "blue"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()

cat("Done.\n")

