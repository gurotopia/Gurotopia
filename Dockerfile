FROM ubuntu:latest

RUN apt update && apt install -y build-essential gcc-14 make

COPY . .

RUN make -j$(nproc)

CMD ["/main.out"]