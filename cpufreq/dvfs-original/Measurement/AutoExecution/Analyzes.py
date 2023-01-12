from DataProcesing import Preprocesign
import os
import shutil
import matplotlib
matplotlib.use('Agg')
import matplotlib.pyplot as plt


class DrawGrpah():
    def __init__(self,net,host):
        self.net = net
        self.host = list(host)


    def plot(self, df_perf, df_power,title,out):
        fig, axes = plt.subplots(nrows=7, ncols=2,figsize=(12, 6))
        fig.suptitle(title, fontsize=12)
        fig = plt.gcf()
        fig.draw(fig.canvas.get_renderer())
        fig.tight_layout()

        def pl(arr, i, j):
            arr.plot(ax=axes[i, j])
            axes[i, j].set_xlabel(arr.name,fontsize=7)

            avg = int(arr.mean())
            std = int(arr.std())
            avgS = 'Average:{}, Std:{}'.format(avg,std)
            axes[i, j].text(0.85, 0.7, avgS,
                            horizontalalignment='center',
                            verticalalignment='center',
                            transform=axes[i, j].transAxes, fontsize=6)



        pl(df_perf['I/O RPS'], 0, 0)
        pl(df_perf['Disk Usage (%)'], 0, 1)
        pl(df_perf['I/O Avg Wait (in ms)'], 1, 0)

        pl(df_perf['Kernel CPU Load (%)'], 1, 1)
        pl(df_perf['User CPU Load (%)'], 2,0)
        pl(df_perf['CPU Idle (%)'], 2, 1)

        pl(df_perf['Net Throughput Incoming (in KB/s)'], 3, 0)
        pl(df_perf['Net Throughput Outgoing (in KB/s)'], 3, 1)
        pl(df_perf['Net Avg Packet (in packet/s)'], 4, 0)

        pl(df_perf['Memory Util (%)'], 4, 1)
        pl(df_perf['Memory Cache (in KB)'], 5, 0)
        pl(df_perf['Memory Buffer (in KB)'], 5, 1)

        pl(df_power['Power (in W)'], 6, 0)

        plt.savefig(out,bbox_inches='tight')

    def plotExpimetnCombination(self):

        if not os.path.exists('combination'):
            Preprocesign.groupExperiments("/home/amkoyan/DVFS/Measurement/data")

        # sort according nb of repeat per combination
        def splitString(string):
            splitList = string.split('-')
            npbclass = splitList[1]
            n_core = splitList[3]
            repeat = splitList[5]
            benchmark = splitList[7]
            freq = splitList[9]
            return npbclass,n_core,repeat,benchmark,freq

        rootDir = os.listdir('.')
        for root in rootDir:
            if root.startswith('combination'):
                for host in self.host:
                    f = os.listdir(root)[0]
                    cur_npbclass, cur_n_core, cur_repeat, cur_Banchmark, cur_Freq = splitString(f)
                    
                    title = 'Hostname:{},Freq:{},Banchmark:{},NbCore:{}'.format(
                        host,
                        cur_Freq,
                        cur_Banchmark,
                        cur_n_core
                    )

                    out = '{}_Hostname-{}_Freq-{}_Banchmark-{}_Block-{}'.format(
                        root,
                        host,
                        cur_Freq,
                        cur_Banchmark,
                        cur_n_core
                    )

                    # Instantiate the DVFS  object and getAvg panda dataframe and convert to csv and html
                    k = Preprocesign(root, self.net, host)
                    avgPerf = k.avgPerfValue().transpose()
                    avgPerf.to_csv('Perf_' + out + '.csv')
                    avgPerf.to_html('Perf_' + out + '.html')

                    avgPower = k.avgPowerValue().transpose()
                    avgPower.to_csv('Power_' + out + '.csv')
                    avgPower.to_html('Power_' + out + '.html')

                    # plot two data frames in the same picture
                    self.plot(avgPerf, avgPower,title,'PNG_' + out + '.png')

                    # sort all observation together
                    for f in os.listdir('.'):
                        if f.endswith('csv') or f.endswith('html')  or f.endswith('png') or f.endswith('.dat'):
                            perExpOut = 'Measurement_' + root
                            if not os.path.exists(perExpOut):
                                os.mkdir(perExpOut)
                            shutil.move(f, perExpOut)




