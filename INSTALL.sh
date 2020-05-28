rm -rf ~/soulmate
sudo easy_install pip
git clone --recursive --recurse-submodules https://github.com/Soulmate-Lights/soulmate-core.git ~/soulmate
python -m pip install --user -r ~/soulmate/esp-idf/requirements.txt
ln -s ~/soulmate/bin/soulmate /usr/local/bin/soulmate

# If you need to install the drivers, open this app:
# open ~/soulmate/drivers/Install\ CP210x\ VCP\ Driver.app
