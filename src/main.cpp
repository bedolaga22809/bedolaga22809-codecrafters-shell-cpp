#include <iostream>
#include <string>
#include <fstream>
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



int main() {
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;
	std::cout << GREEN << "$ " << RESET;
	std::string a;

    std::string homePapka = searchF();
    std::string history = homePapka + "/.kubsh_history";

	while (true) {
		std::getline(std::cin, a);

		if (std::cin.eof() || a == "\\q") {
			break;
		}
        else if (a == "\\history") {
            historyF(history);
        }
        else if(a == "\\delhistory") {
            delHistoryF(history);
        }
        else {
            std::ofstream file(history, std::ios::app);
            if (file.is_open()) {
                file << a << std::endl;
                file.close();
            }
        }
        std::cout << a << std::endl;
        std::cout << GREEN << "$ " << RESET;
	}
}
