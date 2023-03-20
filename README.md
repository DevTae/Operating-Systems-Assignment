# Operating-Systems-Assignment
This is the repository about the assignment in *Operating Systems Lecture (COM2005)*

<br/>

**\<How to execute\>**

```
#\> gcc tsh.c
```

<br/>

**\<How to test codes\>**

- Basic Operations
```Bash
tsh> grep int tsh.c
tsh> grep "if.*MULL" tsh.c &
tsh> ps
```

- Standard IO Redirection
```Bash
tsh> grep "int " < tsh.c
tsh> ls -l
tsh> ls -l > delme
tsh> cat delme
tsh> sort < delme > delme2
tsh> cat delme2
```

- Pipe
```Bash
tsh> ps -A | grep -i system
tsh> ps -A | grep -i system | awk ’{print $1,$4}’
tsh> cat tsh.c | head -6 | tail -5 | head -1
```

- Combination
```Bash
tsh> sort < tsh.c | grep "int " | awk ’{print $1,$2}’ > delme3
```

-----

The End.

Thank you for reading!
