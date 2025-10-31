#include <iostream>
#include <string>

int main() {
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	std::cout << "$ ";

	//печатаем введенную строку и выводим в цикле
	std::string a;

	while (true) {
		std::getline(std::cin, a);

		if (std::cin.eof() || a == "\\q") {
			break;
		}
		else {
			std::cout << a << std::endl;
			std::cout << "$ ";
		}
	}
}
