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
  sys_exit=scan(paste(data_path, "/container_sys_exit.pert", sep="")) / 1000,
  register_entry=scan(paste(data_path, "/container_register_entry.pert", sep="")) / 1000,
  register_exit=scan(paste(data_path, "/container_register_exit.pert", sep="")) / 1000,
  push_event=scan(paste(data_path, "/container_push_event.pert", sep="")) / 1000
)

native <- list(
  sys_enter=scan(paste(data_path, "/native_sys_enter.pert", sep="")) / 1000,
  net_dev_start_xmit=scan(paste(data_path, "/native_net_dev_start_xmit.pert", sep="")) / 1000,
  netif_receive_skb=scan(paste(data_path, "/native_netif_receive_skb.pert", sep="")) / 1000,
  sys_exit=scan(paste(data_path, "/native_sys_exit.pert", sep="")) / 1000,
  register_entry=scan(paste(data_path, "/native_register_entry.pert", sep="")) / 1000,
  register_exit=scan(paste(data_path, "/native_register_exit.pert", sep="")) / 1000,
  push_event=scan(paste(data_path, "/native_push_event.pert", sep="")) / 1000
)


data <- cbind(
  c(mean(container$sys_enter),
    mean(container$net_dev_start_xmit),
    mean(container$netif_receive_skb),
    mean(container$sys_exit),
    mean(container$register_entry),
    mean(container$register_exit),
    mean(container$push_event)
  ),
  c(mean(native$sys_enter),
    mean(native$net_dev_start_xmit),
    mean(native$netif_receive_skb),
    mean(native$sys_exit),
    mean(native$register_entry),
    mean(native$register_exit),
    mean(native$push_event)
  )
)

error <- c(
  getConfidence(sd(container$sys_enter), length(container$sys_enter)),
  getConfidence(sd(container$net_dev_start_xmit), length(container$net_dev_start_xmit)),
  getConfidence(sd(container$netif_receive_skb), length(container$netif_receive_skb)),
  getConfidence(sd(container$sys_exit), length(container$sys_exit)),
  getConfidence(sd(container$register_entry), length(container$register_entry)),
  getConfidence(sd(container$register_exit), length(container$register_exit)),
  getConfidence(sd(container$push_event), length(container$push_event)),

  getConfidence(sd(native$sys_enter), length(native$sys_enter)),
  getConfidence(sd(native$net_dev_start_xmit), length(native$net_dev_start_xmit)),
  getConfidence(sd(native$netif_receive_skb), length(native$netif_receive_skb)),
  getConfidence(sd(native$sys_exit), length(container$sys_exit)),
  getConfidence(sd(native$register_entry), length(native$register_entry)),
  getConfidence(sd(native$register_exit), length(native$register_exit)),
  getConfidence(sd(native$push_event), length(native$push_event))
)

#ybnds <- c(0, max(error + as.vector(data)))
ybnds <- c(0, 1)

pdf(file=paste(data_path, "/means.pdf", sep=""))
barCenters <- barplot(data, beside=T, main="",
        xlab="", ylab=expression("Perturbation ("*mu*"s)"),
        ylim=ybnds,
        names.arg=c("Container", "Native"),
        args.legend=list(y=1.1, ncol=2),
        space=rep(c(1, 0, 0, 0, 0.3, 0, 0.3), 2),
        legend=c("sys_enter", "net_dev_start_xmit", "netif_receive_skb", "sys_exit", "register_entry", "register_exit", "push_event"),
        col=c("darkblue", "red", "orange", "lightblue", "pink", "purple", "green"))

drawArrows(barCenters, as.vector(data), error, "black")
dev.off()
