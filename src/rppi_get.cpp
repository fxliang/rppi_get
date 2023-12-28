#include <algorithm>
#include <cxxopts.hpp>
#include <filesystem>
#include <fstream>
#include <git2.h>
#include <iostream>
#include <nlohmann/json.hpp>
#include <regex>
#include <stdio.h>
#include <string>
#include <vector>
#include <yaml-cpp/yaml.h>
#ifdef _WIN32
#include <Windows.h>
#include <rpcdce.h>
#pragma comment(lib, "rpcrt4.lib")
#endif
using json = nlohmann::json;
using VString = std::vector<std::string>;
#define FINALIZE_GIT(error)                                                    \
  {                                                                            \
    std::cout << std::endl;                                                    \
    git_repository_free(repo);                                                 \
    git_libgit2_shutdown();                                                    \
    return error;                                                              \
  }
// for terminal_width()
#include <utility>
#if defined(_WIN32)
static inline std::pair<size_t, size_t> terminal_size() {
  CONSOLE_SCREEN_BUFFER_INFO csbi;
  int cols, rows;
  GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &csbi);
  cols = csbi.srWindow.Right - csbi.srWindow.Left + 1;
  rows = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
  return {static_cast<size_t>(rows), static_cast<size_t>(cols)};
}
static inline size_t terminal_width() { return terminal_size().second; }
#else
#include <sys/ioctl.h> //ioctl() and TIOCGWINSZ
#include <unistd.h>    // for STDOUT_FILENO
static inline std::pair<size_t, size_t> terminal_size() {
  struct winsize size {};
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &size);
  return {static_cast<size_t>(size.ws_row), static_cast<size_t>(size.ws_col)};
}
static inline size_t terminal_width() { return terminal_size().second; }
#endif
// for terminal_width() ends
struct Recipe {
  std::string repo;
  std::string branch;
  std::string name;
  std::string category;
  VString labels;
  VString schemas;
  VString dependencies;
  VString reverseDependencies;
  std::string license;
  std::string local_path;
};
static int bar_width = 0;
std::string proxy, mirror, user_dir, cache_dir;
#ifdef _WIN32
#define sep "\\"
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = CP_UTF8) {
  unsigned int cp = GetConsoleOutputCP();
  SetConsoleOutputCP(codepage);
  return cp;
}
#else
#define sep "/"
inline unsigned int SetConsoleOutputCodePage(unsigned int codepage = 65001) {
  return 0;
}
#endif /* _WIN32 */
// GetApplicationDirectory
#ifdef _WIN32
#include <shlwapi.h>
#else
#include <libgen.h>
#include <limits.h>
#include <unistd.h>
#endif
std::string GetApplicationDirectory() {
  std::string appPath;
#ifdef _WIN32
  char path[MAX_PATH];
  DWORD len = GetModuleFileName(NULL, path, MAX_PATH);
  if (len != 0) {
    PathRemoveFileSpec(path);
    appPath = std::string(path);
  }
#else
  char path[PATH_MAX];
  ssize_t len = readlink("/proc/self/exe", path, sizeof(path) - 1);
  if (len != -1) {
    path[len] = '\0';
    appPath = std::string(dirname(path));
  }
#endif
  return appPath;
}
// GetApplicationDirectory end
#ifdef _WIN32
std::wstring _to_wstring(const std::string &str, int code_page = CP_ACP) {
  // support CP_ACP and CP_UTF8 only
  if (code_page != 0 && code_page != CP_UTF8)
    return L"";
  // calc len
  int len =
      MultiByteToWideChar(code_page, 0, str.c_str(), (int)str.size(), NULL, 0);
  if (len <= 0)
    return L"";
  std::wstring res;
  wchar_t *buffer = new wchar_t[len + 1];
  MultiByteToWideChar(code_page, 0, str.c_str(), (int)str.size(), buffer, len);
  buffer[len] = '\0';
  res.append(buffer);
  delete[] buffer;
  return res;
}
std::string _to_string(const std::wstring &wstr, int code_page = CP_ACP) {
  // support CP_ACP and CP_UTF8 only
  if (code_page != 0 && code_page != CP_UTF8)
    return "";
  int len = WideCharToMultiByte(code_page, 0, wstr.c_str(), (int)wstr.size(),
                                NULL, 0, NULL, NULL);
  if (len <= 0)
    return "";
  std::string res;
  char *buffer = new char[len + 1];
  WideCharToMultiByte(code_page, 0, wstr.c_str(), (int)wstr.size(), buffer, len,
                      NULL, NULL);
  buffer[len] = '\0';
  res.append(buffer);
  delete[] buffer;
  return res;
}
#else
inline std::string _to_wstring(const std::string &str, int code_page = 0) {
  return str;
}
inline std::string _to_string(const std::string &str, int code_page = 0) {
  return str;
}
#endif
// parse path to abs
#ifdef _WIN32
#include <cstdlib>
#endif
std::string parse_path(const std::string &path) {
  if (path.at(0) == '~') {
#ifdef _WIN32
    char *userProfilePath = nullptr;
    size_t size = 0;
    _dupenv_s(&userProfilePath, &size, "USERPROFILE");
    std::string homedir_;
    if (userProfilePath != nullptr)
      homedir_ = std::string(userProfilePath);
    free(userProfilePath);
    std::filesystem::path homedir = homedir_;
#else
    std::filesystem::path homedir = std::string(getenv("HOME"));
#endif
    std::filesystem::path _path = homedir.string() + path.substr(1);
    return std::filesystem::absolute(_path).string();
  } else {
    std::filesystem::path _path = path;
    return std::filesystem::absolute(_path).string();
  }
}
// for console text color in win32
#ifdef _WIN32
#define FOREGROUND_BLUE 0x0001  // text color contains blue.
#define FOREGROUND_GREEN 0x0002 // text color contains green.
#define FOREGROUND_RED 0x0004   // text color contains red.
void SetConsoleColor(int color) {
  HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
  SetConsoleTextAttribute(hConsole, color);
}
#endif // for console text color in win32 end
// libgit2 relate functions
// process bar for git fetch
int transfer_progress(const git_transfer_progress *stats, void *payload) {
  static int last_percent = -1;
  int barWidth = bar_width; // 50;
  float percentage =
      static_cast<float>(stats->received_objects) / stats->total_objects * 100;
  if (static_cast<int>(percentage) != last_percent) {
    last_percent = static_cast<int>(percentage);
    std::cout << "[";
    int pos = barWidth * percentage / 100;
    for (int i = 0; i < barWidth; ++i) {
      if (i < pos)
#ifndef _WIN32
        std::cout << "\033[32m█\033[0m";
#else
      {
        SetConsoleColor(FOREGROUND_GREEN);
        std::cout << "\xe2\x96\x88";
        SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
      }
#endif
      else if (i == pos)
        std::cout << ">";
      else
        std::cout << " ";
    }
    std::cout << "] " << stats->received_objects << "/" << stats->total_objects
              << " objs " << int(percentage) << " %\r";
    std::cout.flush();
  }
  return 0;
}
int credentials_callback(git_cred **cred, const char *url,
                         const char *username_from_url,
                         unsigned int allowed_types, void *payload) {
  return 0;
}
int clone_repository(const char *repo_url, const char *local_path,
                     const char *proxy_opts) {
  git_libgit2_init();
  git_repository *repo = NULL;
  git_clone_options clone_opts = GIT_CLONE_OPTIONS_INIT;
  clone_opts.fetch_opts.proxy_opts = GIT_PROXY_OPTIONS_INIT;
  clone_opts.fetch_opts.proxy_opts.type = GIT_PROXY_AUTO;
  if (proxy_opts)
    clone_opts.fetch_opts.proxy_opts.url = proxy_opts;
  clone_opts.fetch_opts.callbacks.transfer_progress = transfer_progress;
  clone_opts.fetch_opts.callbacks.credentials = credentials_callback;
  clone_opts.fetch_opts.depth = 1;
  int error = git_clone(&repo, repo_url, local_path, &clone_opts);
  if (error != 0 && error != GIT_EEXISTS) {
    const git_error *git_error = giterr_last();
    printf("Error try cloning repository: %s\n", git_error->message);
  }
  git_repository_free(repo);
  git_libgit2_shutdown();
  return error;
}
int update_repository(const char *repo_path, const char *proxy_opts) {
  git_libgit2_init();
  git_repository *repo = NULL;
  int error = git_repository_open(&repo, repo_path);
  if (error) {
    std::cout << "open repo error " << error << std::endl;
    FINALIZE_GIT(error);
  }
  git_remote *remote = nullptr;
  error = git_remote_lookup(&remote, repo, "origin");
  if (error) {
    std::cout << "lookup remote origin error " << error << std::endl;
    FINALIZE_GIT(error);
  }
  git_fetch_options fetch_opts = GIT_FETCH_OPTIONS_INIT;
  if (proxy_opts)
    fetch_opts.proxy_opts.url = proxy_opts;
  fetch_opts.callbacks.transfer_progress = transfer_progress;
  error = git_remote_fetch(remote, NULL, &fetch_opts, NULL);
  if (error) {
    std::cout << "fetch remote origin error " << error << std::endl;
    FINALIZE_GIT(error);
  }
  git_reference *local_ref = nullptr;
  git_reference *origin_ref = nullptr;
  error = git_branch_lookup(&local_ref, repo, "master", GIT_BRANCH_LOCAL);
  if (error)
    std::cout << "error git_branch_lookup master" << std::endl;
  error =
      git_branch_lookup(&origin_ref, repo, "origin/master", GIT_BRANCH_REMOTE);
  if (error)
    std::cout << "error git_branch_lookup origin/master" << std::endl;
  git_annotated_commit *local_commit = nullptr;
  git_annotated_commit *origin_commit = nullptr;
  git_annotated_commit_from_ref(&local_commit, repo, local_ref);
  git_annotated_commit_from_ref(&origin_commit, repo, origin_ref);
  git_oid local_oid;
  git_oid remote_oid;
  error =
      git_reference_name_to_id(&local_oid, repo, git_reference_name(local_ref));
  error = git_reference_name_to_id(&remote_oid, repo,
                                   git_reference_name(origin_ref));
  if (!std::equal(local_oid.id, local_oid.id + sizeof(local_oid),
                  remote_oid.id)) {
    error = git_graph_descendant_of(repo, &remote_oid, &local_oid);
    if (error == 1) {
      std::cout << "fastcforwardable " << std::endl;
      git_reference *updated_ref = nullptr;
      error = git_reference_set_target(&updated_ref, local_ref,
                                       git_annotated_commit_id(origin_commit),
                                       "Fast-forward");
    } else if (error < 0) {
      std::cout << "error git_graph_descendant_of " << error << std::endl;
      FINALIZE_GIT(error);
    } else if (!error) {
      std::cout << "not fastcforwardable " << std::endl;
      git_repository_set_head(repo, git_reference_name(origin_ref));
    }
  } else
    std::cout << "local is up to date.\n";
  // FINALIZE_GIT(0);
  git_repository_free(repo);
  git_libgit2_shutdown();
  return error;
}
int clone_or_update_repository(const char *repo_url, const char *local_path,
                               const char *proxy_opts) {
  int error = clone_repository(repo_url, local_path, proxy_opts);
  if (error == GIT_EEXISTS) {
    // std::cout << local_path << " repo exists, checking update ..." << std::endl;
    update_repository(local_path, proxy_opts);
  }
  std::cout << std::endl;
  return error;
}
// libgit2 relate functions ends
// list files in a directory
void list_files(const std::filesystem::path &directory) {
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_directory() && entry.path().filename().string().at(0) != '.')
      list_files(entry.path());
    else if (entry.is_regular_file())
      std::cout << entry.path().string() << std::endl;
  }
}
// list files in a directory to a vector for recipe
void list_files_to_vector(const std::filesystem::path &directory,
                          VString &files) {
  std::string pattern = "readme.md|authors|license";
  std::regex regex(pattern, std::regex_constants::icase);
  for (const auto &entry : std::filesystem::directory_iterator(directory)) {
    if (entry.is_directory() && entry.path().filename().string().at(0) != '.')
      list_files_to_vector(entry.path(), files);
    else if (entry.is_regular_file() &&
             !std::regex_match(entry.path().filename().string(), regex))
      files.push_back(entry.path().string());
  }
}
// delete directory
// fix me: some time not works under windows
void delete_directory(const std::string &_path) {
  std::filesystem::path path(parse_path(_path));
  if (!std::filesystem::exists(path))
    return;
  for (const auto &entry : std::filesystem::directory_iterator(path)) {
    if (entry.is_directory())
      delete_directory(entry.path().string());
    else
      std::filesystem::remove(entry.path().string());
  }
  std::filesystem::remove(path);
}
// copy file, create_directory automatically, overwrite_existing
bool copy_file(const std::string &source, const std::string &destination,
               const std::string &base) {
  try {
    std::filesystem::path src = parse_path(source);
    std::filesystem::path dest = parse_path(destination);
    std::filesystem::path base_ = parse_path(base);
    if (!std::filesystem::is_directory(dest)) {
      if (!std::filesystem::exists(dest.parent_path())) {
		  std::string tpath = dest.parent_path().string();
          // fixme: windows simplified Chinese codepage, create folder with space in the path cause exception, tested by iDvel/rime-ice
          // "rime-ice\\others\\iRime\\iRime 九宫格\\melt_eng.custom.yaml" }
		  std::filesystem::create_directory(tpath);
	  }
      std::filesystem::copy_file(
          src, dest, std::filesystem::copy_options::overwrite_existing);
    } else {
      std::filesystem::path rpath = std::filesystem::relative(src, base_);
      std::filesystem::path _path = destination / rpath;
      if (!std::filesystem::exists(_path.parent_path()))
        std::filesystem::create_directory(_path.parent_path());
      std::filesystem::copy_file(
          src, _path, std::filesystem::copy_options::overwrite_existing);
    };
    return true;
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Failed to copy file: " << e.what() << std::endl;
    return false;
  }
}
// delete file
bool delete_file(const std::string &_path) {
  try {
    std::filesystem::path path = parse_path(_path);
    std::filesystem::remove(path);
    return true;
  } catch (const std::filesystem::filesystem_error &e) {
    std::cerr << "Failed to delete file: " << e.what() << std::endl;
    return false;
  }
}
// install recipe repo
int install_recipe(const Recipe &recipe) {
  std::string repo_url = mirror + recipe.repo + ".git";
  std::string local_path = (cache_dir + sep + recipe.local_path);
  std::cout << "update recipe : " << recipe.repo << std::endl;
  VString files;
  int error = clone_or_update_repository(repo_url.c_str(), local_path.c_str(),
                                         proxy.c_str());
  list_files_to_vector(std::filesystem::path(local_path), files);
  for (const auto &file : files) {
	std::string target_path = user_dir + sep + std::filesystem::relative(file, local_path).string();
    copy_file(file, target_path, local_path);
	std::cout << "installed: " << target_path << std::endl;
  }
  VString().swap(files);
  if (recipe.dependencies.size()) {
    for (const auto &dep : recipe.dependencies) {
      repo_url = mirror + dep + ".git";
      size_t pos = dep.find("/");
      std::string local_path = dep.substr(pos + 1);
      local_path = (cache_dir + sep + local_path);
      std::cout << "update dependency : " << dep << std::endl;
      error = clone_or_update_repository(repo_url.c_str(), local_path.c_str(),
                                         proxy.c_str());
      list_files_to_vector(std::filesystem::path(local_path), files);
      for (const auto &file : files) {
		std::string target_path = user_dir + sep + std::filesystem::relative(file, local_path).string();
        copy_file(file, target_path, local_path);
        std::cout << "installed: " << target_path << std::endl;
      }
      VString().swap(files);
    }
  }
  if (recipe.reverseDependencies.size()) {
    for (const auto &dep : recipe.reverseDependencies) {
      repo_url = mirror + dep + ".git";
      size_t pos = dep.find("/");
      std::string local_path = dep.substr(pos + 1);
      local_path = (cache_dir + sep + local_path);
      std::cout << "update reverseDependency : " << dep << std::endl;
      error = clone_or_update_repository(repo_url.c_str(), local_path.c_str(),
                                         proxy.c_str());
      list_files_to_vector(std::filesystem::path(local_path), files);
      for (const auto &file : files) {
		std::string target_path = user_dir + sep + std::filesystem::relative(file, local_path).string();
        copy_file(file, target_path, local_path);
        std::cout << "installed: " << target_path << std::endl;
      }
      VString().swap(files);
    }
  }
  return error;
}
// update rppi cache
int update_rppi(std::string local_path, std::string mirror, std::string proxy) {
  std::string repo_url = mirror + "rime/rppi.git";
  return clone_or_update_repository(
      mirror.empty() ? "https://github.com/rime/rime.git" : repo_url.c_str(),
      local_path.c_str(), proxy.empty() ? nullptr : proxy.c_str());
}
// load a json file
json load_json(const std::string &file_path) {
  std::ifstream file(file_path);
  json j;
  file >> j;
  file.close();
  return j;
}
// parse rppi index.json to get Recipe vector
void parse_index_rppi(const std::string &file_dir, const std::string &category,
                      std::vector<Recipe> &recipes) {
  json j = load_json(file_dir + "/index.json");
  if (j.contains("recipes")) {
    for (const auto &recipe : j["recipes"]) {
      Recipe _recipe;
      _recipe.repo = recipe["repo"];
      size_t pos = _recipe.repo.find("/");
      _recipe.local_path = _recipe.repo.substr(pos + 1);
      _recipe.name = recipe["name"];
      if (recipe.contains("labels"))
        for (const auto &l : recipe["labels"])
          _recipe.labels.push_back(l);
      if (recipe.contains("schemas"))
        for (const auto &s : recipe["schemas"])
          _recipe.schemas.push_back(s);
      if (recipe.contains("dependencies"))
        for (const auto &d : recipe["dependencies"])
          _recipe.dependencies.push_back(d);
      if (recipe.contains("reverseDependencies"))
        for (const auto &d : recipe["reverseDependencies"])
          _recipe.reverseDependencies.push_back(d);
      if (recipe.contains("license"))
        _recipe.license = recipe["license"];
      _recipe.category = category;
      recipes.push_back(_recipe);
    }
  } else if (j.contains("categories")) {
    for (const auto &cat : j["categories"]) {
      parse_index_rppi(file_dir + sep + std::string(cat["key"]),
                       category + sep + std::string(cat["key"]), recipes);
    }
  }
}
// join sring vector to strings seperated with space and quote with []
std::string join_string_vector(const VString &string_vector,
                               const std::string &_sep = " ",
                               const std::string &_start = "[",
                               const std::string &_end = "]") {
  std::string res = _start;
  bool start = true;
  for (const auto &str : string_vector) {
    res += _sep + str;
  }
  res += _sep + _end;
  return res;
}
// filter recipes with keyword, for recipe searching
std::vector<Recipe>
filter_recipes_with_keyword(const std::vector<Recipe> &recipes,
                            const std::string &keyword, bool strict = false) {
  std::vector<Recipe> res;
  const std::regex regex = std::regex(".*" + keyword + ".*");
  for (const auto &r : recipes) {
    if ((!strict &&
         (std::regex_match(r.name, regex) || std::regex_match(r.repo, regex) ||
          std::regex_match(r.category, regex))) ||
        (strict &&
         (r.repo == keyword || r.name == keyword || r.category == keyword)))
      res.push_back(r);
  }
  return res;
}
// print recipes info
void print_recipes(const std::vector<Recipe> &recipes) {
  for (const auto &r : recipes)
    std::cout << "name: " << r.name << ", repo: "
              << r.repo
              //<< ", category: " << r.category
              //<< ", schemas: " << join_string_vector(r.schemas)
              //<< ", dependencies: " << join_string_vector(r.dependencies)
              //<< ", reverseDependencies: " <<
              // join_string_vector(r.reverseDependencies)
              << std::endl;
}
// check if a file exist
#define file_exist(file_path)                                                  \
  std::filesystem::exists(std::filesystem::path(file_path))
