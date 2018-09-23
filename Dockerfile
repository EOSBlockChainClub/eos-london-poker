FROM ubuntu

RUN apt-get update
RUN apt-get install -y curl gnupg apt-transport-https ca-certificates software-properties-common

# yarn
RUN curl -sS https://dl.yarnpkg.com/debian/pubkey.gpg | apt-key add -
RUN echo "deb https://dl.yarnpkg.com/debian/ stable main" | tee /etc/apt/sources.list.d/yarn.list

# docker
RUN curl -fsSL https://download.docker.com/linux/ubuntu/gpg | apt-key add -
RUN add-apt-repository "deb [arch=amd64] https://download.docker.com/linux/ubuntu $(lsb_release -cs) stable"

RUN apt-get update
RUN apt-get install -y git yarn docker-ce
RUN mkdir /home/eos
RUN yarn global add npm

WORKDIR /home/eos

RUN git clone https://github.com/EOSIO/eosio-project-boilerplate-simple.git .

RUN rm -rf "./eosio_docker/data"
RUN mkdir -p "./eosio_docker/data"

RUN docker run hello-world

RUN ./first_time_setup.sh