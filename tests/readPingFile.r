#
# Read and parse a dump from ping
#
# Converts reported RTT miliseconds to microseconds
#
readPingFile <- function(filePath) {
  con <- file(filePath, "r")
  timestamps <- c()
  seqs <- c()
  rtts <- c()
  linePattern <- "\\[([0-9\\.]+)\\] .* icmp_seq=([0-9]+) .* time=([0-9\\.]+) ms"
  while (T) {
    line <- readLines(con, n=1)
    if (length(line) == 0) {
      break
    }
    matches <- grep(linePattern, line, value=T)

    if (length(matches) != 0) {
      ts <- as.numeric(sub(linePattern, "\\1", matches))
      seq <- as.numeric(sub(linePattern, "\\2", matches))
      rtt <- as.numeric(sub(linePattern, "\\3", matches)) * 1000

      timestamps <- c(timestamps, ts)
      rtts <- c(rtts, rtt)
      seqs <- c(seqs, seq)
    }
  }
  close(con)
  data.frame(rtt=rtts, ts=timestamps, seq=seqs)
}
