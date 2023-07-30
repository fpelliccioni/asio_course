# asio_course

# conan.lock creation

```
rm conan.lock
conan lock create conanfile.py --version="1.0.0" --update
```

# Build

```
rm -rf build
conan lock create conanfile.py --version "1.0.0" --lockfile=conan.lock --lockfile-out=build/conan.lock
conan install conanfile.py --lockfile=build/conan.lock -of build --build=missing
cmake --preset conan-release
cmake --build --preset conan-release -j4
```
