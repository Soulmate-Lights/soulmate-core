# ESP-IDF Soulmate

Installation

Add this to your shell: (.bash_profile or .zshrc)
```
export IDF_PATH=~/esp/esp-idf
export PATH=$HOME/esp/xtensa-esp32-elf/bin:$PATH
```

Run these scripts

(from https://docs.espressif.com/projects/esp-idf/en/v3.1.5/get-started/macos-setup.html)

```
sudo easy_install pip

cd ~/esp
git clone -b v3.3 --recursive https://github.com/espressif/esp-idf.git
cd esp-idf
git submodule update --init --recursive
python -m pip install --user -r $IDF_PATH/requirements.txt

mkdir -p ~/esp
cd ~/esp
wget https://dl.espressif.com/dl/xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz
tar -xzf xtensa-esp32-elf-osx-1.22.0-80-g6c4433a-5.2.0.tar.gz
```

To run
```
make -C panel
```
