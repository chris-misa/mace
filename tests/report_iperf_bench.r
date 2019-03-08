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
native_control <- list(mean=list(), sd=list())
native_monitored <- list(mean=list(), sd=list())
container_control <- list(mean=list(), sd=list())
container_monitored <- list(mean=list(), sd=list())
corrected <- list(mean=list(), sd=list())

con <- file(paste(data_path, "/manifest", sep=""), "r")
while (T) {
	line <- readLines(con, n=1)
	if (length(line) == 0) {
		break
	}

	data <- dget(paste(data_path, "/", line, "/summary", sep=""))
	
	native_control$mean <- append(native_control$mean, data$native_control$mean)
	native_control$sd <- append(native_control$sd, data$native_control$sd)

	native_monitored$mean <- append(native_monitored$mean, data$native_monitored$mean)
	native_monitored$sd <- append(native_monitored$sd, data$native_monitored$sd)

	container_control$mean <- append(container_control$mean, data$container_control$mean)
	container_control$sd <- append(container_control$sd, data$container_control$sd)

	container_monitored$mean <- append(container_monitored$mean, data$container_monitored$mean)
	container_monitored$sd <- append(container_monitored$sd, data$container_monitored$sd)

	corrected$mean <- append(corrected$mean, data$corrected$mean)
	corrected$sd <- append(corrected$sd, data$corrected$sd)
}
close(con)

print(native_control)
print(native_monitored)
print(container_control)
print(container_monitored)
print(corrected)

cat("Done.\n")
