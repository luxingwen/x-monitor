打开动态log

```
 echo "module interrupt_test -p" > /sys/kernel/debug/dynamic_debug/control
 echo "module interrupt_test +p" > /sys/kernel/debug/dynamic_debug/control
```

