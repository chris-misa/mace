FROM ubuntu:14.04
COPY requests.sh /usr/local/bin/
#RUN ln -s /usr/local/bin/requests.sh # backwards compat
RUN apt-get update && apt-get install -y wget
ENTRYPOINT ["requests.sh"]