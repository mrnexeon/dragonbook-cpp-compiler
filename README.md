## The Dragon Book compiler on C++

Синтаксически управляемый транслятор на базе метода рекурсивного спуска для генерации промежуточного трехадресного кода с возможностью экспорта АСД в JSON и GraphViz (Dot) файлы

### Использование

```cpp
Usage: <app_name> input_file [options]
   -j, --json filepath     output ast to json in filepath
   -d, --dot filepath      output ast to dot in filepath
```

### Пример

Пример вывода следующей программы:

```
{
	int i;
	i = 0;
	while (i < 100) { 
		i = i + 1;
	}
}
```

Трёхадресный код:
```
L1:     i = 0
L3:     iffalse i < 100 goto L2
L4:     i = i + 1
        goto L3
L2:
```

АСД в GraphViz:
![AST GraphViz sample](ast.png "AST GraphViz")
