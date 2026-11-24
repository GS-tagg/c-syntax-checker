
#!/bin/bash

mkdir -p testing
clang c-checker.c -o testing/program

if [ $? -eq 0 ]; then
    echo "Build successful!"
else
    echo "Build failed!"
    exit 1
fi
