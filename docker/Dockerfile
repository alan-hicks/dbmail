# DBMail Dockerfile
# Copyright (c) 2020-2025 Alan Hicks, Persistent Objects Ltd support@p-o.co.uk
# LICENSE GPL2+

FROM ubuntu:24.04 AS build_base
LABEL org.opencontainers.image.authors="ahicks@p-o.co.uk" \
      org.opencontainers.image.url="https://dbmail.org/docs/docker-images/" \
      org.opencontainers.image.source="https://github.com/dbmail/dbmail/tree/master/docker" \
      org.opencontainers.image.licenses="GPL-2.0+" \
      org.opencontainers.image.title="DBMail-build-environment" \
      org.opencontainers.image.description="This image contains everything to build dbmail"

RUN apt update && \
    apt install -y build-essential git libgmime-3.0-dev libzdb-dev libldap2-dev \
    libmhash-dev libevent-dev libssl-dev libsieve2-dev

FROM build_base AS build
LABEL org.opencontainers.image.authors="ahicks@p-o.co.uk" \
      org.opencontainers.image.url="https://dbmail.org/docs/docker-images/" \
      org.opencontainers.image.source="https://github.com/dbmail/dbmail/tree/main/docker" \
      org.opencontainers.image.licenses="GPL-2.0+" \
      org.opencontainers.image.title="DBMail-build" \
      org.opencontainers.image.description="This image builds dbmail"

RUN apt update && \
    apt install -y libgmime-3.0-0 libzdb11 libldap-common \
    libcurl4-openssl-dev check \
    libmhash2 libevent-2.1-7 libevent-pthreads-2.1-7 libssl3 libsieve2-1

RUN mkdir -p /app/dbmail

RUN cd /app && \
    git clone --branch 3.5.x https://github.com/dbmail/dbmail.git && \
    cd /app/dbmail && \
    ./configure --with-ldap=/usr --with-sieve=/usr/lib/x86_64-linux-gnu/ --with-check && \
    make && make check && make install

WORKDIR /

COPY --chown=nobody:nogroup dbmail-imapd.pid /var/run/dbmail/
COPY --chown=nobody:nogroup dbmail-lmtpd.pid /var/run/dbmail/
COPY --chown=nobody:nogroup dbmail-sieved.pid /var/run/dbmail/
COPY --chown=nobody:nogroup dbmail.err /var/log/dbmail/
COPY --chown=nobody:nogroup dbmail.log /var/log/dbmail/

EXPOSE 143/tcp
EXPOSE 993/tcp
EXPOSE 24/tcp
EXPOSE 4190/tcp

CMD ["/usr/local/sbin/dbmail-imapd", "-D", "-v"]
