#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>
#include <vector>
#include <poll.h>
#include <cstring>
#include <pwd.h>
#include <errno.h>

#define FUSE_USE_VERSION 31
#include <fuse3/fuse.h>

#define RESET   "\033[0m"
#define GREEN   "\033[32m"
#define MAX_USERS 1000

static int vfs_pid = -1;

//поиск домашней папки
std::string searchF() {
    std::string home;
    if(getenv("HOME")) {
        home = getenv("HOME");
    }
    else {
        home = ".";
    }
    return home;
}

//итсория команд
void historyF(std::string& history) {
    std::ifstream file(history);
    std::string strok;
    std::cout << "история: " << std::endl;
    while (std::getline(file, strok)){
        std::cout << strok << std::endl;
    }
    file.close();
}

//удаление истории
void delHistoryF(std::string& history) {
    std::remove(history.c_str());
}

//разбор строки на аргументы
std::vector<std::string> parseArgs(const std::string& cmd) {
    std::vector<std::string> args;
    std::string current;
    bool inQuote = false;
    
    for (size_t i = 0; i < cmd.length(); i++) {
        char c = cmd[i];
        if (c == '\'' || c == '"') {
            inQuote = !inQuote;
        }
        else if (c == ' ' && !inQuote) {
            if (!current.empty()) {
                args.push_back(current);
                current.clear();
            }
        }
        else {
            current += c;
        }
    }
    if (!current.empty()) {
        args.push_back(current);
    }
    return args;
}

//выполнение внешней команды
void executeCommand(const std::string& cmd) {
    std::vector<std::string> args = parseArgs(cmd);
    if (args.empty()) return;
    
    pid_t pid = fork();
    if (pid == 0) {
        // дочерний процесс
        std::vector<char*> argv;
        for (auto& arg : args) {
            argv.push_back(const_cast<char*>(arg.c_str()));
        }
        argv.push_back(nullptr);
        
        execvp(argv[0], argv.data());
        // если execvp вернулся, команда не найдена
        exit(1);
    }
    else if (pid > 0) {
        // родительский процесс
        int status;
        waitpid(pid, &status, 0);
        if (WIFEXITED(status) && WEXITSTATUS(status) == 1) {
            std::cout << args[0] << ": command not found" << std::endl;
        }
    }
}

// FUSE VFS функции
struct user_info {
    char name[256];
    char uid[32];
    char home[256];
    char shell[256];
};

static struct user_info users[MAX_USERS];
static int users_count = 0;

int get_users_list() {
    users_count = 0;
    std::ifstream passwd("/etc/passwd");
    std::string line;
    
    while (std::getline(passwd, line) && users_count < MAX_USERS) {
        if (line.find("sh") != std::string::npos && 
            (line.find("/bin/bash") != std::string::npos || 
             line.find("/bin/sh") != std::string::npos ||
             line.find("/usr/bin/zsh") != std::string::npos)) {
            std::vector<std::string> fields;
            std::stringstream ss(line);
            std::string field;
            while (std::getline(ss, field, ':')) {
                fields.push_back(field);
            }
            
            if (fields.size() >= 7) {
                strncpy(users[users_count].name, fields[0].c_str(), 255);
                strncpy(users[users_count].uid, fields[2].c_str(), 31);
                strncpy(users[users_count].home, fields[5].c_str(), 255);
                strncpy(users[users_count].shell, fields[6].c_str(), 255);
                users_count++;
            }
        }
    }
    passwd.close();
    return users_count;
}

void free_users_list() {
    users_count = 0;
}

static int users_readdir(
    const char *path, 
    void *buf, 
    fuse_fill_dir_t filler,
    off_t offset,
    struct fuse_file_info *fi,
    enum fuse_readdir_flags flags
) {
    if (strcmp(path, "/") != 0)
        return -ENOENT;
    
    get_users_list();
    
    filler(buf, ".", NULL, 0, (fuse_fill_dir_flags)0);
    filler(buf, "..", NULL, 0, (fuse_fill_dir_flags)0);
    
    for (int i = 0; i < users_count; i++) {
        filler(buf, users[i].name, NULL, 0, (fuse_fill_dir_flags)0);
    }
    
    return 0;
}

static int users_open(const char *path, struct fuse_file_info *fi) {
    return 0;
}

static int users_read(
    const char *path, 
    char *buf, 
    size_t size,
    off_t offset,
    struct fuse_file_info *fi
) {
    std::string spath(path);
    get_users_list();
    
    for (int i = 0; i < users_count; i++) {
        std::string user_dir = std::string("/") + users[i].name;
        std::string content;
        
        if (spath == user_dir + "/id") {
            content = users[i].uid;
        } else if (spath == user_dir + "/home") {
            content = users[i].home;
        } else if (spath == user_dir + "/shell") {
            content = users[i].shell;
        }
        
        if (!content.empty()) {
            size_t len = content.length();
            if (offset < (off_t)len) {
                if (offset + size > len)
                    size = len - offset;
                memcpy(buf, content.c_str() + offset, size);
            } else {
                size = 0;
            }
            return size;
        }
    }
    
    return -ENOENT;
}

