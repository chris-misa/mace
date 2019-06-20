#
# Cloud lab disk images do not preserve users or groups by default
# so we have to recreate the docker group
#
# Run this as sudo!
#

addgroup docker
systemctl start docker.socket
systemctl start docker
