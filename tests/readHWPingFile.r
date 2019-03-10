#
# Read and parse a dump from ping
#
# Converts reported RTT miliseconds to microseconds
#
readHWPingFile <- function(filePath) {
  con <- file(filePath, "r")
  timestamps <- c()
  rtts <- c()
  linePattern <- "\\[([0-9\\.]+)\\] HW RTT ([0-9\\.]+)"
  while (T) {
    line <- readLines(con, n=1)
    if (length(line) == 0) {
      break
    }
    matches <- grep(linePattern, line, value=T)

    if (length(matches) != 0) {
      ts <- as.numeric(sub(linePattern, "\\1", matches))
      rtt <- as.numeric(sub(linePattern, "\\2", matches)) * 1000000

      timestamps <- c(timestamps, ts)
      rtts <- c(rtts, rtt)
    }
  }
  close(con)
  data.frame(rtt=rtts, ts=timestamps)
}
