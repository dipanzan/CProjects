import json
import execo
import os
import time

class JSONObject:
    def __init__(self, d):
        self.__dict__ = d

def collect_metric(startTime,endTime,metric,result_dir,site,resultFile,hostname):
    std=resultFile+'_std.txt'
    ste=resultFile+'_err.txt'
    stdCSV=resultFile+'.csv'

    if not os.path.exists(result_dir):
        os.mkdir(result_dir)

    host = hostname.split('.')[0]

    cmd_template = ( 'curl -kn "https://api.grid5000.fr/stable/sites/{site}/metrics/{metric}/timeseries?resolution=15&only={hostname}&from={startTime}&to={endTime}"')
    cmd = cmd_template.format(site=site, metric=metric, hostname=host, startTime=startTime, endTime=endTime)
    print(cmd)

    collector = execo.Process(cmd=cmd)
    collector.run()

    with open(os.path.join(result_dir, std), "w") as stdout, open(
            os.path.join(result_dir, ste), "w") as stderr:
        stdout.write(collector.stdout)
        stderr.write(collector.stderr)

    with open(os.path.join(result_dir, std), "r") as fp, open(
            os.path.join(result_dir, stdCSV), "w") as out:
        data = json.load(fp, object_hook=JSONObject)

        if metric == "power" or metric == "network_in" or metric == "network_out":
            item_index = 0
            for item in data.items:
                href = item.links[0].href
                host_name = os.path.basename(href)
                item_index += 1
                for index in range(len(item.values)):
                    out.write("%s,%f,%d\n" % (host_name, item.timestamps[index], item.values[index]))
        else:
            item_index = 0
            for item in data.items:
                timestamp = [i for i in range(int(startTime), int(startTime) + len(item.values))]
                href = item.links[0].href
                host_name = os.path.basename(href)
                item_index += 1
                for index in range(len(item.values)):
                    out.write("%s,%f,%s\n" % (host_name, timestamp[index], item.values[index]))

# stuff to run always here such as class/def
def main():
    pass
if __name__ == "__main__":
   main()
