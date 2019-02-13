Create Image with
```Bash
truncate --size 10000 mram-image.bin
```

Use Image with
```Bash
make sim
```

Mount image with
```Bash
sudo mount -o loop,offset=4096 mram-image.bin /mnt/image
```
