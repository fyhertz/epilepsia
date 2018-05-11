FROM debian:buster-slim

RUN apt-get update && apt-get install -y wget make g++-arm-linux-gnueabihf

# clpru installer is a 32 bits executable
RUN apt-get install -y lib32z1

# Install clpru compiler, needed to compile the firmware of the PRUs
RUN wget http://software-dl.ti.com/codegen/esd/cgt_public_sw/PRU/2.2.1/ti_cgt_pru_2.2.1_linux_installer_x86.bin -O clpru.bin
RUN chmod +x clpru.bin
RUN ./clpru.bin --mode unattended --prefix /opt/ti

WORKDIR /opt/epilepsia
ENTRYPOINT ["make"]
