from gcc:12.1.0

run apt-get -y update 
run apt-get -y install cmake 


copy . /usr/src/app
workdir /usr/src/app

run rm -r build
run cmake -S . -B build 
run cd build && make .

cmd ["./LC3VM", "../Programs/2028.obj"]
