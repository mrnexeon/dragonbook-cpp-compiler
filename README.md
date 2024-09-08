## The Dragon Book Compiler in C++

A syntax-directed translator based on the recursive descent method for generating intermediate three-address code with the ability to export AST to JSON and GraphViz (Dot) files.

### Usage

```bash
Usage: <app_name> input_file [options]
   -j, --json filepath     output ast to json in filepath
   -d, --dot filepath      output ast to dot in filepath
```

### Example

An example output of the following program:

```
{
	int i;
	i = 0;
	while (i < 100) { 
		i = i + 1;
	}
}
```

Three-address code:

```
L1:     i = 0
L3:     iffalse i < 100 goto L2
L4:     i = i + 1
        goto L3
L2:
```

AST in GraphViz:

<img src="ast.png" alt="ast" width="200"/>
