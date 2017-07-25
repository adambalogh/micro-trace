FROM node:latest

COPY package.json .
RUN npm install

COPY Makefile .
ADD gen-nodejs ./gen-nodejs
COPY server.js .

CMD ["npm", "start"]
