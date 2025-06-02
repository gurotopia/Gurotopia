FROM ubuntu:latest

RUN apt update && apt install -y build-essential gcc make

COPY . .

RUN make -j$(nproc)

CMD ["/main.out"]