#
# Read and parse a mace latency_ns dump
#
# Converts reported nanoseconds to microseconds
#
readLatencyFile <- function(filePath) {
  con <- file(filePath, "r")
  egress <- c()
  egress_timestamps <- c()
  egress_seqs <- c()
  ingress <- c()
  ingress_timestamps <- c()
  ingress_seqs <- c()
  linePattern <- "\\[([0-9\\.]+)\\] \\(([0-9]+)\\) (ingress|egress): ([0-9]+)"
  while (T) {
    line <- readLines(con, n=1)
    if (length(line) == 0) {
      break
    }
    matches <- grep(linePattern, line, value = T)
    if (length(matches) != 0) {
      ts <- as.numeric(sub(linePattern, "\\1", matches))
      seq <- as.numeric(sub(linePattern, "\\2", matches))
      dir <- sub(linePattern, "\\3", matches)
      lat <- as.numeric(sub(linePattern, "\\4", matches)) / 1000
      if (dir == "egress") {
        egress <- c(egress, lat)
        egress_timestamps <- c(egress_timestamps, ts)
        egress_seqs <- c(egress_seqs, seq)
      } else if (dir == "ingress") {
        ingress <- c(ingress, lat)
        ingress_timestamps <- c(ingress_timestamps, ts)
        ingress_seqs <- c(ingress_seqs, seq)
      }
    }
  }
  close(con)
  egress <- data.frame(latency=egress, ts=egress_timestamps, seq=egress_seqs)
  ingress <- data.frame(latency=ingress, ts=ingress_timestamps, seq=ingress_seqs)
  list(egress=egress, ingress=ingress)
}
