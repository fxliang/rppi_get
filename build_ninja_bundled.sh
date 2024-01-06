# determin Debug or Release by 1st param of script
cmake -B ninja . -G Ninja -DCMAKE_BUILD_TYPE=$1 \
	-DUSE_BUNDLED_LIBGIT2=ON \
	-DUSE_BUNDLED_JSON=ON \
	-DUSE_BUNDLED_YAMLCPP=ON \
	-DUSE_BUNDLED_CXXOPTS=ON
cmake --build ninja --config $1
cp ninja/rppi_get ./
