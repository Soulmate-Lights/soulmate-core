FROM espressif/idf:v3.3.4 as esp
RUN apt-get update \
  && apt-get install -y curl \
  && apt-get -y autoclean

RUN curl -sL https://deb.nodesource.com/setup_12.x | bash -
RUN apt-get install -y nodejs
RUN npm install -g yarn --unsafe-perm
RUN yarn global add nodemon

COPY . /project
WORKDIR /project
ENV SOULMATE_PATH=/project
ENV IDF_PATH=/opt/esp/idf
RUN mkdir /project/output
RUN rm -rf /project/builder/build
RUN yarn
EXPOSE 8081
CMD ["node", "index.js"]
