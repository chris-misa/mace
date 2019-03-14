#
# R script for parsing perturbation results and graphing
#

#
# Argument should be directory holding raw data
#
args <- commandArgs(trailingOnly=T)
if (length(args) != 1) {
  stop("Need path to data")
}

options(digits=22)

data_path <- args[1]

#
# Compute rough confidence interval
#
confidence <- 0.90
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
         length=0.05, angle=90, code=3, col=color)
}


#
# Read in data
#
container <- list(
  sys_enter=scan(paste(data_path, "/container_sys_enter.pert", sep="")) / 1000,
  net_dev_start_xmit=scan(paste(data_path, "/container_net_dev_start_xmit.pert", sep="")) / 1000,
  netif_receive_skb=scan(paste(data_path, "/container_netif_receive_skb.pert", sep="")) / 1000,
  sys_exit=scan(paste(data_path, "/container_sys_exit.pert", sep="")) / 1000
)

native <- list(
  sys_enter=scan(paste(data_path, "/native_sys_enter.pert", sep="")) / 1000,
  net_dev_start_xmit=scan(paste(data_path, "/native_net_dev_start_xmit.pert", sep="")) / 1000,
  netif_receive_skb=scan(paste(data_path, "/native_netif_receive_skb.pert", sep="")) / 1000,
  sys_exit=scan(paste(data_path, "/native_sys_exit.pert", sep="")) / 1000
)


data <- cbind(
  c(mean(container$sys_enter),
    mean(container$net_dev_start_xmit),
    mean(container$netif_receive_skb),
    mean(container$sys_exit)),
  c(mean(native$sys_enter),
    mean(native$net_dev_start_xmit),
    mean(native$netif_receive_skb),
    mean(native$sys_exit))
)

error <- c(
  getConfidence(sd(container$sys_enter), length(container$sys_enter)),
  getConfidence(sd(container$net_dev_start_xmit), length(container$net_dev_start_xmit)),
  getConfidence(sd(container$netif_receive_skb), length(container$netif_receive_skb)),
  getConfidence(sd(container$sys_exit), length(container$sys_exit)),

  getConfidence(sd(native$sys_enter), length(native$sys_enter)),
  getConfidence(sd(native$net_dev_start_xmit), length(native$net_dev_start_xmit)),
  getConfidence(sd(native$netif_receive_skb), length(native$netif_receive_skb)),
  getConfidence(sd(native$sys_exit), length(container$sys_exit))
)

ybnds <- c(0, max(error + as.vector(data)))

pdf(file=paste(data_path, "/means.pdf", sep=""))
barCenters <- barplot(data, beside=T, main="",
        xlab="", ylab=expression("Perturbation ("*mu*"s)"),
        ylim=ybnds,
        names.arg=c("Container", "Native"),
        legend=c("sys_enter", "net_dev_start_xmit", "netif_receive_skb", "sys_exit"),
        col=c("darkblue", "red", "orange", "lightblue"))

drawArrows(barCenters, as.vector(data), error, "black")
dev.off()