// load rppi config
bool load_config() {
  std::string app_path = GetApplicationDirectory();
  std::string config_path = file_exist("~/rppi_config.yaml")
                                ? "~/rppi_config.yaml"
                                : app_path + "/rppi_config.yaml";
  if (!file_exist(config_path)) // no configure file found
    return false;
  // load proxy and mirror configuration
  YAML::Node config = YAML::LoadFile(config_path);
  if (config["proxy"])
    proxy = config["proxy"].as<std::string>();
  if (config["mirror"]) {
    mirror = config["mirror"].as<std::string>();
    if (mirror.back() != '/')
      mirror.append("/");
  } else {
    mirror = "https://github.com/";
  }
  if (config["user_dir"]) {
    user_dir = config["user_dir"].as<std::string>();
    if (user_dir.at(user_dir.length() - 1) == '/')
      user_dir = user_dir.substr(0, user_dir.length() - 1);
    user_dir = parse_path(user_dir);
  }
  if (config["cache_dir"]) {
    cache_dir = config["cache_dir"].as<std::string>();
    if (cache_dir.at(cache_dir.length() - 1) == '/')
      cache_dir = cache_dir.substr(0, cache_dir.length() - 1);
    cache_dir = parse_path(cache_dir);
  }
  return true;
}
// main function
int main(int argc, char **argv) {
  int codepage = SetConsoleOutputCodePage();
  bar_width = (int)(terminal_width() * 0.6); // setup processbar width
  load_config(); // load proxy and mirror configuration
  try {
    cxxopts::Options options("rppi_get", " - A toy to play with rppi");
    options.add_options()("h,help", "print help")("u,update", "update rppi")(
        "i,install", "install recipe",
        cxxopts::value<std::string>())("s,search", "search recipe with keyword",
                                       cxxopts::value<std::string>())(
        "c,clean", "clean caches")("v,verbose", "verbose settings")("l,list", "list recipes in rppi");
    auto result = options.parse(argc, argv);
    int retry = 0;
    if (result.count("verbose")) {
#ifdef _WIN32
      std::cout << "mirror: ";
      SetConsoleColor(FOREGROUND_GREEN);
      std::cout << mirror << std::endl;
      SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
      std::cout << "proxy: ";
      SetConsoleColor(FOREGROUND_GREEN);
      std::cout << proxy << std::endl;
      SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
      std::cout << "cache directory: ";
      SetConsoleColor(FOREGROUND_GREEN);
      std::cout << cache_dir << std::endl;
      SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
      std::cout << "user directory: ";
      SetConsoleColor(FOREGROUND_GREEN);
      std::cout << user_dir << std::endl;
      SetConsoleColor(FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
#else
      printf(
          "mirror: \033[32m%s\033[0m\nproxy: \033[32m%s\033[0m\ncache "
          "directory: \033[32m%s\033[0m\nuser directory: \033[32m%s\033[0m\n",
          mirror.c_str(), proxy.c_str(), cache_dir.c_str(), user_dir.c_str());
#endif
    }
    if (argc == 1 || result.count("help")) {
      std::cout << options.help() << std::endl;
      return 0;
    } else if (result.count("clean")) {
      std::filesystem::path directory = cache_dir;
      if (std::filesystem::exists(cache_dir))
        for (const auto &entry :
             std::filesystem::directory_iterator(directory)) {
          if (entry.is_directory() && entry.path().filename() != "." &&
              entry.path().filename() != "..")
            std::cout << "delete directory: " << entry.path() << std::endl;
          delete_directory(entry.path().string());
        }
      return 0;
    } else if (result.count("update")) {
    updaterppi:
      std::cout << "update rppi index" << std::endl;
      update_rppi(cache_dir + "/rppi", mirror, proxy);
      if (retry > 10)
        return 0;
    }
    std::vector<Recipe> recipes;
    if (file_exist(cache_dir + "/rppi/index.json")) {
      parse_index_rppi(cache_dir + "/rppi", "rppi", recipes);
    } else
      goto updaterppi;
    if (!recipes.size()) {
      std::cout << "update rppi index" << std::endl;
      update_rppi(cache_dir + "/rppi", mirror, proxy);
      std::vector<Recipe>().swap(recipes);
      return 0;
    }
    if (result.count("list")) {
		print_recipes(recipes);
	}
    if (result.count("install")) {
      std::string repo = result["install"].as<std::string>();
#ifdef _WIN32
      repo = _to_string(_to_wstring(repo, CP_ACP), CP_UTF8);
#endif
      std::vector<Recipe> res =
          filter_recipes_with_keyword(recipes, repo, true);
      if (res.size()) {
        std::cout << "install recipe by keyword : " << repo << std::endl;
        install_recipe(res.at(0));
      } else {
        std::cout << "install recipe by : "
                  << result["install"].as<std::string>() << " failed ||-_-"
                  << std::endl;
      }
      std::vector<Recipe>().swap(recipes);
      return 0;
    } else if (result.count("search")) {
      std::string repo = result["search"].as<std::string>();
#ifdef _WIN32
      repo = _to_string(_to_wstring(repo, CP_ACP), CP_UTF8);
#endif
      std::cout << "search recipe with keyword: " << repo << std::endl;
      std::vector<Recipe> res =
          filter_recipes_with_keyword(recipes, repo, false);
      print_recipes(res);
      std::vector<Recipe>().swap(recipes);
      return 0;
    }
  } catch (const std::exception &e) {
    std::cerr << "Error parsing options: " << e.what() << std::endl;
    return 1;
  }
  SetConsoleOutputCodePage(codepage);
  return 0;
}
