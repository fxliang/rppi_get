cmake -B ninja . -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build ninja --config Release
mkdir rppi_get
copy ninja\rppi_get.exe .\rppi_get\
copy rppi_config.yaml .\rppi_get\
copy LICENSE .\rppi_get\
copy README.md .\rppi_get\
