# rppi_get 
A toy to play with [rppi](https://github.com/rime/rppi)

## how to build
### linux (Ubuntu for example)
```bash
sudo apt update
sudo apt install libgit2-dev -y
git clone -v --depth=1 https://github.com/fxliang/rppi_get.git --recursive
cd rppi_get
cmake -B build .
cmake --build build --config Release
```
output in `build/rppi_get`, copy and play with it. I am not a good linux user, so help yourself.

### windows

install cmake, visual studio and git, and cmake --build it. 

Or you can download the latest artifact of main branch, in [Actions](https://github.com/fxliang/actions)

maybe mingw-w64 also works, but I haven't tried it out.

```cmd

git clone -v --depth=1 https://github.com/fxliang/rppi_get.git --recursive
cd rppi_get
cmake -B build .
cmake --build build --config Release
mkdir rppi_get
copy build\Release\rppi_get.exe .\rppi_get\
copy build\deps\libgit2\Release\git2.dll .\rppi_get\
copy build\deps\yaml-cpp\Release\yaml-cpp.dll .\rppi_get\
copy rppi_config.yaml .\rppi_get\
```

## Usage

configurations in rppi_config.yaml in the app directory, or `~/rppi_config.yaml` , `#` to disable, however don't comment `user_dir` and `cache_dir`

```yaml
# your proxy setting for libgit2
# proxy: http://localhost:8118
# your github mirror setting, if your network not so good
# mirror: https://hub.yzuu.cf/
# target user directory
user_dir: ~/usr_dir
# cache directory
cache_dir: ~/.rppi_cache/
```

command helps bellow
```
 - A toy to play with rppi
Usage:
  rppi_get [OPTION...]

  -h, --help         print help
  -u, --update       update rppi
  -i, --install arg  install recipe
  -g, --git arg      install recipe by git repo
  -s, --search arg   search recipe with keyword
  -c, --clean        clean caches
  -v, --verbose      verbose settings
  -l, --list         list recipes in rppi
```

## Examples(windows)

- to update rppi index
```cmd
rppi_get.exe -u
```


- to list all recipes in rppi
```cmd
rppi_get.exe -l
```

- to search recipes in rppi
```cmd
rppi_get.exe -s wubi
```
got output like bellow
```cmd
search recipe with keyword: wubi
name: 86五笔, repo: rime/rime-wubi
name: 86五笔极点, repo: KyleBing/rime-wubi86-jidian
name: 98五笔, repo: lotem/rime-wubi98
name: 大字符集五笔, repo: lotem/rime-linguistic-wubi
name: 孤狐五笔, repo: lotem/rime-guhuwubi
```

- to install a recipe to user_dir, type the name or repo 
```cmd
rppi_get.exe -i rime/rime-bopomofo
```

or

```cmd
rppi_get.exe -i 注音
```

- to install a recipe with specific recipe.yaml
```cmd
rppi_get.exe -i iDvel/rime-ice:others/recipes/full
```

- to install a recipe with github repository
```cmd
rppi_get.exe -g iDvel/rime-ice
```
- to install a recipe with github repository with specific recipe.yaml
```cmd
rppi_get.exe -g iDvel/rime-ice:others/recipes/full
```
