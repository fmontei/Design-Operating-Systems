=== USAGE ===
1) python ytfs -d <root_directory>
2) ./ytfs <mount_directory>

This will be streamlined soon into one Python call later

=== PYTHON SETUP ===
1) install pip: http://pip.readthedocs.org/en/stable/installing/
2a) run: sudo pip install -U boto
2b) run: sudo pip install -U eyed3
3) Create file boto.cfg from the credentials.csv file I'm attaching. 

Include the [Credentials] header. 
Fill in the pieces in {} with the data in credentials.csv.
credentials.csv file is not on Github for security reasons.

boto.cfg format:

[Credentials]
aws_access_key_id = {ACCESS KEY ID}
aws_secret_access_key = {SECRET ACCESS KEY}

4) Put the file in /etc/boto.cfg

=== C SETUP ===
1) Install sqlite3 for C

=== USAGE ===
1) Download: python ytfs.py -d {FOLDER you want to download all files to}
2) Upload: python ytfs.py -u {path to a FILE or a FOLDER containing files}
   
   Example of absolute path: /home/osboxes/Desktop/Design-Operating-Systems/project_3_fm

=== DEPENDENCIES ==
-pip
-boto (Python module for storing music in cloud) 
-eyed3 (Python module for extracting mp3 headers)


