FROM debian:buster-slim
LABEL maintainer="fyhertz@gmail.com"

# Install package build dependencies
# clpru installer is a 32 bits executable: lib32z1 needed
RUN dpkg --add-architecture armhf \
 && apt-get -yq update \
 && apt-get -yq --no-install-suggests --no-install-recommends install \
    dh-systemd \
    build-essential \
    devscripts \
    debhelper \
    wget lib32z1 \
    make \
    g++-arm-linux-gnueabihf \
    libc6:armhf \
 && apt-get clean \
 && rm -rf /var/lib/apt/lists/*

# Install clpru compiler, needed to compile the firmware of the PRUs
RUN wget http://software-dl.ti.com/codegen/esd/cgt_public_sw/PRU/2.2.1/ti_cgt_pru_2.2.1_linux_installer_x86.bin -O clpru.bin \
 && chmod +x clpru.bin \
 && ./clpru.bin --mode unattended --prefix /opt/ti \
 && rm clpru.bin

COPY entry.sh /
VOLUME /data
WORKDIR /data
ENTRYPOINT ["/entry.sh"]
