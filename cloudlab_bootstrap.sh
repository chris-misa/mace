#
# Run this before anything else if using CloudLab disk image.
#
# CloudLab disk images do not preserve users or groups by default
# so we have to recreate the docker group and restart docker
#
# Run this as sudo!
#

addgroup docker
systemctl start docker.socket
systemctl start docker
