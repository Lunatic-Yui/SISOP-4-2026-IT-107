FROM ubuntu:latest

ENV DEBIAN_FRONTEND=noninteractive

RUN apt-get update && apt-get install -y samba && apt-get clean

WORKDIR /app

COPY . /app/

RUN cp /app/smb.conf /etc/samba/smb.conf

ENTRYPOINT ["/bin/bash", "/app/entrypoint.sh"]