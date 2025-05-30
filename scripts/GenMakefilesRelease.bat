@echo off
SetLocal EnableDelayedExpansion

cd ..

if not exist bin (
    mkdir bin
)

pushd bin

cmake -S .. -B . -G "MinGW Makefiles" -DCMAKE_EXPORT_COMPILE_COMMANDS=ON -DCMAKE_BUILD_TYPE=Release
copy compile_commands.json ..

popd

cd scripts
