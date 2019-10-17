#
# Read and parse a dump from ping
#
# Converts reported RTT miliseconds to microseconds
#
readNetUsageFile <- function(filePath) {
    con <- file(filePath, "r")
    timestamps <- c()
    rx_bps <- c()
    rx_pps <- c()
    tx_bps <- c()
    tx_pps <- c()
    
    linePattern <- "\\[([0-9\\.]+)\\] *rx_bps: *([0-9\\.]+) *rx_pps: *([0-9\\.]+) *tx_bps: *([0-9\\.]+) *tx_pps: *([0-9\\.]+)"
    while (T) {
        line <- readLines(con, n=1)
        if (length(line) == 0) {
            break
        }

        matches <- grep(linePattern, line, value=T)

        if (length(matches) != 0) {
            ts <- as.numeric(sub(linePattern, "\\1", matches))

            timestamps <- c(timestamps, ts)
            rx_bps <- c(rx_bps, as.numeric(sub(linePattern, "\\2", matches)))
            rx_pps <- c(rx_pps, as.numeric(sub(linePattern, "\\3", matches)))
            tx_bps <- c(tx_bps, as.numeric(sub(linePattern, "\\4", matches)))
            tx_pps <- c(tx_pps, as.numeric(sub(linePattern, "\\5", matches)))
        }
    }
    close(con)
    data.frame(ts=timestamps, rx_bps=rx_bps, rx_pps=rx_pps, tx_bps=tx_bps, tx_pps=tx_pps)
}
