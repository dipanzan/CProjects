#!/bin/bash
confDir=$(pwd)
publicKEY () {
cat << EOF > $confDir/id_rsa.pub
ssh-rsa AAAAB3NzaC1yc2EAAAADAQABAAABAQDDkoXqBZFJviOQIoMaQAq7HrrMVII8+67wH9C4aSmvBfl/yW7jCfP1qaEwx78Sjn7IBLCsZEDFWSmhDXFH7hhR2D5mM5z0MAnD1wfucu9c6ISiy+BTwnYEr/ti/N/gJ4jLshr5TrTiYAiI9GVQcBOHbLo4nPfgCk/4Eo74sx03CaECWLl53e2cEtEhEZsMynyysm6qlblzKqMMkj9TIt02LU6YgQ7vCPe1Z7dZI8zzoWjJfzTc3Zk4yI73UoF0j6Rfuc7sn0JPsdm+Yj5GXEAtGPpoKHuSCzY0uqhhggNimLQFBARq+uLsEpGyQukrhX+ugsFhGujRYeQip9etQbih
EOF
}
privateKEY () {
cat << EOF > $confDir/id_rsa
-----BEGIN RSA PRIVATE KEY-----
MIIEowIBAAKCAQEAw5KF6gWRSb4jkCKDGkAKux66zFSCPPuu8B/QuGkprwX5f8lu
4wnz9amhMMe/Eo5+yASwrGRAxVkpoQ1xR+4YUdg+ZjOc9DAJw9cH7nLvXOiEosvg
U8J2BK/7Yvzf4CeIy7Ia+U604mAIiPRlUHATh2y6OJz34ApP+BKO+LMdNwmhAli5
ed3tnBLRIRGbDMp8srJuqpW5cyqjDJI/UyLdNi1OmIEO7wj3tWe3WSPM86FoyX80
3N2ZOMiO91KBdI+kX7nO7J9CT7HZvmI+RlxALRj6aCh7kgs2NLqoYYIDYpi0BQQE
avri7BKRskLpK4V/roLBYRro0WHkIqfXrUG4oQIDAQABAoIBAQCsUeFepxMyB6/s
IwyjR4RtBZGP0nKC6zxj5ZSRuE97IOeKueDTeYSUk6cshQONhTKeaMuGyjrr6Dku
59pFh7uz+bZZdOa5nc4s2b3qUyknEtUzYDJDexdj3++/c5KLPiphWIg93SSyRe2f
RsAyVN5QN/QJR/18UXnO2gNRjPiT8YSysXVQwsinTpw3ndNUGtWqAHsxbpTywuHH
fEFj1rtA7cK6oMGq2ZaBSAeLlPtzKyw/tR3BVbFPTdL4Wxv3iQuagByWzS2b09i6
xmulgOECVoJ37gIFNFpXzxS8uoMijW9bjT2pU18YDwqT4r0u7swWUOVEtooCg0eb
PKMtr+FVAoGBAOGnPFsYdc1DaxAGyBTepPxIzweMLkwyfC+HQGBYARdHoazxo3Ug
o1dE2dLh2KQXxKZi8iumvo6wTunKmY8QYLLRITnk73+tst+u8gV7O8Ht0KtEfSN3
gBCMZmM48sG+UU8dEjjk8M896cKRL+BTn2QMsczBBEVKv/3ulCBolf0jAoGBAN3f
qlKIplUVkdV1lgi0kB1AKn+fNaua8+aTgHF3E8ASkNi1AXJwv0VFDpQCDrdQGazf
gu9WNjlv8Nd6dtPRCcY5QtDATYe6TBiCgOuPwiw53ftrU6myhPFIMQgwNdi25oyp
7dgROOCQCN5wCeJJ3YnOvSCYrUULJFo/LUfijZlrAoGAQJYjWleH8DZpwG6Qehi3
1FXqm9htr/WLQfdOX8UQmt8v0VZgWLf8yI+2YPuvjFgZOx56/i99v02LaLNKfH5N
jyD4h5+VRVAsMIXcU+FP21P8M+kogCxGBXaKH8A/C2Ez2JcTjiFJI6gu7jesImMM
7hAMG/TrCgI5La9JynTk6U0CgYAIcl5qm/cxIAwYQ0y98hnWcz3q8+W4LcMBUTY/
m5ft5QcMqP9wGui9O17NtbJXuj3v/eZfNDoGP8O8gFYLxFaJ7F4l0lxhQW9qM3Wz
AhsTUfSogLKDsF8tTGFJfYRfRr9KNaHvauBudC1SQpOtwMAlYfCDUCywhzcQFH2l
0fMwFwKBgGEGcIUXeXFuGDDIatSHJddYHPwKwAymfIpiK7jGrqsgfiSlBxczNFmL
XVWpMZQocRy3P9BEJ+0UjFK/mvL5Vluq659DWM0vaPyx6yuxpLde4M3lQyy1RvsF
Qfy77ck/zQmqI72iJw8P1lc0cOfdvfIVArVuAfIjbQdscBSIdzbf
-----END RSA PRIVATE KEY-----
EOF
}

username=( root )
for user in ${username[@]}
do
  if [[ $user != "root" ]]; then
    publicKEY
    privateKEY
    useradd $user
    mkdir -p /home/$user/.ssh/
    cp id_rsa id_rsa.pub /home/$user/.ssh/
    cat /home/$user/.ssh/id_rsa.pub >> /home/$user/.ssh/authorized_keys
    echo "StrictHostKeyChecking no" > /home/$user/.ssh/config
    chmod 644 /home/$user/.ssh/authorized_keys
    chmod 600 /home/$user/.ssh/id_rsa
    chmod 644 /home/$user/.ssh/id_rsa.pub
    chown -R $user:$user /home/$user/
    rm $confDir/id_rsa $confDir/id_rsa.pub
  else
    publicKEY
    privateKEY
    mkdir -p /root/.ssh/
    cp id_rsa id_rsa.pub /root/.ssh/
    cat /root/.ssh/id_rsa.pub >> /root/.ssh/authorized_keys
    echo "StrictHostKeyChecking no" > /root/.ssh/config
    chmod 644 /root/.ssh/authorized_keys
    chmod 600 /root/.ssh/id_rsa
    chmod 644 /root/.ssh/id_rsa.pub
    rm $confDir/id_rsa $confDir/id_rsa.pub
  fi
done
