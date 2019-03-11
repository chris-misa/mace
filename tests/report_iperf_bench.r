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
# Begin main work
#


#
# Collect means and sds from all traffic settings
#
native_control <- list(mean=c(), median=c(), sd=c())
native_control_hw <- list(mean=c(), median=c(), sd=c())
native_monitored <- list(mean=c(), median=c(), sd=c())
container_control <- list(mean=c(), median=c(), sd=c())
container_monitored <- list(mean=c(), median=c(), sd=c())
container_corrected <- list(mean=c(), median=c(), sd=c())
native_corrected <- list(mean=c(), median=c(), sd=c())
native_pert_areas <- c()
container_pert_areas <- c()
container_counts <- c()

con <- file(paste(data_path, "/manifest", sep=""), "r")
while (T) {
	line <- readLines(con, n=1)
	if (length(line) == 0) {
		break
	}
	container_counts <- c(container_counts, as.numeric(line))

	data <- dget(paste(data_path, "/", line, "/summary", sep=""))
	
	native_control$mean <- c(native_control$mean, data$native_control$mean)
	native_control$median <- c(native_control$median, data$native_control$median)
	native_control$sd <- c(native_control$sd, data$native_control$sd)

	native_control_hw$mean <- c(native_control_hw$mean, data$native_control_hw$mean)
	native_control_hw$median <- c(native_control_hw$median, data$native_control_hw$median)
	native_control_hw$sd <- c(native_control_hw$sd, data$native_control_hw$sd)

	native_monitored$mean <- c(native_monitored$mean, data$native_monitored$mean)
	native_monitored$median <- c(native_monitored$median, data$native_monitored$median)
	native_monitored$sd <- c(native_monitored$sd, data$native_monitored$sd)

	container_control$mean <- c(container_control$mean, data$container_control$mean)
	container_control$median <- c(container_control$median, data$container_control$median)
	container_control$sd <- c(container_control$sd, data$container_control$sd)

	container_monitored$mean <- c(container_monitored$mean, data$container_monitored$mean)
	container_monitored$median <- c(container_monitored$median, data$container_monitored$median)
	container_monitored$sd <- c(container_monitored$sd, data$container_monitored$sd)

	container_corrected$mean <- c(container_corrected$mean, data$container_corrected$mean)
	container_corrected$median <- c(container_corrected$median, data$container_corrected$median)
	container_corrected$sd <- c(container_corrected$sd, data$container_corrected$sd)

	native_corrected$mean <- c(native_corrected$mean, data$native_corrected$mean)
	native_corrected$median <- c(native_corrected$median, data$native_corrected$median)
	native_corrected$sd <- c(native_corrected$sd, data$native_corrected$sd)

  native_pert_areas <- c(native_pert_areas , data$native_pert_area)
  container_pert_areas <- c(container_pert_areas, data$container_pert_area)
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
# Plot perturbation areas
#
xbnds <- range(container_counts)
ybnds <- c(0, max(container_pert_areas))
pdf(file=paste(data_path, "/perturbation_areas.pdf", sep=""))
plot(0, type="n", ylim=ybnds, xlim=xbnds,
     xlab="Number of traffic flows",
     ylab=expression(paste("Area Between Control and Monitored (",mu,"s^2)", sep="")),
     main="")

lines(container_counts, native_pert_areas, col="red", type="l")
lines(container_counts, container_pert_areas, col="blue", type="l")

legend("topleft",
  legend=c("native", "container"),
  col=c("red", "blue"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()


cat("Done.\n")

