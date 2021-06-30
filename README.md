# Blue Archive Spine exporter

## Usage
```sh
out/exporter <input directory> <output directory> <image format> <scale>
```

`<input directory>` default is current directory

`<output directory>` default is 'result'

`<image format>` default is 'png', based on [SFML Image](https://www.sfml-dev.org/documentation/2.5.1/classsf_1_1Image.php#a51537fb667f47cbe80395cfd7f9e72a4) supports (which current are bmp, png, tga and jpg)

`<scale>` default is 0.5f


## Requirement
`gcc`, `sfml` libraries


## Build
```sh
sh build.sh
```


## Library used
[spine-runtimes](https://github.com/EsotericSoftware/spine-runtimes)

[SFML](https://sfml-dev.org)
