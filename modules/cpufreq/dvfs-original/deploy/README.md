# Deploy the experiment to grid5000

Let be $USER the variable that contain your grid5000 user and $FRONT the one that define with which frontend you are working with.

## Initialisation
1. Run the _pustToFront.sh_ script (located in /deploy). provide the option -u $USER and -f $FRONT. It will automatically copy the complete project to the grid05000 frontend you specified. The process can take some time depending on your connection speed.
```
cd deploy/
bash pustToFront -u $USER -f $FRONT
```

2. Connect to your frontend and run a screen session to get an uninteruptable ssh session.
```
ssh $USER@access.grid5000.fr
ssh $FRONT
screen
cd /tmp/$USER/dvfs-linux-kernel
```

## Prepare the experiment
3. Reserve the node you need
	* __wattmeter='YES'__: Required to tell grid5000 that we need nodes where the power consumption can be probe.
	* __cluster='orion'__: We would like to perform the experiment on the orion cluster. (orion have fairly new intel cpu [lyon:hardware](https://www.grid5000.fr/mediawiki/index.php/Lyon:Hardware). To perform the experiment, cpu with dynamic frequency functionnality are needed. Choose your cluter carrefully (link above)
	* __I__: For interactive mode. As soon as the ssh session is terminated, the node are released.
	* __cluster=1__: All node should be on the same cluster to ensure an homogeneity of the hardware.
	* __nodes=2__: Here put as much as you need (should fit in the number of node present in the cluster you asked for).
	*__walltime=4:0:0__: The node are reserved for a maximum of of 4 hours. see [grid5000:UsagePolicy](https://www.grid5000.fr/mediawiki/index.php/Grid5000:UsagePolicy)
	* __t__: deploy the ssh key to the node
```
oarsub -p "cluster='orion' AND wattmeter='YES'" -I -l cluster=1/nodes=2,walltime=4:0:0 -t deploy
```

4. Deploy the linux image
	* "_ubuntu16.04-x64-min.env_" is in the _deploy_ directory
```
kadeploy3 -a ubuntu16.04-x64-min.env  -f $OAR_FILE_NODES -k
```


5. oarsub reserves the nodes and store the name of the chosen machine in the variable $OAR_FILE_NODES. It is important to copy the content of this variable into the _cluster.txt_ file in _deploy_ directory in order for the scripts to work. It is also required to export the local python bin path to use very usefull commands such as pssh.
```
cd deploy/
cat $OAR_FILE_NODES | uniq > cluster.txt
export PATH=$PATH:~/.local/bin/
```

6. Compile the experiment benchmark software on all nodes by running the script _main.sh_,  :warning: the process is quite long and depends on the hardware you reserved. installation time for a i5-6200U is approximatively 20 minutes.
```
bash main.sh
```

7. Change the kernel to 4.10 for each nodes by running _postdeploy-smart-governor_ script
 :warning: This will reboot the nodes and the procedure can take sometime
```
bash postdeploy-smart-governor.sh
```

8. Run the _buildSmartGovernor.sh_ script to compile and set the DVFS governor in the nodes.
```
bash buildSmartGovernor.sh
```

## Run the experiment
1. Move the the _Measurement_ directory and create here a new one, called _results_, that will contain all the result of the experiment, the AutoExecution/main.py will successively run the different benchmark on all node and aggregate the data in the _result_ directory.
```
cd ../Measurement
mkdir results
python AutoExecution/main.py --nodes ../deploy/cluster.txt -$FRONT -o results/
```

2. Repeat the experiment for all the governor you would like to compare your result with. _performance_, _ondemand_, _powersave_, ...


	8.2)
		screen -d -m sh -c '/tmp/NPB/bin/sar.sh'
		# sysstat script removes afters restart, so ensure that is runing 
