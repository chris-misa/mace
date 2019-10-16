FROM ubuntu:18.04
#RUN ln -s /usr/local/bin/requests.sh # backwards compat
RUN apt-get update && apt-get install -y wget python3-pip
RUN pip3 install numpy
COPY requests.sh /usr/local/bin/
COPY exponential.py /usr/local/bin/ 
ENTRYPOINT ["requests.sh"]
