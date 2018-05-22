# Set this to your local install of clpru
PRU_CGT = /opt/ti/ti-cgt-pru_2.2.1

# Remote execution requires ssh access as root
SSH_HOST = beagle_root

BIN_ARM = arm/epilepsia
BIN_PRU_0 = pru/gen/am335x-pru0-fw
BIN_PRU_1 = pru/gen/am335x-pru1-fw

PINS = P8_46 P8_43 P8_42 P8_28 # PRU 1
PINS += P9_25 P9_27 P9_31 P8_11 # PRU 0

SYSFS = /sys/class/remoteproc

.PHONY:
all: build

build:
	@${MAKE} -C arm
	@PRU_CGT=${PRU_CGT} ${MAKE} -C pru

.PHONY: 
clean:
	@${MAKE} -C arm clean
	@${MAKE} -C pru clean


.PHONY: reboot run justrun upload_arm upload_pru upload reset start stop

# Uploads the three binaries to the beaglebone
upload_pru:
	scp $(BIN_PRU_0) $(BIN_PRU_1) $(SSH_HOST):/root/
	ssh $(SSH_HOST) 'su -c "cp /root/am335x-* /lib/firmware"'

upload_arm:
	scp $(BIN_ARM) $(SSH_HOST):/root/

upload: upload_arm upload_pru

# Stop PRUs
stop:
	ssh $(SSH_HOST) 'su -c "$(foreach pru,1 2,echo stop > $(SYSFS)/remoteproc$(pru)/state;)" 2> /dev/null || true'

# Start PRUs
start:
	ssh $(SSH_HOST) 'su -c "$(foreach pru,1 2,echo start > $(SYSFS)/remoteproc$(pru)/state;)" 2> /dev/null || true'

# Reboot PRUs
reset: stop start

# Start epilepsia on beagleboard
just_run:
	ssh -t -q $(SSH_HOST) /root/$(notdir $(BIN_ARM))

# Run everything
run: upload reset just_run

# Set the pins we use to pruout
config:	
	ssh $(SSH_HOST) 'su -c "$(foreach pin,$(PINS),config-pin -a $(pin) pruout;)' \
	'cd $(SYSFS); echo am335x-pru0-fw > remoteproc1/firmware;echo am335x-pru1-fw > remoteproc2/firmware" || true'
	


