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
# Read in data
#
container <- list(
  sys_enter=scan(paste(data_path, "/container_sys_enter.pert", sep="")),
  net_dev_start_xmit=scan(paste(data_path, "/container_net_dev_start_xmit.pert", sep="")),
  netif_receive_skb=scan(paste(data_path, "/container_netif_receive_skb.pert", sep="")),
  sys_exit=scan(paste(data_path, "/container_sys_exit.pert", sep=""))
)

native <- list(
  sys_enter=scan(paste(data_path, "/native_sys_enter.pert", sep="")),
  net_dev_start_xmit=scan(paste(data_path, "/native_net_dev_start_xmit.pert", sep="")),
  netif_receive_skb=scan(paste(data_path, "/native_netif_receive_skb.pert", sep="")),
  sys_exit=scan(paste(data_path, "/native_sys_exit.pert", sep=""))
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

pdf(file=paste(data_path, "/means.pdf", sep=""))
barplot(data, beside=T, main="",
        xlab="", ylab="Perturbation (ns)",
        names.arg=c("Container", "Native"),
        legend=c("sys_enter", "net_dev_start_xmit", "netif_receive_skb", "sys_exit"))
dev.off()
