#!/bin/bash
set -x 
set -e
chown -R root.root /tmp/NPB/
chmod 755 -R /tmp/NPB/


dpkgInstallURL() {
  local url="${1}"

  TEMP_DEB="$(mktemp)"
  wget -O "$TEMP_DEB" "$url"
  dpkg -i "$TEMP_DEB"
  rm -f "$TEMP_DEB"
}



# Install Deps
apt-get update -y && apt-get install csh m4 libpng-dev libnetcdff-dev netcdf-bin ncl-ncarg libnetcdf-dev -y
dpkgInstallURL http://ftp.us.debian.org/debian/pool/main/libj/libjpeg8/libjpeg8_8d-1+deb7u1_amd64.deb
dpkgInstallURL http://security.debian.org/debian-security/pool/updates/main/j/jasper/libjasper1_1.900.1-13+deb7u6_amd64.deb
dpkgInstallURL http://security.debian.org/debian-security/pool/updates/main/j/jasper/libjasper-dev_1.900.1-13+deb7u6_amd64.deb

echo 'export WRFIO_NCD_LARGE_FILE_SUPPORT=1' >> /root/.bashrc
echo 'export NETCDF=/usr' >> /root/.bashrc
source /root/.bashrc


# Test WRF libs compatability
cd /tmp/WRF/compilersTest/
cp ${NETCDF}/include/netcdf.inc .
mpicc -c 02_fortran+c+netcdf+mpi_c.c
mpif90 02_fortran+c+netcdf+mpi_f.f \
	02_fortran+c+netcdf+mpi_c.o \
	     -L${NETCDF}/lib -lnetcdff -lnetcdf

mpirun ./a.out

# Compile WRF
cd /tmp/WRF/WRFV3/
./clean
./configure
./compile em_real >& compile.log 
ldconfig

