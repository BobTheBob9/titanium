cmake . -Bbuild -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ -DCMAKE_BUILD_TYPE=DEBUG && compdb -p build/ list > compile_commands.json && make -C build -j8
