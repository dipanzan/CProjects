from optparse import OptionParser
from DVFS import DVFS
from Analyzes import DrawGrpah
parser = OptionParser()
parser.add_option(
                    "-n",
                    "--nodes",
                    dest="nodes",
                    metavar = "FILE",
                    help="Host file",
                    default="/home/amkoyan/DVFS/deploy/cluster.txt"
                  )

parser.add_option(
                    "-o",
                    "--outDir",
                    dest="outDir",
                    help="Output Path",
                    default="/home/amkoyan/DVFS/Measurement/data/"
                  )

parser.add_option(
                    "-s",
                    "--site",
                    dest="site",
                    help="Grid5000 Site name",
                    default="lyon"
                  )

parser.add_option(
                    "-d",
                    "--device",
                    dest="device",
                    help="Network Type Ethernet | rdma",
                    default="ethernet"
                  )

(options, args) = parser.parse_args()

def parseNode(filename=options.nodes):
        dataList = []
        with open(filename, "r") as f:
            for line in f.read().splitlines():
                dataList.append(line)
            return dataList

def main():
    cluster = parseNode()
    path = options.outDir
    grid5000site = options.site

    startProcess = DVFS(path,cluster,grid5000site)
    startProcess.run()

    analyzesData = DrawGrpah(options.device,cluster)
    analyzesData.plotExpimetnCombination()

if __name__ == "__main__":
    main()
