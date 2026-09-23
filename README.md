# wish — простий Unix shell на C++

Проєкт з курсу «Операційні системи»: реалізація shell `wish` за специфікацією
[OSTEP processes-shell](https://github.com/remzi-arpacidusseau/ostep-projects/tree/master/processes-shell).

## Структура

```
tester/run-tests.sh        офіційний скрипт запуску тестів (ostep-projects)
processes-shell/
  wish.cpp                 вихідний код shell
  Makefile
  test-wish.sh             запуск офіційних тестів
  tests/                   офіційні тести (1..22)
```

## Збірка і тести (Linux / WSL)

```sh
cd processes-shell
make
./test-wish.sh        # або: make test (продовжує після першого провалу)
```

## Етапи розробки

| Тег  | Етап                                              |
|------|---------------------------------------------------|
| `v0` | Каркас репозиторію, Makefile, офіційні тести      |
| `v1` | Цикл REPL, interactive/batch режими, помилки      |
| `v2` | Парсинг рядка, запуск програм через fork/execv    |
| `v3` | Вбудовані команди `exit`, `cd`, `path`            |
| `v4` | Перенаправлення виводу `>`                        |
| `v5` | Паралельні команди `&`                            |
| `v6` | Усі тести проходять, рефакторинг, документація    |
