FROM gcc:11.2.0

RUN set -ex; \
  apt-get update; \
  apt-get install libjson-c-dev -y

COPY . /usr/src/myapp
WORKDIR /usr/src/myapp
RUN make

CMD ["./CPUMiner"]
