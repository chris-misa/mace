#
# R script to gather data from iperf traffic pair bench mark
#
# Assumes report_one_shot.r has already been run for each particular traffic setting
#
# Plots RTT against number of server / client iperf pairs
#

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
         length=0.01, angle=90, code=3, col=color)
}


#
# Begin main work
#


#
# Collect means and sds from all traffic settings
#
native_control <- list(mean=c(), median=c(), sd=c(), len=c())
native_control_hw <- list(mean=c(), median=c(), sd=c(), len=c())
native_control_socket <- list(mean=c(), median=c(), sd=c(), len=c())
native_monitored <- list(mean=c(), median=c(), sd=c(), len=c())
container_control <- list(mean=c(), median=c(), sd=c(), len=c())
container_monitored <- list(mean=c(), median=c(), sd=c(), len=c())
container_corrected <- list(mean=c(), median=c(), sd=c(), len=c(), drop=c())
native_corrected <- list(mean=c(), median=c(), sd=c(), len=c(), drop=c())
native_pert_areas <- c()
container_pert_areas <- c()
native_corrected_areas <- c()
container_corrected_areas <- c()


container_counts <- c()

con <- file(paste(data_path, "/manifest", sep=""), "r")
while (T) {
	line <- readLines(con, n=1)
	if (length(line) == 0) {
		break
	}

	summaryPath <- paste(data_path, "/", line, "/summary", sep="")
	if (!file.exists(summaryPath)) {
		next
	}
	data <- dget(summaryPath)

	container_counts <- c(container_counts, as.numeric(line))
	
	native_control$mean <- c(native_control$mean, data$native_control$mean)
	native_control$median <- c(native_control$median, data$native_control$median)
	native_control$sd <- c(native_control$sd, data$native_control$sd)
	native_control$len <- c(native_control$len, data$native_control$len)

	native_control_hw$mean <- c(native_control_hw$mean, data$native_control_hw$mean)
	native_control_hw$median <- c(native_control_hw$median, data$native_control_hw$median)
	native_control_hw$sd <- c(native_control_hw$sd, data$native_control_hw$sd)
	native_control_hw$len <- c(native_control_hw$len, data$native_control_hw$len)

	native_control_socket$mean <- c(native_control_socket$mean, data$native_control_socket$mean)
	native_control_socket$median <- c(native_control_socket$median, data$native_control_socket$median)
	native_control_socket$sd <- c(native_control_socket$sd, data$native_control_socket$sd)
	native_control_socket$len <- c(native_control_socket$len, data$native_control_socket$len)

	native_monitored$mean <- c(native_monitored$mean, data$native_monitored$mean)
	native_monitored$median <- c(native_monitored$median, data$native_monitored$median)
	native_monitored$sd <- c(native_monitored$sd, data$native_monitored$sd)
	native_monitored$len <- c(native_monitored$len, data$native_monitored$len)

	container_control$mean <- c(container_control$mean, data$container_control$mean)
	container_control$median <- c(container_control$median, data$container_control$median)
	container_control$sd <- c(container_control$sd, data$container_control$sd)
	container_control$len <- c(container_control$len, data$container_control$len)

	container_monitored$mean <- c(container_monitored$mean, data$container_monitored$mean)
	container_monitored$median <- c(container_monitored$median, data$container_monitored$median)
	container_monitored$sd <- c(container_monitored$sd, data$container_monitored$sd)
	container_monitored$len <- c(container_monitored$len, data$container_monitored$len)

	container_corrected$mean <- c(container_corrected$mean, data$container_corrected$mean)
	container_corrected$median <- c(container_corrected$median, data$container_corrected$median)
	container_corrected$sd <- c(container_corrected$sd, data$container_corrected$sd)
	container_corrected$len <- c(container_corrected$len, data$container_corrected$len)
  container_corrected$drop <- c(container_corrected$drop, data$container_corrected$drop)

	native_corrected$mean <- c(native_corrected$mean, data$native_corrected$mean)
	native_corrected$median <- c(native_corrected$median, data$native_corrected$median)
	native_corrected$sd <- c(native_corrected$sd, data$native_corrected$sd)
  native_corrected$len <- c(native_corrected$len, data$native_corrected$len)
  native_corrected$drop <- c(native_corrected$drop, data$native_corrected$drop)

  native_pert_areas <- c(native_pert_areas , data$native_pert_area)
  container_pert_areas <- c(container_pert_areas, data$container_pert_area)

  native_corrected_areas <- c(native_corrected_areas, data$native_corrected_area)
  container_corrected_areas <- c(container_corrected_areas, data$container_corrected_area)
}
close(con)

