#!/bin/bash

#
# Install dependencies and do a little configuring
#

apt-get update

apt-get install -y r-base

# Set up docker
apt-get install -y \
  apt-transport-https \
  ca-certificates \
  curl \
  software-properties-common

curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -

add-apt-repository \
  "deb [arch=amd64] https://download.docker.com/linux/ubuntu \
  $(lsb_release -cs) \
  stable"

apt-get update
apt-get install -y docker-ce

# Get docker-compose (from https://docs.docker.com/compose/install/#install-compose)
sudo curl -L "https://github.com/docker/compose/releases/download/1.23.2/docker-compose-$(uname -s)-$(uname -m)" -o /usr/local/bin/docker-compose
sudo chmod +x /usr/local/bin/docker-compose


#
# Eject app armor which gets in the way of
# automatically doing thing in containers from native space
#
# use `aa-status` to check that this went through
#
systemctl disable apparmor.service --now
service apparmor teardown

