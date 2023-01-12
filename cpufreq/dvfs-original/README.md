# Linux Kernel DVFS CPU Governor
## Setup your environment for local testing
2) Download Ubuntu latest stable release (Ubuntu 16.04.3 LTS)
3) Install it
4) Log in, open a terminal, and run :
```shell
# should output your kernel version : 4.13.0-36-generic
uname -r

# install linux-source, linux-headers, and linux-image corresponding to your kernel version
sudo apt install linux-source-4.13.0
sudo apt install linux-headers-4.13.0-36-generic
sudo apt install linux-image-4.13.0-36-generic

# install python dependencies
sudo apt install python-pip
pip install pssh

# install utils that we'll use to switch CPU governor
sudo apt install cpufrequtils
```
5) You're done !

## Build the project, and run it
The `Makefile` has a few tasks in it, that make it easier to run commands so don't have to type them everytime.
```shell
make build # compile the .c files & create the .ko (kernel object) patch file
```
```shell
make clean # clean all the compiled files (.o, etc...)o
```
```shell
make install # install the mod on your current VM (you need to build it first of course)
```
```shell
make remove # uninstall the mod
```
```shell
make log # show the kernel logs; just an alias for dmesg
make tail # stream kernel logs in real time; any new log will be displayed
```
```shell
make set # set your CPU frequency management governor
```
```shell
make unset # unset your CPU frequency management governor
```

So, a basic workflow when doing experiments should look like :
```shell
make build ; make install ; make set
make unset ; make remove
```

## Run experiment on grid5000

### Register an account on grid5000
https://www.grid5000.fr/mediawiki/index.php/Grid5000:Get_an_account

### Connect to grid5000
> Note : fore more informations on this part, you can also check [the Getting Started guide of grid5000](https://www.grid5000.fr/mediawiki/index.php/Getting_Started#Connecting_for_the_first_time)

```
# add in your ~/.ssh/config
Host g5k
        HostName access.grid5000.fr
        User your_user_name
        IdentityFile ~/.ssh/id_rsa  
```
Then to connect you'll type in your shell : `ssh g5k`

### Deploy experiment and collect results
See [README file in the deploy/ folder](deploy/README.md)
