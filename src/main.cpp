#include <iostream>
#include <string>

int main() {
	std::cout << std::unitbuf;
	std::cerr << std::unitbuf;

	std::cout << "$ ";

	//печатаем введенную строку и выводим
	std::string a;
	std::cin >> a;
	std::cout << "введенная строка: " << a << std::endl;
}
