# determin Debug or Release by 1st param of script
cmake -B ninja . -G Ninja -DCMAKE_BUILD_TYPE=$1 \
	-DUSE_BUNDLED_LIBGIT2=OFF \
	-DUSE_BUNDLED_CXXOPTS=OFF \
	-DUSE_BUNDLED_JSON=OFF \
	-DUSE_BUNDLED_YAMLCPP=OFF
cmake --build ninja --config $1
cp ninja/rppi_get ./
