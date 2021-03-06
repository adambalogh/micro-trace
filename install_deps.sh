apt-get update

apt-get install software-properties-common -y
add-apt-repository ppa:ubuntu-toolchain-r/test -y
apt-get update

apt-get install g++-5 -y
apt-get install automake make wget curl tar unzip autoconf libtool curl make -y

mkdir /usr/local/download
cd /usr/local/download

wget https://github.com/google/protobuf/archive/v3.3.0.tar.gz -O protobuf.tar.gz

mkdir protobuf
tar -xzf protobuf.tar.gz -C ./protobuf --strip-components=1

cd ./protobuf

./autogen.sh
./configure CXX="g++-5"
mak
make install

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
rm -rf protobuf
rm spdlog.tar.gz
rm -rf spdlog
rm boost.tar.gz
rm -rf boost
rm libuv.tar.gz
rm -rf libuv
