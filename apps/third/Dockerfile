FROM ubuntu:latest

RUN apt-get update

RUN apt-get install -y nodejs npm git git-core
RUN update-alternatives --install /usr/bin/node node /usr/bin/nodejs 10

RUN apt-get install -y curl libstdc++6

RUN mkdir -p /usr/local/lib
ENV LD_LIBRARY_PATH /usr/local/lib/:$LD_LIBRARY_PATH

RUN mkdir -p /usr/src/app
WORKDIR /usr/src/app

COPY package.json .
RUN npm install

COPY trace.so /usr/local/lib/.
COPY libprotobuf.so.13 /usr/local/lib/.
COPY server.js .

EXPOSE 3131
ENV LD_PRELOAD /usr/local/lib/trace.so
CMD ["npm", "start"]
