cmd_/home/dipanzan/CProjects/modules/cpufreq/ondemandx-test/Module.symvers := sed 's/\.ko$$/\.o/' /home/dipanzan/CProjects/modules/cpufreq/ondemandx-test/modules.order | scripts/mod/modpost -m -a  -o /home/dipanzan/CProjects/modules/cpufreq/ondemandx-test/Module.symvers -e -i Module.symvers   -T -