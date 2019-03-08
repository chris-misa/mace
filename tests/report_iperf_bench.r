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
native_control <- list(mean=c(), sd=c())
native_monitored <- list(mean=c(), sd=c())
container_control <- list(mean=c(), sd=c())
container_monitored <- list(mean=c(), sd=c())
corrected <- list(mean=c(), sd=c())
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
	native_control$sd <- c(native_control$sd, data$native_control$sd)

	native_monitored$mean <- c(native_monitored$mean, data$native_monitored$mean)
	native_monitored$sd <- c(native_monitored$sd, data$native_monitored$sd)

	container_control$mean <- c(container_control$mean, data$container_control$mean)
	container_control$sd <- c(container_control$sd, data$container_control$sd)

	container_monitored$mean <- c(container_monitored$mean, data$container_monitored$mean)
	container_monitored$sd <- c(container_monitored$sd, data$container_monitored$sd)

	corrected$mean <- c(corrected$mean, data$corrected$mean)
	corrected$sd <- c(corrected$sd, data$corrected$sd)
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
     ylab=expression(paste("RTT (",mu,"s)", sep="")),
     main="")

lines(container_counts, native_control$mean, col="pink", type="l")
lines(container_counts, native_monitored$mean, col="purple", type="l")
lines(container_counts, container_control$mean, col="lightblue", type="l")
lines(container_counts, container_monitored$mean, col="blue", type="l")
lines(container_counts, corrected$mean, col="black", type="l")

legend("bottomright",
  legend=c("native control", "native monitored", "container control", "container monitored", "container corrected"),
  col=c("pink", "purple", "lightblue", "blue", "black"),
  cex=0.8,
  lty=1,
  bg="white")
dev.off()

cat("Done.\n")
