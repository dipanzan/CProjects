#!/bin/bash
git clone https://github.com/eswak/dvfs-linux-kernel.git
cd dvfs-linux-kernel/
make
make install
make set