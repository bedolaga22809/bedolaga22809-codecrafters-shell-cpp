#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <sstream>
#include <vector>
#include <filesystem>
#include <thread>
#include <chrono>
#include <set>
#include <poll.h>

#define RESET   "\033[0m"
#define GREEN   "\033[32m"

namespace fs = std::filesystem;

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

//синхронизация VFS с /etc/passwd
void syncVFS() {
    const std::string vfsPath = "/opt/users";
    
    // проверяем существование и доступ
    if (!fs::exists(vfsPath)) {
        return;
    }
    
    try {
        // читаем существующие папки в VFS
        std::set<std::string> vfsDirs;
        for (const auto& entry : fs::directory_iterator(vfsPath)) {
            if (entry.is_directory()) {
                vfsDirs.insert(entry.path().filename().string());
            }
        }
        
        // читаем пользователей из /etc/passwd с shell
        std::set<std::string> passwdUsers;
        std::ifstream passwd("/etc/passwd");
        std::string line;
        while (std::getline(passwd, line)) {
            if (line.find("sh") != std::string::npos && 
                (line.find("/bin/bash") != std::string::npos || 
                 line.find("/bin/sh") != std::string::npos ||
                 line.find("/usr/bin/zsh") != std::string::npos)) {
                size_t pos = line.find(':');
                if (pos != std::string::npos) {
                    std::string username = line.substr(0, pos);
                    passwdUsers.insert(username);
                }
            }
        }
        passwd.close();
        
        // добавляем пользователей в /etc/passwd для новых папок СНАЧАЛА
        for (const auto& dir : vfsDirs) {
            if (passwdUsers.find(dir) == passwdUsers.end()) {
                // добавляем нового пользователя
                std::ofstream passwdOut("/etc/passwd", std::ios::app);
                if (passwdOut.is_open()) {
                    passwdOut << dir << ":x:1000:1000::/home/" << dir << ":/bin/bash" << std::endl;
                    passwdOut.close();
                }
                passwdUsers.insert(dir);
            }
        }
        
        // теперь добавляем недостающие папки для пользователей из passwd
        passwd.open("/etc/passwd");
        while (std::getline(passwd, line)) {
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
                    std::string username = fields[0];
                    std::string uid = fields[2];
                    std::string home = fields[5];
                    std::string shell = fields[6];
                    
                    if (vfsDirs.find(username) == vfsDirs.end()) {
                        try {
                            fs::create_directory(vfsPath + "/" + username);
                        } catch (...) {}
                    }
                    
                    // создаем файлы в папке пользователя
                    std::string userPath = vfsPath + "/" + username;
                    if (fs::exists(userPath)) {
                        std::ofstream idFile(userPath + "/id");
                        if (idFile.is_open()) {
                            idFile << uid;
                            idFile.close();
                        }
                        
                        std::ofstream homeFile(userPath + "/home");
                        if (homeFile.is_open()) {
                            homeFile << home;
                            homeFile.close();
                        }
                        
                        std::ofstream shellFile(userPath + "/shell");
                        if (shellFile.is_open()) {
                            shellFile << shell;
                            shellFile.close();
                        }
                    }
                }
            }
        }
        passwd.close();
    } catch (...) {
        // игнорируем все ошибки доступа
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
    
    // запускаем поток для мониторинга VFS
    std::thread vfsThread([&]() {
        while (true) {
            syncVFS();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    });
    vfsThread.detach();
    
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
                    std::cout << std::endl;  // перенос строки ДО вывода
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
                std::cout << std::endl << answ << std::endl;  // перенос строки ДО вывода
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
                std::cout << std::endl << answ << std::endl;  // перенос строки ДО вывода
                //для корректного сохранения в истории
                std::ofstream file(history, std::ios::app);
                if (file.is_open()) {
                    file << save << std::endl;
                    file.close();
                }
            }
            else if (!a.empty()) {
                // выполнение внешней команды
                std::cout << std::endl;  // перенос строки перед выполнением команды
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
    
    return 0;
}
