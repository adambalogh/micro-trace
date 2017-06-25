FROM ubuntu:latest

RUN \
  apt-get update && \
  apt-get install -y python python-dev python-pip python-virtualenv && \
  rm -rf /var/lib/apt/lists/*

RUN apt-get update && apt-get install -y libstdc++6

RUN mkdir -p /usr/local/lib
ENV LD_LIBRARY_PATH /usr/local/lib/:$LD_LIBRARY_PATH

RUN mkdir -p /usr/src/app
WORKDIR /usr/src/app

COPY requirements.txt .
RUN pip install --no-cache-dir -r requirements.txt

RUN mkdir -p /var/log/microtrace
COPY libtrace.so /usr/local/lib/.
COPY libprotobuf.so.13 /usr/local/lib/.
COPY server.py .

EXPOSE 5001
ENV LD_PRELOAD /usr/local/lib/libtrace.so
CMD ["python", "server.py"]
