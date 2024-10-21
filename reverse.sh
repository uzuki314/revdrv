#/usr/bin/env sh
if [ -z "$(lsmod | grep revdrv)" ]
then
    echo "make sure to insmod first."
    exit 1
fi

echo "enter the string to be reversed:"
read string
echo -n "$string" | sudo tee /dev/reverse > /dev/null
sudo cat /dev/reverse && echo
exit 0
