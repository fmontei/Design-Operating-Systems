=== USAGE ===
1) Configure the directories in ytfs.env and make sure all the directories exist on disk 
(For example, the original root directory is music/, make sure you execute 
“mkdir music” in the same directory as ytfs.py).
2) Configure the environment variables in ytfs.env. Examples have been provided for you.
3) Execute:
    a) ./run.sh
    OR:
    a) make clean all
    b) source ytfs.env 
    c) python ytfs.py 
    OPTIONAL if c) automatically fails to execute C routine: ./ytfs <mount_directory>

=== SYSTEM SETUP ===
1) Place boto.cfg in /etc/boto.cfg

=== PYTHON SETUP ===
1) Install pip: http://pip.readthedocs.org/en/stable/installing/
2) Execute:
    a) sudo pip install -U boto
    b) sudo pip install -U eyed3
    c) sudo pip install -U requests

=== C++ SETUP ===
1) Install FUSE for C
    link: http://wiki.vpslink.com/HOWTO:_Install_and_mount_FUSE_filesystems
2) Install sqlite3 for C
    link: http://www.tutorialspoint.com/sqlite/sqlite_installation.htm
