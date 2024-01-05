cmake -B build .
cmake --build build --config Release
cmake -B build32 . -A Win32
cmake --build build32 --config Release
mkdir rppi_get
copy build\Release\rppi_get.exe .\rppi_get\rppi_get_x64.exe
copy build32\Release\rppi_get.exe .\rppi_get\
copy rppi_config.yaml .\rppi_get\
copy LICENSE .\rppi_get\
copy README.md .\rppi_get\
