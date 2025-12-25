[![progress-banner](https://backend.codecrafters.io/progress/shell/bcdceccf-0c5b-4e41-b8ac-4f5ee536eeb4)](https://app.codecrafters.io/users/codecrafters-bot?r=2qF)

# kubsh — Реализация POSIX Shell на C++17

Кастомная командная оболочка с поддержкой виртуальной файловой системы через FUSE.

## Установка

### Из DEB пакета
```bash
sudo apt-get update
sudo apt-get install -y ./kubsh.deb
```

Пакет автоматически установит зависимость: `libfuse3-3` или `libfuse3-4`

### Из исходного кода
```bash
# Установка зависимостей для сборки
sudo apt-get install -y build-essential libfuse3-dev

# Сборка и тестирование
make build
make deb
sudo dpkg -i kubsh.deb
```

## Быстрый тест

```bash
kubsh
$ echo "Привет, мир!"
Привет, мир!
$ \q
```

## Сборка

```bash
make build    # Компилировать оболочку
make run      # Запустить интерактивно
make deb      # Создать DEB пакет
make clean    # Удалить артефакты сборки
```

## Возможности

- REPL оболочка с историей команд (`~/kubsh_history`)
- Встроенные команды: `echo`, `debug`, `\e $VAR`, `\l /dev/device`, `history`, `\q`
- Полная поддержка запуска внешних команд
- Виртуальная файловая система FUSE по адресу `/users/{username}/{id|home|shell}`
- Обработка сигналов (SIGHUP)
- Парсинг строк в одинарных кавычках

## Структура проекта

```
src/
├── main.cpp     # Реализация REPL оболочки
└── vfs.cpp      # Виртуальная файловая система FUSE

kubsh-package/  # Файлы DEB пакета
```

## Зависимости

- **Runtime**: `libfuse3-3` или `libfuse3-4`
- **Сборка**: `g++` (C++17), `make`, `libfuse3-dev`

---

**Исходное задание**: [Build Your Own Shell](https://app.codecrafters.io/courses/shell/overview)
