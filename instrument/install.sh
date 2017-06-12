apt-get install software-properties-common python-software-properties
sudo add-apt-repository ppa:ubuntu-toolchain-r/test -y

apt-get update
apt-get install gcc-5 g++-5 -y
apt-get install automake make wget curl tar unzip autoconf libtool curl make -y

mkdir /usr/local/download
cd /usr/local/download

wget https://github.com/google/protobuf/archive/v3.3.0.tar.gz -O protobuf.tar.gz

mkdir protobuf
tar -xzf protobuf.tar.gz -C ./protobuf --strip-components=1

cd ./protobuf

./autogen.sh
./configure
make -j 3
make install

cd /usr/local/download

wget https://github.com/gabime/spdlog/archive/v0.13.0.tar.gz -O spdlog.tar.gz
mkdir spdlog
tar -xzf spdlog.tar.gz -C spdlog --strip-components=1
cd spdlog/include
cp -r spdlog /usr/local/include/.

cd /usr/local/download

wget -c 'http://sourceforge.net/projects/boost/files/boost/1.64.0/boost_1_64_0.tar.gz/download' \
    -O boost.tar.gz
mkdir boost
tar -xzf boost.tar.gz -C boost --strip-components=1
cd boost
cp -r boost /usr/local/include/.

cd /usr/local/download

wget https://github.com/libuv/libuv/archive/v1.11.0.tar.gz -O libuv.tar.gz
mkdir libuv
tar -xzf libuv.tar.gz -C libuv --strip-components=1
cd libuv/include
cp * /usr/local/include/.

cd /usr/local/download
rm protobuf.tar.gz
rm spdlog.tar.gz
rm boost.tar.gz

