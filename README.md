kstem
=====
Clone of Krovetz stemmer, originally distributed [here](http://lexicalresearch.com/software.html).

## Usage

```
> echo hollies | kstem
holly

> echo the cats run up the hills | kstem
the cat run up the hill

```

## Notes

Builds on OSX.  

```
cd kstem
make
make install
echo export STEM_DIR=$HOME/local/share/kstem >> ~/.bashrc
. ~/.bashrc
```