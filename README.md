
https://docs.kernel.org/i2c/dev-interface.html
https://devblogs.microsoft.com/cppblog/clear-functional-c-documentation-with-sphinx-breathe-doxygen-cmake/

sudo apt install i2c-tools
sudo apt install libi2c-dev
sudo raspi-config nonint do_i2c 0
sudo apt install cmake
sudo apt install doxygen
sudo apt install pip
sudo apt install python3-venv
pip install sphinx
pip install sphinx_rtd_theme
pip install breathe

On /boot/config.txt:

-> dtparam=i2c_arm=on