RRPOISON
========
 * DNS Resource Recode Poisoning Software

```
# cd rrpoison
# cmake .
# make
```

```
rrpoison
MUST
-s [source address]
-d [destination address]
-a [dns ans address]
-r [dns req name]
OPTION
-c [count number]  : 0 is loop   (default:0)
-x [target port]   : 0 is random (default:0)
-y [target dns_is] : 0 is random (default:0)

If you want to spoof the source address, you set a same option (-s) alias address to the sending IF.
Example: ifconfig lo0 alias x.x.x.x.

```

* Depend
 - MacOSX  o
 - FreeBSD o
 - Linux   x

## Contributing

1. Fork it
2. Create your feature branch (`git checkout -b new-branch-name`)
3. Commit your changes (`git commit -am 'Add comment at some your new features'`)
4. Push to the branch (`git push origin new-branch-name`)
5. Create new Pull Request


