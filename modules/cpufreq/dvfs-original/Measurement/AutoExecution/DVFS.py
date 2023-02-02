from execo_engine import Engine
import os
import sys
from execo_engine import ParamSweeper, sweep
from execo import Remote,SshProcess, SequentialActions,Get
from execo_engine import logger,slugify
from execo_g5k import get_host_attributes
import time
import datetime
from itertools import takewhile, count
from energy import *
import math


class DVFS(Engine):

    def __init__(self,result_dir,cluster,site):
        Engine.__init__(self)
        self.result_dir = result_dir
        self.cluster = cluster
        self.site = site

    def run(self):
        """Inherited method, put here the code for running the engine"""
        self.define_parameters()
        self.run_xp()

    def define_parameters(self):
        nbNodes = len(self.cluster)
        # build parameters and make nbCore list per benchmark
        freqList = [2534000,2000000,1200000]
        n_nodes = float(len(self.cluster))
        max_core = SshProcess('cat /proc/cpuinfo | grep -i processor |wc -l',self.cluster[0],connection_params={'user': 'root'}).run().stdout
        max_core =  n_nodes * float(max_core)
        even = filter(lambda i: i > n_nodes,list(takewhile(lambda i: i<max_core, (2**i for i in count(0, 1))))) 
        powerTwo = filter(lambda i: i > n_nodes,list(takewhile(lambda i: i<max_core, (i**2 for i in count(0, 1)))))

        # Define parameters 
        self.parameters = {
            'Repeat': [1],
            "Freq": [2534000],
            "NPBclass" : ['C'],
            "Benchmark": {
                # 'ft': {
                #     'n_core': even
                #     },
                # 'ep': {
                #     'n_core': even
                #     },
                # 'lu': {
                #     'n_core': even
                #     },
                # 'is': {
                #     'n_core': even
                #     },
                # 'sg': {
                #     'n_core': even
                #     },
                # 'bt': {
                #     'n_core': powerTwo
                #     },
                'sp': {
                    'n_core': powerTwo
                    }
                }
            }

        logger.info(self.parameters)
        # make all possible parameters object, 
        self.sweeper = ParamSweeper(os.path.join(self.result_dir, "sweeps"), sweep(self.parameters))
        logger.info('Number of parameters combinations %s', len(self.sweeper.get_remaining()))

    def run_xp(self):

        master = self.cluster[0]
        opt=''
        """Iterate over the parameters and execute the bench"""
        while len(self.sweeper.get_remaining()) > 0:
            # Take sweeper 
            comb = self.sweeper.get_next()

            logger.info('Processing new combination %s' % (comb,))

            try:
                # metric from linux sar tools, works with clock
                def takeMetric(path,startTime,endTime,metric=['cpu','mem','disk','swap','network']):
                    opt=''
                    cmd_template_sar = ("sar -f /var/log/sysstat/sa* -{opt} -s {startTime} -e {endTime}")
                    for met in metric:
                        if met == 'cpu':
                            opt='u'
                        elif met == 'mem':
                            opt  = 'r'
                        elif met == 'disk':
                            opt = 'dp'
                        elif met == 'swap':
                            opt = 'S'
                        elif met == 'network':
                            opt = 'n DEV'

                        cmd = cmd_template_sar.format(opt=opt,startTime=startTime,endTime=endTime)
                        for host in self.cluster:
                            hE = SshProcess(cmd, host, connection_params={'user': 'root'})
                            hE.run()
                            stdMetric = host + '-' + met + '.txt'
                            with open(os.path.join(path, stdMetric), "w") as sout:
                                sout.write(hE.stdout)


                #Set CPU Freq and Policy according current combination
                cmd_template_Freq_Policy = ("cpufreq-set -r  -g {policy}")
                cmd_template_Freq = ("cpufreq-set -r -f {freq}")
                
                if comb['Freq'] == 'OnDemand':
                    cmd_freq_policy = cmd_template_Freq_Policy.format(policy='ondemand')
                    Remote(cmd_freq_policy, master, connection_params={'user': 'root'}).run()
                elif comb['Freq'] == 'conservative':
                    cmd_freq_policy = cmd_template_Freq_Policy.format(policy='conservative')
                    Remote(cmd_freq_policy, master, connection_params={'user': 'root'}).run()
                else:
                    cmd_freq_policy = cmd_template_Freq_Policy.format(policy='userspace')
                    Remote(cmd_freq_policy, master, connection_params={'user': 'root'}).run()
                    cmd_freq = cmd_template_Freq.format(freq=comb['Freq'])
                    Remote(cmd_freq, master, connection_params={'user': 'root'}).run()


                # build command
                src = 'source /opt/intel-performance-snapshoot/apsvars.sh'
                cmd_mpirun_template = ("mpirun {opt} -f /root/cluster.txt -np {pr1} aps -r '/tmp/log/' /tmp/NPB/npb-mpi/bin/{typeNPB}.{NPBclass}.{pr2}")
                cmd_mpirun = cmd_mpirun_template.format(opt='',pr1=comb['n_core'],typeNPB=comb['Benchmark'],NPBclass=comb['NPBclass'],pr2=comb['n_core'])
                cmd = "{}; /tmp/NPB/bin/runMPI.sh '{}' '{}'".format(src,cmd_mpirun,slugify(comb))

                curPath = self.result_dir + slugify(comb)
                
                # run Mpi through execo remote SshProcess                 
                def runMpi(cmd):
                    act = SshProcess(cmd, master, connection_params={'user': 'root'},shell=True)
                    act.run()

                    if not os.path.exists(curPath):
                        os.makedirs(curPath)

                    with open(os.path.join(curPath, "stdout.txt"), "a+") as sout, open(
                            os.path.join(curPath, "stderr.txt"), "w") as serr:
                        sout.write(act.stdout)
                        serr.write(act.stderr)
                    return act.ok

                # start clock and exec command in the master node
                time.sleep(5)
                startUnix = int(time.time())
                start24Hour = datetime.datetime.fromtimestamp(startUnix).strftime('%H:%M:%S')

                task1 = runMpi(cmd)

                endUnix = int(time.time())
                end24Hour = datetime.datetime.fromtimestamp(endUnix).strftime('%H:%M:%S')
                time.sleep(5)

                with open(os.path.join(curPath, "executionTime.txt"), "w") as sout:
                    sout.write('ExecTime:{}\nStartDate:{}\nEndDate:{}\n'.format(str(endUnix - startUnix ),start24Hour,end24Hour))

                takeMetric(curPath,start24Hour,end24Hour,['cpu','mem','disk','swap','network'])

                # collect power from kWAPI: grid5000 infrastructure made tool
                for hostname in self.cluster:   
                    powerOut = '{}_power'.format(hostname)
                    collect_metric(startUnix,endUnix,'power',curPath,self.site,powerOut,hostname)


                st = '/tmp/out/' + slugify(comb)
                intelAppPerf = str(st + '.html')

                # get the data from ['Application Performance Snapshot', 'Storage Performance Snapshot']
                # https://software.intel.com/en-us/performance-snapshot
                Get(master, [intelAppPerf],curPath,connection_params={'user': 'root'}).run()
                if task1:
                    logger.info("comb ok: %s" % (comb,))
                    self.sweeper.done(comb)
                    continue

            except OSError as err:
                print("OS error: {0}".format(err))
            except ValueError:
                print("Could not convert data to an integer.")
            except:
                print("Unexpected error:", sys.exc_info()[0])
                raise

            logger.info("comb NOT ok: %s" % (comb,))
            self.sweeper.cancel(comb)
