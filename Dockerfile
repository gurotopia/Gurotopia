FROM ubuntu:latest

RUN apt update && apt install -y build-essential libssl-dev libsqlite3-dev

COPY . .

RUN make -j$(nproc)

CMD ["/main.out"]