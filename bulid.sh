
#!/bin/bash

mkdir -p testing
clang main.c -o testing/program

if [ $? -eq 0 ]; then
    echo "Build successful!"
else
    echo "Build failed!"
    exit 1
fi
