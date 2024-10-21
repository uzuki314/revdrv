# revdrv

Linux kernel module that creates device /dev/reverse. 
Writing to this device will reverse the order.

## run check
Make sure uefi secure boot is disable, and `linux-headers gcc/clang make` installed

kernel compiled with gcc
```
make check
```
compiled with clang
```
env LLVM=1 make check
```
## interactive
Make sure insmod first
`make load` or `sudo insmod revdrv.ko`
then run `./reverse.sh`

## References
* [The Linux Kernel Module Programming Guide](https://sysprog21.github.io/lkmpg/)
* [Writing a simple device driver](https://www.apriorit.com/dev-blog/195-simple-driver-for-linux-os)
* [Character device drivers](https://linux-kernel-labs.github.io/refs/heads/master/labs/device_drivers.html)
* [cdev interface](https://lwn.net/Articles/195805/)
* [Character device files](https://sysplay.in/blog/linux-device-drivers/2013/06/character-device-files-creation-operations/)

## License

`revdrv` is released under the MIT license. Use of this source code is governed by
a MIT-style license that can be found in the LICENSE file.

External source code:
* [git-good-commit](https://github.com/tommarshall/git-good-commit): MIT License
