export SERVER=$1

export REPO='git@github.com:jbapple/HalftimeHash.git' &&
export DIRECTORY='HalftimeHash' &&
scp -o StrictHostKeyChecking=no -i ~/.ssh/aws-key-001.pem ~/.ssh/id_rsa.1* ubuntu@$SERVER:~/.ssh/ &&
scp -o StrictHostKeyChecking=no -i ~/.ssh/aws-key-001.pem ~/.ssh/config ubuntu@$SERVER:~/.ssh/ &&
scp -o StrictHostKeyChecking=no -i ~/.ssh/aws-key-001.pem -r ~/.aws ubuntu@$SERVER: &&
ssh -i ~/.ssh/aws-key-001.pem ubuntu@$SERVER sudo apt-get update &&
ssh -i ~/.ssh/aws-key-001.pem ubuntu@$SERVER sudo apt-get --yes install mosh &&
mosh --ssh "ssh -i ~/.ssh/aws-key-001.pem" ubuntu@$SERVER

return

# TODO: test with tabulation hashing

export REPO='git@github.com:jbapple/HalftimeHash.git' &&
export DIRECTORY='HalftimeHash' &&
export INSTANCE_TYPE=$(curl http://169.254.169.254/latest/dynamic/instance-identity/document 2>/dev/null | grep instanceType 2>/dev/null | cut -d '"' -f 4) &&
sudo add-apt-repository --yes ppa:ubuntu-toolchain-r/test &&
sudo apt-get update &&
sudo apt --yes install gcc-10 g++-10 make git awscli &&
wget https://apt.llvm.org/llvm.sh &&
chmod +x llvm.sh &&
sudo ./llvm.sh 11 &&
git clone $REPO $DIRECTORY &&
cd $DIRECTORY

git submodule update --init --recursive &&
export COMMIT=$(git rev-parse --short HEAD) &&
export CC=clang-11 &&
export CXX=clang++-11 &&
git checkout scratch &&
git submodule update --init --recursive &&
make -k -j$(nproc) benchmark.exe >/dev/null &&
date &&
./benchmark.exe 1 10000000 >>$INSTANCE_TYPE-$CC-$COMMIT.txt &&
date &&
aws s3 cp $INSTANCE_TYPE-$CC-$COMMIT.txt s3://halftime-hash/ &&
date &&
make clean >/dev/null &&
export CC=gcc-10 &&
export CXX=g++-10 &&
make -k -j$(nproc) benchmark.exe >/dev/null &&
date &&
./benchmark.exe 1 10000000 >>$INSTANCE_TYPE-$CC-$COMMIT.txt &&
date &&
aws s3 cp $INSTANCE_TYPE-$CC-$COMMIT.txt s3://halftime-hash/ &&
date &&
sudo shutdown -h now

return

scp -o StrictHostKeyChecking=no -i ~/.ssh/aws-key-001.pem ubuntu@$SERVER:$DIRECTORY/*.txt /tmp/
mosh --ssh "ssh -i ~/.ssh/aws-key-001.pem" ubuntu@$SERVER #sudo shutdown -h now

#shutdown -h now
return
