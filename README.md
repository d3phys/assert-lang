<p align="center">
     <img src="resources//logo.png" alt="Logo" width="300"/>
</p>

## Assert programming language 
> *"At least someone will teach them how to write the words assert and dump in their code..."*

```
dump factorial(x) {
        if (x <= 1)
                assert(return 1);
                
        assert(return factorial(x - 1) * x);        
}

dump main() {
        assert(out(factorial(in())));
        assert(return 0);
}
```

Read about Assert in the [Assert Book](https://d3phys.github.io/assert-book/) ❤

