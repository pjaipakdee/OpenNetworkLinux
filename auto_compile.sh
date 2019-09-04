#!/bin/bash
echo "This is just a docker test"
##sudo -s

#modify the wget configuration
http_proxy="http:\/\/shrd:QQww4321@10.204.9.8:9090"
https_proxy="http:\/\/shrd:QQww4321@10.204.9.8:9090"
ftp_proxy="http:\/\/shrd:QQww4321@10.204.9.8:9090"
use_proxy="on"

result=$(cat ~/.wgetrc | grep "http_proxy=")
if [[ "$result" != "" ]]
then
     sed -i "s/\(http_proxy=\).*/\1${http_proxy}/g" ~/.wgetrc
else
     echo "http_proxy=http://shrd:QQww4321@10.204.9.8:9090" >> ~/.wgetrc
fi

result=$(cat ~/.wgetrc | grep "https_proxy=")
if [[ "$result" != "" ]]
then
     sed -i "s/\(https_proxy=\).*/\1${https_proxy}/g" ~/.wgetrc
else
     echo "https_proxy=http://shrd:QQww4321@10.204.9.8:9090" >> ~/.wgetrc
fi


result=$(cat ~/.wgetrc | grep "ftp_proxy=")
if [[ "$result" != "" ]]
then
     sed -i "s/\(ftp_proxy=\).*/\1${ftp_proxy}/g" ~/.wgetrc
else
     echo "ftp_proxy=http://shrd:QQww4321@10.204.9.8:9090" >> ~/.wgetrc
fi


result=$(cat ~/.wgetrc | grep "use_proxy =")
if [[ "$result" != "" ]]
then
     sed -i "s/\(use_proxy = \).*/\1${use_proxy}/g" ~/.wgetrc
else
     echo "use_proxy = on" >> ~/.wgetrc
fi

cat ~/.wgetrc

#git config add  proxy
git config  --global http.proxy http://shrd:QQww4321@10.204.9.8:9090
git config  --global https.proxy http://shrd:QQww4321@10.204.9.8:9090

#apt-get add proxy
echo "Proxy: http://shrd:QQww4321@10.204.28.89:9090" >> /etc/apt-cacher-ng/acng.conf

#modify the situation for shell
#echo "export http_proxy=\"http://shrd:QQww4321@10.204.9.8:9090\"" >> /etc/rc.local
#echo "export  git config --global http.proxy http://shrd:QQww4321@10.204.9.8:9090" >> /etc/rc.local
#echo "export  git config --global https.proxy http://shrd:QQww4321@10.204.9.8:9090" >> /etc/rc.local
result=$(cat /etc/rc.local | grep "git config --global https.proxy")
if [[ "$result" != "" ]]
then
     sed -i "s/\(https.proxy \).*/\1${https_proxy}/g" /etc/rc.local
else
     sed -i '2i\export  git config --global https.proxy http:\/\/shrd:QQww4321@10.204.9.8:9090
' /etc/rc.local
fi


result=$(cat /etc/rc.local | grep "git config --global http.proxy")
if [[ "$result" != "" ]]
then
     sed -i "s/\(http.proxy \).*/\1${http_proxy}/g" /etc/rc.local
else
     sed -i '2i\export  git config --global http.proxy http:\/\/shrd:QQww4321@10.204.9.8:9090
' /etc/rc.local
fi




result=$(cat /etc/rc.local | grep "http_proxy=")
if [[ "$result" != "" ]]
then
     sed -i "s/\(http_proxy=\).*/\1\"${http_proxy}\"/g" /etc/rc.local
else
     sed -i '2i\export http_proxy=\"http:\/\/shrd:QQww4321@10.204.9.8:9090\"' /etc/rc.local
fi

export http_proxy="http://shrd:QQww4321@10.204.9.8:9090"


#modify onl onl source to support multitrap apt
sed -i "s/source: http:\/\/apt.opennetlinux.org\/debian/source: http:\/\/\${APT_CACHE}apt.opennetlinux.org\/debian/g" ./builds/any/rootfs/jessie/standard/standard.yml

#compile onl
source setup.env
sudo service apt-cacher-ng stop
sudo /etc/init.d/apt-cacher-ng restart
make amd64

#modify 
echo "finish"