#
# Draw line plot of means
#
xbnds <- range(container_counts)
ybnds <- c(0, max(container_monitored$mean))
pdf(file=paste(data_path, "/means.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("Mean RTT (",mu,"s)", sep="")),
     main="")

lines(container_counts, native_control_hw$mean, col="green", type="l")
lines(container_counts, native_control$mean, col="pink", type="l")
lines(container_counts, native_monitored$mean, col="purple", type="l")
lines(container_counts, container_control$mean, col="lightblue", type="l")
lines(container_counts, container_monitored$mean, col="blue", type="l")
lines(container_counts, container_corrected$mean, col="black", type="l")
lines(container_counts, native_corrected$mean, col="gray", type="l")

legend("topleft",
  legend=c("hardware", "native control", "native monitored", "container control", "container monitored", "container corrected", "native corrected"),
  col=c("green", "pink", "purple", "lightblue", "blue", "black", "gray"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()

#
# Draw line plot of medians
#
xbnds <- range(container_counts)
ybnds <- c(0, max(container_monitored$median))
pdf(file=paste(data_path, "/medians.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("Median RTT (",mu,"s)", sep="")),
     main="")

lines(container_counts, native_control_hw$median, col="green", type="l")
lines(container_counts, native_control$median, col="pink", type="l")
lines(container_counts, native_monitored$median, col="purple", type="l")
lines(container_counts, container_control$median, col="lightblue", type="l")
lines(container_counts, container_monitored$median, col="blue", type="l")
lines(container_counts, container_corrected$median, col="black", type="l")
lines(container_counts, native_corrected$median, col="gray", type="l")

legend("topleft",
  legend=c("hardware", "native control", "native monitored", "container control", "container monitored", "container corrected", "native corrected"),
  col=c("green", "pink", "purple", "lightblue", "blue", "black", "gray"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()


#
# Draw line plot of deviations
#
xbnds <- range(container_counts)
ybnds <- c(0, max(container_monitored$sd))
pdf(file=paste(data_path, "/sds.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("RTT Std. Deviation (",mu,"s)", sep="")),
     main="")

lines(container_counts, native_control_hw$sd, col="green", type="l")
lines(container_counts, native_control$sd, col="pink", type="l")
lines(container_counts, native_monitored$sd, col="purple", type="l")
lines(container_counts, container_control$sd, col="lightblue", type="l")
lines(container_counts, container_monitored$sd, col="blue", type="l")
lines(container_counts, container_corrected$sd, col="black", type="l")
lines(container_counts, native_corrected$sd, col="gray", type="l")

legend("topleft",
  legend=c("hardware", "native control", "native monitored", "container control", "container monitored", "container corrected", "native corrected"),
  col=c("green", "pink", "purple", "lightblue", "blue", "black", "gray"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()


#
# Sub plots for motivating presentation
#

#
# Draw line plot of medians for just un-monitored traces
#
xbnds <- range(container_counts)
ybnds <- c(0, max(container_control$median))
pdf(file=paste(data_path, "/medians_unmonitored.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("Median RTT (",mu,"s)", sep="")),
     main="")

lines(container_counts, native_control_hw$median, col="green", type="l")
lines(container_counts, native_control$median, col="red", type="l")
lines(container_counts, container_control$median, col="blue", type="l")

legend("topleft",
  legend=c("hardware", "native", "container"),
  col=c("green", "red", "blue"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()

#
# Draw line plot of sds for just un-monitored traces
#
xbnds <- range(container_counts)
ybnds <- c(0, max(container_control$sd))
pdf(file=paste(data_path, "/sds_unmonitored.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("RTT Std. Deviation (",mu,"s)", sep="")),
     main="")

lines(container_counts, native_control_hw$sd, col="green", type="l")
lines(container_counts, native_control$sd, col="red", type="l")
lines(container_counts, container_control$sd, col="blue", type="l")

legend("topleft",
  legend=c("hardware", "native", "container"),
  col=c("green", "red", "blue"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()

#
# Line plot of differences to hardware mean
#

native_control_socket_conf <- getConfidence(native_control_socket$sd, native_control_socket$len)
native_corrected_conf <- getConfidence(native_corrected$sd, native_corrected$len)

container_control_conf <- getConfidence(container_control$sd, container_control$len)
container_corrected_conf <- getConfidence(container_corrected$sd, container_corrected$len)

xbnds <- range(container_counts)
ybnds <- c(0, max(container_control$mean - native_control_hw$mean) + 100)
pdf(file=paste(data_path, "/mean_diffs.pdf", sep=""), width=4, height=4)
par(mar=c(2.5, 2.5, 1, 1), mgp=c(1.1,0.2,0), tck=-0.02)
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("RTT Mean Difference (",mu,"s)", sep="")),
     main="")
grid()

lines(container_counts, container_control$mean - native_control_hw$mean, col="blue", type="l")
drawArrows(container_counts,
           container_control$mean - native_control_hw$mean,
           container_control_conf,
           "blue")
lines(container_counts, native_control_socket$mean - native_control_hw$mean, col="green", type="l")
drawArrows(container_counts,
           native_control_socket$mean - native_control_hw$mean,
           native_control_socket_conf,
           "green")
# lines(container_counts, native_corrected$mean - native_control_hw$mean, col="gray", type="l")
# drawArrows(container_counts,
#            native_corrected$mean - native_control_hw$mean,
#            native_corrected_conf,
#            "gray")
lines(container_counts, container_corrected$mean - native_control_hw$mean, col="black", type="l")
drawArrows(container_counts,
           container_corrected$mean - native_control_hw$mean,
           container_corrected_conf,
           "black")

legend("topleft",
  #legend=c("Native Reference", "Native Corrected", "Container Reference", "Container Corrected"),
    # Ram's changes:
  legend=c("Ground-truth RTT",  "Container - Baseline RTT", "Container - In-kernel Filtered RTT"),
  col=c("green", "blue", "black"),
  lty=1,
  cex=1,
  bg="white")
dev.off()


#
# Perturbation Diffs
#

native_perturbation_mean <- native_monitored$mean - native_control$mean
native_perturbation_conf <- getConfidence(native_monitored$sd + native_control$sd, min(native_monitored$len, native_control$len))


container_perturbation_mean <- container_monitored$mean - container_control$mean
container_perturbation_conf <- getConfidence(container_monitored$sd + container_control$sd, min(container_monitored$len, container_control$len))

xbnds <- range(container_counts)
ybnds <- c(0, max(container_perturbation_mean + container_perturbation_conf))
pdf(file=paste(data_path, "/perturbation_diffs.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("RTT Mean Perturbation (",mu,"s)", sep="")),
     main="")


lines(container_counts, native_perturbation_mean, col="gray", type="l")
drawArrows(container_counts,
           native_perturbation_mean,
           native_perturbation_conf,
           "gray")
lines(container_counts, container_perturbation_mean, col="black", type="l")
drawArrows(container_counts,
           container_perturbation_mean,
           container_perturbation_conf,
           "black")

legend("topleft",
  legend=c("Native", "Container"),
  col=c("gray", "black"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()




#
# Line plot of differences to hardware median
#
xbnds <- range(container_counts)
ybnds <- c(0, max(native_control_socket$median - native_control_hw$median))
pdf(file=paste(data_path, "/median_diffs.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("RTT Median Difference (",mu,"s)", sep="")),
     main="")

lines(container_counts, native_control_socket$median - native_control_hw$median, col="darkgreen", type="l")
lines(container_counts, native_corrected$median - native_control_hw$median, col="gray", type="l")
lines(container_counts, container_corrected$median - native_control_hw$median, col="black", type="l")

legend("topleft",
  legend=c("Native Reference", "Native Corrected", "Container Corrected"),
  col=c("darkgreen", "gray", "black"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()

#
# Draw packet drop lines
#

container_drops <- 100 * container_corrected$len / container_monitored$len
native_drops <- 100 * native_corrected$len / native_monitored$len

xbnds <- range(container_counts)
ybnds <- range(container_drops, native_drops)
pdf(file=paste(data_path, "/packet_drop.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab="Percent of packets",
     main="")

lines(container_counts, native_drops, col="gray", type="l")
lines(container_counts, container_drops, col="black", type="l")

legend("topright",
  legend=c("Native", "Container"),
  col=c("gray", "black"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()



cat("Done.\n")

