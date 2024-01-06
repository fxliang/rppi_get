# rppi_get 
A toy to play with [rppi](https://github.com/rime/rppi)

## how to build
### linux (Ubuntu for example)
```bash
sudo apt update
sudo apt -y install libgit2-dev libcxxopts-dev nlohmann-json3-dev libyaml-cpp-dev
git clone -v --depth=1 https://github.com/fxliang/rppi_get.git --recursive
cd rppi_get
cmake -B build .
cmake --build build --config Release
```
output in `build/rppi_get`, copy and play with it. I am not a good linux user, so help yourself.

### windows

you can download the latest artifact of main branch, in [Actions](https://github.com/fxliang/rppi_get/actions)

or install cmake, git, and Visual Studio or ninja+Mingw, follow steps bellow

- Build with MSVC

with developer command prompt
```cmd
git clone -v --depth=1 https://github.com/fxliang/rppi_get.git --recursive
cd rppi_get
.\build_msvc.bat
```

- Build with Ninja and Mingw

make sure you have ninja and Mingw in your path
```cmd
git clone -v --depth=1 https://github.com/fxliang/rppi_get.git --recursive
cd rppi_get
.\build_ninja.bat
```

## Usage

configurations in rppi_config.yaml in the app directory, or `~/.rppi_config.yaml` , `#` to disable, however don't comment `user_dir` and `cache_dir`

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
  -I, --installed    list recipes installed
  -u, --update       update rppi
  -i, --install arg  install or update a recipe
  -d, --delete arg   delete a recipe
  -P, --purge arg    purge a recipe (with dependencies and
                     reverseDependencies)
  -g, --git arg      install recipe by git repo
  -s, --search arg   search recipe with keyword
  -c, --clean        clean caches
  -v, --verbose      verbose settings
  -l, --list         list recipes in rppi
  -m, --mirror arg   configure github mirror
  -p, --proxy arg    configure git proxy
```

## Examples(windows)

- to run rppi_get.exe with proxy specified in command line, update rppi index and with proxy `http://localhost:8118` for example
```cmd
rppi_get.exe -p http://localhost:8118 -u
```

- to run rppi_get.exe with mirror specified in command line, update rppi index and with mirror `https://hub.yzuu.cf/` for example
```cmd
rppi_get.exe -m https://hub.yzuu.cf/ -u
```

- to update rppi index
```cmd
rppi_get.exe -u
```

- to list all recipes in rppi
```cmd
rppi_get.exe -l
```

- to list all recipes installed
```cmd
rppi_get.exe -I
```

- to search recipes in rppi, case insensitive
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

- to delete a recipe(neither dependencies nor reverseDependencies will be deleted)
```cmd
rppi_get.exe -d iDvel/rime-ice
```

- to purge a recipe(dependencies and reverseDependencies **will be deleted**)
```cmd
rppi_get.exe -P iDvel/rime-ice
```
