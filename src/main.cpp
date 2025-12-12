#include <iostream>
#include <string>
#include <fstream>
#include <cstdlib>
#include <unistd.h>
#include <sys/wait.h>
#include <signal.h>
#include <vector>
#include <sstream>
#include <cstring>

#define RESET   "\033[0m"
#define GREEN   "\033[32m"

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

//история команд
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

//выполнение внешней команды
int executeCommand(std::string& cmd) {
    std::vector<char*> args;
    std::string token;
    std::istringstream iss(cmd);
    
    while (iss >> token) {
        char* arg = new char[token.size() + 1];
        std::strcpy(arg, token.c_str());
        args.push_back(arg);
    }
    args.push_back(nullptr);
    
    pid_t pid = fork();
    
    if (pid == 0) {
        // Дочерний процесс
        execvp(args[0], args.data());
        std::cerr << args[0] << ": command not found" << std::endl;
        exit(127);
    }
    else if (pid > 0) {
        // Родительский процесс
        int status;
        waitpid(pid, &status, 0);
        
        // Освобождаем память
        for (char* arg : args) {
            if (arg != nullptr) {
                delete[] arg;
            }
        }
        
        return WEXITSTATUS(status);
    }
    else {
        // Ошибка fork
        std::cerr << "Ошибка при создании процесса" << std::endl;
        return -1;
    }
}

//обработка переменных окружения
std::string expandVariables(std::string& input) {
    std::string result;
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == '$' && i + 1 < input.size()) {
            std::string varName;
            i++;
            
            // Имя переменной (буквы, цифры, подчеркивания)
            while (i < input.size() && (isalnum(input[i]) || input[i] == '_')) {
                varName += input[i];
                i++;
            }
            i--; // Возвращаемся к последнему символу
            
            // Получаем значение переменной
            char* value = getenv(varName.c_str());
            if (value != nullptr) {
                result += value;
            }
            else {
                result += "$" + varName;
            }
        }
        else {
            result += input[i];
        }
    }
    return result;
}

//команда debug (аналог echo из тестов)
void handleDebug(std::string& a) {
    // debug 'text' - удаляем debug и кавычки
    std::string text = a;
    
    // Удаляем "debug"
    size_t debug_pos = text.find("debug");
    if (debug_pos != std::string::npos) {
        text.erase(debug_pos, 5);
    }
    
    // Удаляем пробелы в начале
    size_t first_not_space = text.find_first_not_of(" ");
    if (first_not_space != std::string::npos) {
        text = text.substr(first_not_space);
    }
    
    // Удаляем одинарные кавычки если есть
    if (!text.empty() && text.front() == '\'') {
        text.erase(0, 1);
    }
    if (!text.empty() && text.back() == '\'') {
        text.pop_back();
    }
    
    std::cout << text << std::endl;
}

//команда \e (вывод переменной окружения)
void handleEchoEnv(std::string& a) {
    if (a.length() > 3) {
        std::string var = a.substr(3); // "\e "
        
        // Удаляем пробелы в начале
        size_t first_not_space = var.find_first_not_of(" ");
        if (first_not_space != std::string::npos) {
            var = var.substr(first_not_space);
        }
        
        // Удаляем $ если он есть в начале
        if (!var.empty() && var.front() == '$') {
            var.erase(0, 1);
        }
        
        char* value = getenv(var.c_str());
        if (value != nullptr) {
            std::cout << value << std::endl;
        }
        else {
            std::cout << "" << std::endl;
        }
    }
}

//обработчик сигналов
volatile sig_atomic_t signal_received = 0;

void signalHandler(int sig) {
    signal_received = sig;
}

int main() {
    // Устанавливаем обработчики сигналов
    signal(SIGHUP, signalHandler);
    signal(SIGINT, signalHandler);
    signal(SIGTERM, signalHandler);
    
    std::cout << std::unitbuf;
    std::cerr << std::unitbuf;
    std::string a;

    std::string homePapka = searchF();
    std::string history = homePapka + "/.kubsh_history";
    
    // Создаем файл истории если его нет
    std::ofstream createFile(history, std::ios::app);
    createFile.close();

    while (true) {
        // Выводим приглашение каждый раз
        std::cout << GREEN << "$ " << RESET << std::flush;
        
        // Проверяем сигналы
        if (signal_received == SIGHUP) {
            std::cout << "\nПолучен сигнал SIGHUP, завершение..." << std::endl;
            break;
        }
        
        std::getline(std::cin, a);
        
        if (std::cin.eof() || a == "\\q") {
            break;
        }
        else if (signal_received != 0) {
            // Если получили другой сигнал
            std::cout << "\nПолучен сигнал, завершение..." << std::endl;
            break;
        }
        else if (a == "\\history") {
            historyF(history);
        }
        else if (a == "\\delhistory") {
            delHistoryF(history);
        }
        else if (a.find("debug") == 0) {
            std::string save = a;
            handleDebug(a);
            // Сохраняем оригинальную команду в истории
            std::ofstream file(history, std::ios::app);
            if (file.is_open()) {
                file << save << std::endl;
                file.close();
            }
        }
        else if (a.find("\\e ") == 0) {
            std::string save = a;
            handleEchoEnv(a);
            // Сохраняем в истории
            std::ofstream file(history, std::ios::app);
            if (file.is_open()) {
                file << save << std::endl;
                file.close();
            }
        }
        else if (a == "\\e") {
            // Просто \e без аргументов - выводим все переменные окружения
            std::string save = a;
            extern char** environ;
            for (char** env = environ; *env != nullptr; env++) {
                std::cout << *env << std::endl;
            }
            // Сохраняем в истории
            std::ofstream file(history, std::ios::app);
            if (file.is_open()) {
                file << save << std::endl;
                file.close();
            }
        }
        else if (a.empty()) {
            // Пустая строка - просто новая строка с приглашением
            continue;
        }
        else {
            // Внешняя команда
            std::string save = a;
            
            // Проверяем, не пытается ли пользователь запустить debug как внешнюю команду
            if (a.find("debug") == 0) {
                // debug - это внутренняя команда, выводим ошибку
                std::string cmdName;
                std::istringstream iss(a);
                iss >> cmdName;
                std::cerr << cmdName << ": command not found" << std::endl;
            }
            else {
                // Сохраняем в истории ДО выполнения
                std::ofstream file(history, std::ios::app);
                if (file.is_open()) {
                    file << save << std::endl;
                    file.close();
                }
                
                // Выполняем команду
                int result = executeCommand(a);
                
                // Если команда не найдена, выводим сообщение
                if (result == 127) {
                    std::string cmdName;
                    std::istringstream iss(a);
                    iss >> cmdName;
                    std::cerr << cmdName << ": command not found" << std::endl;
                }
            }
        }
    }
    
    return 0;
}