static int users_getattr(const char *path, struct stat *stbuf,
                         struct fuse_file_info *fi) {
    memset(stbuf, 0, sizeof(struct stat));
    
    if (strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
        return 0;
    }
    
    std::string spath(path);
    get_users_list();
    
    for (int i = 0; i < users_count; i++) {
        std::string user_dir = std::string("/") + users[i].name;
        if (spath == user_dir) {
            stbuf->st_mode = S_IFDIR | 0755;
            stbuf->st_nlink = 2;
            return 0;
        }
        
        if (spath == user_dir + "/id" || 
            spath == user_dir + "/home" || 
            spath == user_dir + "/shell") {
            stbuf->st_mode = S_IFREG | 0444;
            stbuf->st_nlink = 1;
            stbuf->st_size = 100;
            return 0;
        }
    }
    
    return -ENOENT;
}

static int users_mkdir(const char *path, mode_t mode) {
    std::string spath(path);
    if (spath[0] == '/') spath = spath.substr(1);
    
    // добавляем пользователя в /etc/passwd
    std::ofstream passwd("/etc/passwd", std::ios::app);
    if (passwd.is_open()) {
        passwd << spath << ":x:1000:1000::/home/" << spath << ":/bin/bash" << std::endl;
        passwd.flush();
        passwd.close();
    }
    
    // обновляем список пользователей
    get_users_list();
    
    return 0;
}

static struct fuse_operations users_oper = {};

void init_fuse_operations() {
    memset(&users_oper, 0, sizeof(users_oper));
    users_oper.getattr = users_getattr;
    users_oper.open = users_open;
    users_oper.read = users_read;
    users_oper.readdir = users_readdir;
    users_oper.mkdir = users_mkdir;
}

int start_users_vfs(const char *mount_point) {
    int pid = fork();    
    if (pid == 0) {
        char *fuse_argv[] = {
            (char*)"users_vfs",
            (char*)"-f",
            (char*)"-s",
            (char*)mount_point,
            NULL
        };
        
        if (get_users_list() <= 0) {
            fprintf(stderr, "Не удалось получить список пользователей\n");
            exit(1);
        }
        
        init_fuse_operations();
        int ret = fuse_main(4, fuse_argv, &users_oper, NULL);
        
        free_users_list();
        exit(ret);
    } else {
        vfs_pid = pid;
        return 0;
    }
}

void stop_users_vfs() {
    if (vfs_pid != -1) {
        kill(vfs_pid, SIGTERM);
        waitpid(vfs_pid, NULL, 0);
        vfs_pid = -1;
    }
}

volatile sig_atomic_t sigReceived = 0;

void signalHandler(int signum) {
    sigReceived = signum;
}

int main() {
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    
    // обработка сигналов
    signal(SIGHUP, signalHandler);
    
    std::string homePapka = searchF();
    std::string history = homePapka + "/.kubsh_history";
    
    // запускаем VFS
    start_users_vfs("/opt/users");
    sleep(1); // даем время VFS запуститься
    
    std::cout << GREEN << "$ " << RESET << std::flush;
    
    while (true) {
        // проверяем сигналы
        if (sigReceived == SIGHUP) {
            std::cout << std::endl << "Configuration reloaded" << std::endl;
            std::cout << GREEN << "$ " << RESET << std::flush;
            sigReceived = 0;
        }
        
        // проверяем наличие данных для чтения
        struct pollfd pfd;
        pfd.fd = STDIN_FILENO;
        pfd.events = POLLIN;
        
        int ret = poll(&pfd, 1, 100);
        
        if (ret > 0) {
            std::string a;
            std::getline(std::cin, a);
            
            if (std::cin.eof() || a == "\\q") {
                break;
            }
            else if (a == "history") {
                historyF(history);
            }
            else if(a == "delhis") {
                delHistoryF(history);
            }
            else if(a.find("\\e ") == 0) {
                // вывод переменных окружения
                std::string varName = a.substr(3);
                if (varName[0] == '$') {
                    varName = varName.substr(1);
                }
                const char* value = getenv(varName.c_str());
                if (value) {
                    std::cout << std::endl;
                    std::string val(value);
                    // если PATH, то выводим построчно
                    if (varName == "PATH") {
                        std::stringstream ss(val);
                        std::string path;
                        while (std::getline(ss, path, ':')) {
                            std::cout << path << std::endl;
                        }
                    }
                    else {
                        std::cout << val << std::endl;
                    }
                }
            }
            else if(a.find("debug ") == 0) {
                std::string save = a;
                std::string answ = a.substr(6);
                // убираем кавычки если есть
                if (!answ.empty() && answ.front() == '\'' && answ.back() == '\'') {
                    answ = answ.substr(1, answ.length() - 2);
                }
                std::cout << std::endl << answ << std::endl;
                //для корректного сохранения в истории
                std::ofstream file(history, std::ios::app);
                if (file.is_open()) {
                    file << save << std::endl;
                    file.close();
                }
            }
            else if(a.find("echo ") == 0) {
                std::string save = a;
                std::string answ = a.substr(5);
                std::cout << std::endl << answ << std::endl;
                //для корректного сохранения в истории
                std::ofstream file(history, std::ios::app);
                if (file.is_open()) {
                    file << save << std::endl;
                    file.close();
                }
            }
            else if (!a.empty()) {
                // выполнение внешней команды
                std::cout << std::endl;
                executeCommand(a);
                std::ofstream file(history, std::ios::app);
                if (file.is_open()) {
                    file << a << std::endl;
                    file.close();
                }
            }
            
            if (!std::cin.eof()) {
                std::cout << GREEN << "$ " << RESET << std::flush;
            }
        }
    }
    
    stop_users_vfs();
    return 0;
}