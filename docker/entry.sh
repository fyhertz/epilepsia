#!/usr/bin/env bash

dpkg-buildpackage -us -uc --host-arch armhf -d
cp ../*.deb .