#!/bin/bash

while getopts u:f: option
do 
    case "${option}"
        in
        u) USER=${OPTARG};;
        f) FRONT=${OPTARG};;
    esac
done



# check if the remote tmp directory is ready to receive the project
# if not create the directory
ssh $USER@access.grid5000.fr touch /tmp/$USER/test
if [ $? -eq 1 ]; then
    ssh $USER@access.grid5000.fr mkdir /tmp/$USER
fi

# create the archive
echo "Creating archive"
tar zcvf ../../DVFS.tar.gz ../../dvfs-linux-kernel/ > /dev/null

# send the archive to the access
echo "Sending archive to access"
scp ../../DVFS.tar.gz $USER@access.grid5000.fr:/tmp/$USER


# check if the $FRONT frontend tmp directory is ready
# if not create it
ssh $USER@access.grid5000.fr ssh $FRONT touch /tmp/$USER/test
if [ $? -eq 1 ]; then
    ssh $USER@access.grid5000.fr ssh $FRONT mkdir /tmp/$USER
fi

# send the archive from access to front
echo "sending archive to the front end"
ssh $USER@access.grid5000.fr scp /tmp/$USER/DVFS.tar.gz $FRONT:/tmp/$USER

# extract the DVFS project in the front
echo "extracting archive in the front"
ssh $USER@access.grid5000.fr ssh $FRONT tar -zxvf /tmp/$USER/DVFS.tar.gz -C /tmp/$USER/ > /dev/null

# cleaning
echo "cleaning"
ssh $USER@access.grid5000.fr ssh $FRONT rm /tmp/$USER/DVFS.tar.gz
ssh $USER@access.grid5000.fr rm /tmp/$USER/DVFS.tar.gz
rm ../../DVFS.tar.gz
