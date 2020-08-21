set -ex

#export GOOS=windows GOACH=amd64 CC=x86_64-w64-mingw32-gcc CXX=x86_64-w64-mingw32-g++ CGO_ENABLED=1

go get -d ./...
git submodule update --init --recursive
cd dep/stk
autoconf

if [ "$CC" = "x86_64-w64-mingw32-gcc" ]; then
./configure --host=x86_64-w64-mingw32
else
./configure
fi

make
cd -
mkdir -p ./bin
cd ./bin/
if [ "$CC" = "x86_64-w64-mingw32-gcc" ]; then
  go build -x -ldflags=all='-H windowsgui -s -w' ..
else
  go build -x ..
fi
cd ..
